#pragma once
#include <cstddef>
#include <cassert>
#include <span>
#include <functional>
#include <memory>

#include "ip.h"

namespace lpvpn::tun {
	using namespace lpvpn::ip;
	class Tun {
		public:
		Tun();
		~Tun();
		void write(Packet &packet);
		void onData(std::function<void(Packet&)> cb);
		void setIP4(const Subnet4 &subnet);

		private:
		class Impl;
		std::unique_ptr<Impl> impl;
	};
}
