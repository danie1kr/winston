#include "WinstonSharedTypes.h"

namespace winston
{
	SerialMessage::SerialMessage(const Operation op, unsigned char size)
		: header(((op << 4) & 0xF0) | (size & 0x0F))
	{

	}

	const SerialMessage::Operation SerialMessage::op() const
	{
		return (Operation)((header >> 4) & 0x0F);
	}

	const size_t SerialMessage::size() const
	{
		return header & 0x0F;
	}

	RailComDetectorSerialMessage::RailComDetectorSerialMessage(const Address detector, const uint8_t zone, const Address loco, const uint8_t present)
		: SerialMessage(RailComOccupancy, sizeof(RailComDetectorSerialMessage)), data( (present ? 1 << 4 : 0) | (zone & 0x0F)), detector(detector), loco(loco)
	{

	}

	RailComDetectorSerialMessage::RailComDetectorSerialMessage(const unsigned char data, const Address detector, const Address loco)
		: SerialMessage(RailComOccupancy, sizeof(RailComDetectorSerialMessage)), data(data), detector(detector), loco(loco)
	{

	}

	const bool RailComDetectorSerialMessage::present() const
	{
		return (this->data >> 4) & 0b1;
	}

	const unsigned char RailComDetectorSerialMessage::zone() const
	{
		return this->data & 0x0F;
	}

	URI::URI(const std::string host, const unsigned int port, const std::string resource, const bool secure)
		: host{ host }, port{ port }, resource{ resource }, secure{ secure }
	{

	}

	const std::string URI::toString() const
	{
		return (secure ? "wss://" : "ws://") + host + ":" + std::to_string(port) + resource;
	}
}