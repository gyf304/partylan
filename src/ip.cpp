#include <stdexcept>

#include "ip.h"

namespace lpvpn::ip {
	// Packet
	Packet::Packet(std::span<uint8_t> packet) : packet(packet) {}
	Packet::~Packet() {}
	uint8_t Packet::version() {
		return packet[0] >> 4;
	}
	Packet4 Packet::toPacket4() {
		if (version() != 4) {
			throw std::runtime_error("Packet is not IPv4");
		}
		return Packet4(packet);
	}

	// Address4
	Address4::Address4(std::span<uint8_t, 4> addr) {
		memcpy(this->addr.data(), addr.data(), 4);
	}
	Address4::Address4(std::array<uint8_t, 4> addr) : addr(addr) {}
	Address4::Address4(uint32_t addr) {
		for (size_t i = 0; i < 4; i++) {
			this->addr[i] = (addr >> (8 * (3 - i))) & 0xFF;
		}
	}
	Address4::~Address4() {}
	uint32_t Address4::toUint32() const {
		uint32_t ret = 0;
		for (size_t i = 0; i < 4; i++) {
			ret |= addr[i] << (8 * (3 - i));
		}
		return ret;
	}
	bool Address4::isBroadcast() const {
		return toUint32() == 0xFFFFFFFF;
	}
	bool Address4::isMulticast() const {
		return (addr[0] & 0xF0) == 0xE0;
	}
	bool Address4::operator==(const Address4& other) const {
		return addr == other.addr;
	}
	Address4 Address4::operator+(uint32_t other) const {
		return Address4(toUint32() + other);
	}
	Address4 Address4::operator-(uint32_t other) const {
		return Address4(toUint32() - other);
	}
	Address4 Address4::operator++() {
		return Address4(toUint32() + 1);
	}
	Address4 Address4::operator--() {
		return Address4(toUint32() - 1);
	}
	bool Address4::operator<(const Address4& other) const {
		return toUint32() < other.toUint32();
	}
	bool Address4::operator>(const Address4& other) const {
		return toUint32() > other.toUint32();
	}
	std::string Address4::toString() const {
		std::string ret;
		for (size_t i = 0; i < 4; i++) {
			ret += std::to_string(addr[i]);
			if (i != 3) {
				ret += ".";
			}
		}
		return ret;
	}

	// Subnet4
	Subnet4::Subnet4(Address4 addr, uint8_t prefix): prefix(prefix) {
		this->addr = addr.addr;
	}
	Subnet4::Subnet4(std::span<uint8_t, 4> addr, uint8_t prefix): prefix(prefix) {
		memcpy(this->addr.data(), addr.data(), 4);
	}
	Subnet4::Subnet4(std::array<uint8_t, 4> addr, uint8_t prefix): prefix(prefix) {
		this->addr = addr;
	}
	Subnet4::~Subnet4() {}
	Address4 Subnet4::start() const {
		uint32_t start = 0;
		for (size_t i = 0; i < 4; i++) {
			start |= addr[i] << (8 * (3 - i));
		}
		start &= ~((1 << (32 - prefix)) - 1);
		return Address4(start);
	}
	Address4 Subnet4::end() const {
		uint32_t end = 0;
		for (size_t i = 0; i < 4; i++) {
			end |= addr[i] << (8 * (3 - i));
		}
		end |= (1 << (32 - prefix)) - 1;
		return Address4(end);
	}
	Address4 Subnet4::mask() const {
		uint32_t mask = 0xFFFFFFFF;
		mask <<= (32 - prefix);
		return Address4(mask);
	}
	uint32_t Subnet4::size() const {
		return 1 << (32 - prefix);
	}
	bool Subnet4::operator==(const Subnet4& other) const {
		return addr == other.addr && prefix == other.prefix;
	}

	Packet4::Packet4(std::span<uint8_t> packet) : Packet(packet) {}
	Packet4::~Packet4() {}

	Address4 Packet4::srcAddr() {
		return Address4(std::span<uint8_t, 4>(packet.data() + 12, 4));
	}

	Address4 Packet4::dstAddr() {
		return Address4(std::span<uint8_t, 4>(packet.data() + 16, 4));
	}

	std::span<uint8_t> Packet4::payload() {
		if (version() == 4) {
			return packet.subspan(20);
		} else if (version() == 6) {
			return packet.subspan(40);
		} else {
			return packet.subspan(0, 0);
		}
	}

	void Packet4::recalculateChecksum() {
		uint16_t checksum = 0;
		for (size_t i = 0; i < this->packet.size(); i++) {
			if (i == 10 || i == 11) {
				continue;
			}
			uint16_t prev = checksum;
			checksum += this->packet[i];
			if (checksum < prev) {
				checksum++;
			}
		}
	}

	void Packet4::setSrcAddr(Address4 addr) {
		memcpy(packet.data() + 12, addr.addr.data(), 4);
		recalculateChecksum();
	}

	void Packet4::setDstAddr(Address4 addr) {
		memcpy(packet.data() + 16, addr.addr.data(), 4);
		recalculateChecksum();
	}
}
