#pragma once

#include <string>
#include <vector>
#include "WinstonTypes.h"

namespace winston
{
	namespace hal
	{
		extern unsigned long long now();
		extern void text(const std::string& text);
		extern void fatal(const std::string text);

		extern void delay(const unsigned int ms);

		extern const unsigned char storageRead(const size_t address);
		extern void storageWrite(const size_t address, const uint8_t data);
		extern bool storageCommit();

		extern void init();

		class UDPSocket : public Shared_Ptr<UDPSocket>
		{
		public:
			enum class State
			{
				NotConnected,
				Connecting,
				Connected,
				Closing
			};

			UDPSocket(const std::string ip, const unsigned short port);
			virtual const Result send(const std::vector<unsigned char> data) = 0;

			const bool isConnected();

		protected:
			State state;
		};

		class Device : public Shared_Ptr<Device>
		{
		public:
			virtual void init() = 0;
			virtual Result send(std::vector<unsigned char> data) = 0;
			virtual unsigned int receive() = 0;
		};
	}
}
