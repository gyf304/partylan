#include <iostream>
#include <thread>
#include <stdexcept>
#include <system_error>
#include <wintunimp.h>
#include <guiddef.h>
#include <netioapi.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#include <netlistmgr.h>
#include <atlcomcli.h>

#include "tun.h"

// F8D2D65B-7012-4602-805E-FD00529352DA
const GUID LPVPN_ADAPTER_GUID = {
	0xF8D2D65B, 0x7012, 0x4602,
	{ 0x80, 0x5E, 0xFD, 0x00, 0x52, 0x93, 0x52, 0xDA }
};

const wchar_t *LPVPN_ADAPTER_NAME = L"PartyLAN";

namespace lpvpn::tun {
	class Tun::Impl {
		public:
		Impl() {
			CoInitialize(NULL);

			wintun = WintunOpenAdapter(LPVPN_ADAPTER_NAME);
			if (wintun != nullptr) {
				WintunCloseAdapter(wintun);
			}

			wintun = WintunCreateAdapter(LPVPN_ADAPTER_NAME, LPVPN_ADAPTER_NAME, &LPVPN_ADAPTER_GUID);
			if (wintun == nullptr) {
				throw std::runtime_error("Failed to create adapter");
			}

			WintunGetAdapterLUID(wintun, &wintunLUID);

			sessionHandle = WintunStartSession(wintun, WINTUN_MAX_RING_CAPACITY);
			if (sessionHandle == nullptr) {
				WintunCloseAdapter(wintun);
				wintun = nullptr;
				throw std::runtime_error("Failed to start session");
			}

			thread = std::thread([this]() {
				while (this->running) {
					DWORD incomingPacketSize;
					auto incomingPacket = WintunReceivePacket(sessionHandle, &incomingPacketSize);
					if (incomingPacket != nullptr) {
						std::span<uint8_t> data(reinterpret_cast<uint8_t*>(incomingPacket), incomingPacketSize);
						if (this->dataCb != nullptr) {
							auto packet = Packet(data);
							this->dataCb(packet);
						}
						WintunReleaseReceivePacket(sessionHandle, incomingPacket);
					} else if (GetLastError() == ERROR_NO_MORE_ITEMS) {
						WaitForSingleObject(WintunGetReadWaitEvent(sessionHandle), 1000);
					} else {
						break;
					}
				}
			});
		};

		~Impl() {
			running = false;
			thread.join();
			if (sessionHandle) {
				WintunEndSession(sessionHandle);
			}
			if (wintun) {
				WintunCloseAdapter(wintun);
			}

			CoUninitialize();
		};

		void write(Packet &packet) {
			auto wintunPacket = WintunAllocateSendPacket(sessionHandle, packet.packet.size());
			if (wintunPacket != nullptr) {
				memcpy(wintunPacket, packet.packet.data(), packet.packet.size());
				WintunSendPacket(sessionHandle, wintunPacket);
			}
		};

		void onData(std::function<void(Packet &packet)> cb) {
			this->dataCb = cb;
		};

		void setCategory() {
			HRESULT hr = S_OK;
			CComPtr<INetworkListManager> pLocalNLM;
			CComPtr<IEnumNetworks> pEnumNetworks;
			hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pLocalNLM));
			if (hr != S_OK) {
				return;
			}
			hr = pLocalNLM->GetNetworks(NLM_ENUM_NETWORK_ALL, &pEnumNetworks);
			if (hr != S_OK) {
				return;
			}
			while (1)
			{
				CComPtr<INetwork> pNetwork;
				hr = pEnumNetworks->Next(1, &pNetwork, NULL);
				if (hr == S_OK)
				{
					BSTR name = NULL;
					pNetwork->GetName(&name);
					if (wcscmp(name, LPVPN_ADAPTER_NAME) == 0) {
						pNetwork->SetCategory(NLM_NETWORK_CATEGORY_PRIVATE);
						std::cout << "Network category set" << std::endl;
					}
				}
				else
					break;
			}
		}

		void setIP4(const Subnet4 &subnet) {
			NET_IFINDEX ifindex;

			if (currentSubnet != nullptr && subnet == *currentSubnet) {
				return;
			}

			if (ipContext != 0) {
				DeleteIPAddress(ipContext);
				ipContext = 0;
			}

			if (ConvertInterfaceLuidToIndex(&wintunLUID, &ifindex) != NO_ERROR) {
				throw std::runtime_error("Failed to get interface index");
			}
			auto osAddr = SOCKADDR_IN();
			osAddr.sin_family = AF_INET;
			osAddr.sin_addr.S_un.S_un_b.s_b1 = subnet.addr[0];
			osAddr.sin_addr.S_un.S_un_b.s_b2 = subnet.addr[1];
			osAddr.sin_addr.S_un.S_un_b.s_b3 = subnet.addr[2];
			osAddr.sin_addr.S_un.S_un_b.s_b4 = subnet.addr[3];

			auto mask = subnet.mask();
			auto osMask = SOCKADDR_IN();
			osMask.sin_family = AF_INET;
			osMask.sin_addr.S_un.S_un_b.s_b1 = mask.addr[0];
			osMask.sin_addr.S_un.S_un_b.s_b2 = mask.addr[1];
			osMask.sin_addr.S_un.S_un_b.s_b3 = mask.addr[2];
			osMask.sin_addr.S_un.S_un_b.s_b4 = mask.addr[3];

			auto ret = AddIPAddress(
				osAddr.sin_addr.S_un.S_addr,
				osMask.sin_addr.S_un.S_addr,
				ifindex,
				&ipContext,
				&ipInstance
			);
			if (ret != NO_ERROR) {
				throw std::system_error(std::error_code(ret, std::system_category()));
			}
			auto newSubnet = std::make_unique<Subnet4>(subnet);
			currentSubnet = std::move(newSubnet);

			setCategory();
		};

		private:
		std::thread thread;
		std::atomic<bool> running = true;
		std::unique_ptr<Subnet4> currentSubnet;

		ULONG ipContext = 0;
		ULONG ipInstance = 0;

		WINTUN_ADAPTER_HANDLE wintun = nullptr;
		WINTUN_SESSION_HANDLE sessionHandle = nullptr;
		NET_LUID wintunLUID;

		std::function<void(Packet &packet)> dataCb;
	};

	Tun::Tun() : impl(std::make_unique<Impl>()) {};
	Tun::~Tun() {};
	void Tun::write(Packet &packet) {
		this->impl->write(packet);
	};
	void Tun::onData(std::function<void(Packet &packet)> cb) {
		this->impl->onData(cb);
	};
	void Tun::setIP4(const Subnet4 &subnet) {
		this->impl->setIP4(subnet);
	};
}
