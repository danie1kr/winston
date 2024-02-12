#pragma once
#include <cstdint>
#include <stddef.h>
#include <string>

namespace winston
{
	using Address = uint16_t;

	// uint16_t Address, uint8_t Zone, uint16_t LocoAddress, uint8_t Present
	struct SerialMessage
	{
		enum Operation : unsigned char
		{
			RailComOccupancy = 2
		};
		const unsigned char header;

		SerialMessage(const Operation op, unsigned char size);
		virtual ~SerialMessage() = default;

		const Operation op() const;
		const size_t size() const;
	};
	
	struct RailComDetectorSerialMessage : public SerialMessage
	{
		RailComDetectorSerialMessage(const Address detector, const uint8_t zone, const Address loco, const uint8_t present);
		RailComDetectorSerialMessage(const unsigned char data, const Address detector, const Address loco);

		const unsigned char data;
		const Address detector;
		const Address loco;

		const bool present() const;
		const unsigned char zone() const;
	};

	struct URI
	{
		const std::string host;
		const unsigned int port;
		const std::string resource;
		const bool secure;

		URI(const std::string host, const unsigned int port = 8080, const std::string resource = "/", const bool secure = false);
		const std::string toString() const;
	};
};