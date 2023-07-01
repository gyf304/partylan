#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <thread>

#include <steam_api.h>
#include "ip.h"


namespace lpvpn::steam {
	using namespace lpvpn::ip;

	// RAII wrapper for Steam API
	class Steam {
		public:
		Steam();
		~Steam();
		private:
		std::atomic<bool> running;
		std::thread thread;
	};

	class SteamNet {
		public:
		struct Endpoint {
			std::string name;
			Address4 addr;
			Address4 canonicalAddr;
			bool isOnline;
		};

		SteamNet(std::shared_ptr<Steam> steam);
		~SteamNet();

		void write(Packet &packet);
		void onData(std::function<void(Packet&)> cb);
		void onEndpoints(std::function<void(std::vector<Endpoint>&)> cb);

		Subnet4 localAddr();

		private:
		class Impl;
		std::unique_ptr<Impl> impl;
	};
}
