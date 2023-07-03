#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <queue>
#include <fstream>
#include <iostream>

#include <steam_api.h>
#include "ui.h"
#include "steam.h"
#include "tun.h"
#include "log.h"

enum EventCode {
	EVENT_UI_EXIT = 1,
	EVENT_UI_CLICK,
};

using namespace lpvpn;

class LogRedirect {
	public:
	LogRedirect(const std::string &filename) {
		old = std::clog.rdbuf();
		file = std::ofstream(filename, std::ios_base::app);
		std::clog.rdbuf(file.rdbuf());
	}

	~LogRedirect() {
		std::clog.rdbuf(old);
		file.flush();
		file.close();
	}

	private:
	std::ofstream file;
	std::streambuf *old;
};

int appMain(int argc, char **argv) {
	if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID)) {
		return 0;
	}

	// check privacy flag
	bool privacy = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--privacy") == 0 || strcmp(argv[i], "-privacy") == 0) {
			privacy = true;
		}
	}

	std::string logFilename = "lpvpn.log.txt";
	LogRedirect logRedirect(logFilename);

	std::atomic<bool> hasEvent;
	std::mutex eventMutex;
	std::queue<EventCode> eventQueue;
	std::atomic<bool> running = true;

	auto ui = ui::UI();
	ui.onEvent([&](ui::Event &ev) {
		std::lock_guard<std::mutex> lk(eventMutex);
		switch (ev.code)
		{
		case ui::EventCode::EXIT:
			eventQueue.push(EventCode::EVENT_UI_EXIT);
			hasEvent.store(true);
			hasEvent.notify_all();
			break;
		default:
			break;
		}
	});

	try {
		auto steam = std::make_shared<steam::Steam>();
		auto steamNet = steam::SteamNet(steam);
		auto tun = tun::Tun();

		auto localIP = steamNet.localAddr();
		ui.notify("Local IP", localIP.toString());
		tun.setIP4(localIP);
		tun.onData([&](ip::Packet &packet) {
			steamNet.write(packet);
		});

		steamNet.onData([&](ip::Packet &packet) {
			tun.write(packet);
		});

		auto exitCb = [&](ui::MenuItem &mi){
			std::lock_guard<std::mutex> lk(eventMutex);
			eventQueue.push(EventCode::EVENT_UI_EXIT);
			hasEvent.store(true);
			hasEvent.notify_all();
		};

		auto endpointItemCb = [&](ui::MenuItem &mi) {
			auto endpoint = *std::static_pointer_cast<steam::SteamNet::Endpoint>(mi.userData);
			ui.setClipboard(endpoint.addr.toString());
			ui.notify("PartyLAN", "IP copied to clipboard");
		};

		auto urlCb = [&](ui::MenuItem &mi) {
			ui.openURL(*std::static_pointer_cast<std::string>(mi.userData));
		};

		steamNet.onEndpoints([&](std::vector<steam::SteamNet::Endpoint> &endpoints) {
			auto allFriendsMenu = std::make_shared<std::vector<ui::MenuItem>>();
			auto onlineFriendsMenu = std::make_shared<std::vector<ui::MenuItem>>();
			for (auto &endpoint : endpoints) {
				auto ptr = std::make_shared<steam::SteamNet::Endpoint>(endpoint);
				auto text = endpoint.name + " (" + endpoint.addr.toString() + ")";
				if (privacy) {
					// if privacy mode is enabled, only show first 3 characters of name
					auto name = endpoint.name;
					if (name.length() > 3) {
						name = name.substr(0, 3) + std::string(name.length() - 3, '*');
					}
					// hide middle 2 bytes of IP address, since they are computed from steam ids and can be reversed
					auto ipstr = endpoint.addr.toString();
					auto firstDotPos = ipstr.find_first_of('.');
					auto lastDotPos = ipstr.find_last_of('.');
					if (firstDotPos != std::string::npos && lastDotPos != std::string::npos && firstDotPos != lastDotPos) {
						ipstr = ipstr.substr(0, firstDotPos + 1) + std::string(lastDotPos - firstDotPos - 1, '*') + ipstr.substr(lastDotPos);
					}
					text = name + " (" + ipstr + ")";
				}
				allFriendsMenu->push_back({
					text,
					nullptr,
					endpointItemCb,
					ptr,
				});
				if (endpoint.isOnline) {
					onlineFriendsMenu->push_back({
						text,
						nullptr,
						endpointItemCb,
						ptr,
					});
				}
			}
			auto menu = std::make_unique<ui::MenuItem>();
			auto moreMenu = std::make_shared<std::vector<ui::MenuItem>>();
			moreMenu->push_back({ "Version: " LPVPN_VERSION, nullptr, nullptr, nullptr });
			moreMenu->push_back({ "Website / Support", nullptr, urlCb, std::make_shared<std::string>("https://github.com/gyf304/partylan") });
			moreMenu->push_back({ "LAN Game DB", nullptr, urlCb, std::make_shared<std::string>("https://github.com/gyf304/partylan/blob/main/resources/github/lan-games-db/lan-games.csv") });
			menu->submenu = std::make_shared<std::vector<ui::MenuItem>>();
			menu->submenu->push_back({ "All Friends", allFriendsMenu, nullptr, nullptr });
			menu->submenu->push_back({ "Online Friends", onlineFriendsMenu, nullptr, nullptr });
			if (privacy) {
				menu->submenu->push_back({ "Privacy Mode Enabled, IP hidden", nullptr, nullptr, nullptr });
			} else {
				menu->submenu->push_back({ "My IP: " + localIP.toString(), nullptr, nullptr, nullptr });
			}
			menu->submenu->push_back({ "More", moreMenu, nullptr, nullptr});
			menu->submenu->push_back({ "Exit", nullptr, exitCb, nullptr });
			ui.setMenu(std::move(menu));
		});

		while (true) {
			hasEvent.store(false);
			// semantics for wait() is to wait until value is different
			// from the one passed in
			hasEvent.wait(false);

			{
				std::lock_guard<std::mutex> lk(eventMutex);
				while (!eventQueue.empty()) {
					auto ev = eventQueue.front();
					eventQueue.pop();

					switch (ev) {
						case EventCode::EVENT_UI_EXIT:
							LOG("Exiting");
							return 0;
					}
				}
			}
		}
	} catch (std::exception &e) {
		LOG("Exception" << e.what());
		ui.alert("Error", e.what());
		return 1;
	}

	return 0;
}
