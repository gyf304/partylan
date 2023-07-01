#pragma once
#include <array>
#include <span>
#include <string>
#include <cstddef>
#include <cstdint>

namespace lpvpn::ip {
	class Packet4;

	class Packet {
		public:
		Packet(std::span<uint8_t> packet);
		~Packet();
		uint8_t version();
		Packet4 toPacket4();
		std::span<uint8_t> packet;
	};

	class Address4 {
		public:
		Address4() = default;
		Address4(std::span<uint8_t, 4> addr);
		Address4(std::array<uint8_t, 4> addr);
		Address4(uint32_t addr);
		~Address4();
		uint32_t toUint32() const;
		bool operator==(const Address4& other) const;
		Address4 operator+(uint32_t other) const;
		Address4 operator-(uint32_t other) const;
		Address4 operator++();
		Address4 operator--();
		bool operator<(const Address4& other) const;
		bool operator>(const Address4& other) const;
		std::string toString() const;

		std::array<uint8_t, 4> addr;
	};

	class Subnet4: public Address4 {
		public:
		uint8_t prefix;

		Subnet4(Address4 addr, uint8_t prefix);
		Subnet4(std::span<uint8_t, 4> addr, uint8_t prefix);
		Subnet4(std::array<uint8_t, 4> addr, uint8_t prefix);
		~Subnet4();

		Address4 start() const;
		Address4 end() const;
		Address4 mask() const;
		uint32_t size() const;

		bool operator==(const Subnet4& other) const;
	};

	class Packet4: public Packet {
		public:
		Packet4(std::span<uint8_t> packet);
		~Packet4();

		Address4 srcAddr();
		Address4 dstAddr();
		std::span<uint8_t> payload();

		void recalculateChecksum();
		void setSrcAddr(Address4 addr);
		void setDstAddr(Address4 addr);
	};
}
