#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <mutex>

#include "steam.h"
#include "log.h"

#define FREE_EVERY 1000

const auto LOOP_INTERVAL = std::chrono::milliseconds(10);
const auto FRIEND_REFRESH_INTERVAL = std::chrono::seconds(10);

namespace lpvpn::steam {
	Steam::Steam() {
		if (!SteamAPI_Init()) {
			throw std::runtime_error("SteamAPI_Init failed");
		}
		running = true;
		thread = std::thread([this]() {
			LOG("Steam callback thread started");
			while (this->running) {
				SteamAPI_RunCallbacks();
				std::this_thread::sleep_for(LOOP_INTERVAL);
			}
		});
	}

	Steam::~Steam() {
		running = false;
		thread.join();
		SteamAPI_Shutdown();
	}

	class SteamNet::Impl {
		public:
		Impl(std::shared_ptr<Steam> steam): steam(steam) {
			localSteamID = SteamUser()->GetSteamID();
			_localAddr = assignAddr(localSteamID);

			refreshEndpoints();
			SteamNetworkingUtils()->InitRelayNetworkAccess();

			thread = std::thread([this]() {
				while (this->running) {
					SteamNetworkingMessage_t *msg = nullptr;
					auto count = SteamNetworkingMessages()->ReceiveMessagesOnChannel(0, &msg, 1);
					if (count != 1) {
						std::this_thread::sleep_for(LOOP_INTERVAL);
						continue;
					}
					auto steamID = msg->m_identityPeer.GetSteamID();
					auto size = msg->GetSize();
					std::vector<uint8_t> data(size);
					memcpy(data.data(), msg->m_pData, size);
					msg->Release();

					readPacketCount++;
					if (readPacketCount % FREE_EVERY == 0) {
						SteamAPI_ReleaseCurrentThreadMemory();
					}

					if (!steamIDToAddr.contains(steamID)) {
						continue;
					}
					Address4 addr;
					{
						std::lock_guard<std::mutex> lk(refreshMutex);
						addr = steamIDToAddr[steamID];
					}
					auto packet = Packet(data);
					if (packet.version() != 4) {
						continue;
					}
					packet.toPacket4().setSrcAddr(addr);
					packet.toPacket4().setDstAddr(_localAddr);
					onDataCb(packet);
				}
			});

			refreshThread = std::thread([this]() {
				auto elapsed = std::chrono::milliseconds(0);
				while (this->running) {
					if (elapsed >= FRIEND_REFRESH_INTERVAL) {
						SteamAPI_ReleaseCurrentThreadMemory();
						refreshEndpoints();
						elapsed = std::chrono::milliseconds(0);
					}
					std::this_thread::sleep_for(LOOP_INTERVAL);
					elapsed += LOOP_INTERVAL;
				}
			});
		}

		~Impl() {
			running = false;
			thread.join();
			refreshThread.join();

			LOG("SteamNet::Impl destroyed");
		}

		Subnet4 localAddr() {
			return Subnet4(_localAddr, 10);
		}

		void write(Packet &packet) {
			if (packet.version() != 4) {
				return;
			}
			auto addr = packet.toPacket4().dstAddr();
			if (!addrToSteamID.contains(addr)) {
				return;
			}
			auto steamID = addrToSteamID[addr];
			// send packet to steamID
			SteamNetworkingIdentity identity;
			identity.SetSteamID(steamID);
			auto result = SteamNetworkingMessages()->SendMessageToUser(
				identity,
				packet.packet.data(), packet.packet.size(),
				k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_AutoRestartBrokenSession,
				0
			);
			
			if (result != k_EResultOK) {
				LOG("Failed to send packet to " << steamID.ConvertToUint64());
				LOG("Error: " << result);
			}

			writtenPacketCount++;
			if (writtenPacketCount % FREE_EVERY == 0) {
				SteamAPI_ReleaseCurrentThreadMemory();
			}
		}

		void onData(std::function<void(Packet&)> cb) {
			onDataCb = cb;
		}

		void onEndpoints(std::function<void(std::vector<Endpoint>&)> cb) {
			if (cb != nullptr) {
				cb(_endpoints);
			}
			onEndpointsCb = cb;
		}

		private:
		std::shared_ptr<Steam> steam;
		std::thread thread;
		std::thread refreshThread;
		std::atomic<bool> running = true;
		std::uint32_t writtenPacketCount = 0;
		std::uint32_t readPacketCount = 0;

		CSteamID localSteamID;
		Address4 _localAddr;
		std::vector<Endpoint> _endpoints;
		std::map<CSteamID, Address4> steamIDToAddr;
		std::map<Address4, CSteamID> addrToSteamID;
		std::mutex refreshMutex;

		std::function<void(Packet&)> onDataCb;
		std::function<void(std::vector<Endpoint>&)> onEndpointsCb;

		STEAM_CALLBACK(Impl, onSteamNetworkingMessagesSessionRequest, SteamNetworkingMessagesSessionRequest_t);
		STEAM_CALLBACK(Impl, onSteamNetworkingMessagesSessionFailed, SteamNetworkingMessagesSessionFailed_t);
		STEAM_CALLBACK(Impl, onPersonaStateChange, PersonaStateChange_t);

		Address4 computeAddr(CSteamID steamID, uint32_t offset = 0) {
			// use IP in the CGNAT range
			// https://en.wikipedia.org/wiki/Carrier-grade_NAT
			auto range = Subnet4({100, 64, 0, 0}, 10);
			auto size = range.size();
			auto mod = (steamID.ConvertToUint64() + offset) % size;
			if (mod == 0) {
				mod = 1;
			} else if (mod == size - 1) {
				mod = size - 2;
			}
			auto canonicalAddr = range.start().toUint32() + mod;
			return Address4(canonicalAddr);
		}

		Address4 assignAddr(CSteamID steamID) {
			if (steamIDToAddr.contains(steamID)) {
				return steamIDToAddr[steamID];
			}
			for (uint32_t offset = 0; offset < 4; offset++) {
				auto addr = computeAddr(steamID, offset);
				if (!addrToSteamID.contains(addr)) {
					steamIDToAddr[steamID] = addr;
					addrToSteamID[addr] = steamID;
					return addr;
				}
			}
			throw std::runtime_error("no available address");
		}

		void refreshEndpoints() {
			std::lock_guard<std::mutex> lk(refreshMutex);
			_endpoints.clear();
			for (auto i = 0; i < SteamFriends()->GetFriendCount(k_EFriendFlagImmediate); i++) {
				auto steamID = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
				auto canonicalAddr = computeAddr(steamID);
				auto addr = assignAddr(steamID);
				bool online = SteamFriends()->GetFriendPersonaState(steamID) != k_EPersonaStateOffline;
				_endpoints.push_back({SteamFriends()->GetFriendPersonaName(steamID), addr, canonicalAddr, online});
			}
			if (onEndpointsCb != nullptr) {
				onEndpointsCb(_endpoints);
			}
		}
	};

	void SteamNet::Impl::onSteamNetworkingMessagesSessionRequest(SteamNetworkingMessagesSessionRequest_t *ev) {
		// accept all requests
		if (ev == nullptr) {
			return;
		}
		SteamNetworkingMessages()->AcceptSessionWithUser(ev->m_identityRemote);
		LOG("Accepted session with " << ev->m_identityRemote.GetSteamID().ConvertToUint64());
	}

	void SteamNet::Impl::onSteamNetworkingMessagesSessionFailed(SteamNetworkingMessagesSessionFailed_t *ev) {
		if (ev == nullptr) {
			return;
		}
		auto steamID = ev->m_info.m_identityRemote.GetSteamID();
		LOG("Session with " << steamID.ConvertToUint64() << " failed");
	}

	void SteamNet::Impl::onPersonaStateChange(PersonaStateChange_t *ev) {
		// refresh endpoints
		if (ev == nullptr) {
			return;
		}
		refreshEndpoints();
	}

	SteamNet::SteamNet(std::shared_ptr<Steam> steam) : impl(std::make_unique<Impl>(steam)) {}
	SteamNet::~SteamNet() {}

	Subnet4 SteamNet::localAddr() {
		return impl->localAddr();
	}

	void SteamNet::onEndpoints(std::function<void(std::vector<Endpoint>&)> cb) {
		return impl->onEndpoints(cb);
	}

	void SteamNet::write(Packet &packet) {
		return impl->write(packet);
	}

	void SteamNet::onData(std::function<void(Packet&)> cb) {
		return impl->onData(cb);
	}
}
