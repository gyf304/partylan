#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <queue>

#include <steam_api.h>
#include "ui.h"
#include "steam.h"
#include "tun.h"

enum EventCode {
	EVENT_UI_EXIT = 1,
	EVENT_UI_CLICK,
};

#ifndef LPVPN_VERSION
#define LPVPN_VERSION "unknown"
#endif

using namespace lpvpn;

int appMain(int argc, char **argv) {
	if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID)) {
		return 0;
	}

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

	steamNet.onEndpoints([&](std::vector<steam::SteamNet::Endpoint> &endpoints) {
		auto allFriendsMenu = std::make_shared<std::vector<ui::MenuItem>>();
		auto onlineFriendsMenu = std::make_shared<std::vector<ui::MenuItem>>();
		for (auto &endpoint : endpoints) {
			auto ptr = std::make_shared<steam::SteamNet::Endpoint>(endpoint);
			auto text = endpoint.name + " (" + endpoint.addr.toString() + ")";
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
		menu->submenu = std::make_shared<std::vector<ui::MenuItem>>();
		menu->submenu->push_back({ "All Friends", allFriendsMenu, nullptr, nullptr });
		menu->submenu->push_back({ "Online Friends", onlineFriendsMenu, nullptr, nullptr });
		menu->submenu->push_back({ "My IP: " + localIP.toString(), nullptr, nullptr, nullptr });
		menu->submenu->push_back({ "Version: " LPVPN_VERSION, nullptr, nullptr, nullptr });
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
						return 0;
				}
			}
		}
	}

	return 0;
}
