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


		template<typename T, unsigned int bits = 8 * sizeof(T)>
		class SPIDevice : public Device, public SendDevice<T, bits>, public Shared_Ptr<SPIDevice<T, bits>>
		{
		public:
			using SendDevice<T, bits>::DataType;
			using Pin = unsigned int;
			enum class SPIMode
			{
				SPI_0, SPI_1, SPI_2, SPI_3
			};
			enum class SPIDataOrder
			{
				MSBFIRST, LSBFIRST
			};

			SPIDevice(const Pin chipSelect, const unsigned int speed, SPIDataOrder order = SPIDataOrder::MSBFIRST, SPIMode mode = SPIMode::SPI_0, const Pin clock = 0, const Pin mosi = 0, const Pin miso = 0) :
				Device(), SendDevice<T, bits>(), chipSelect(chipSelect), speed(speed), order(order), mode(mode), clock(clock), miso(miso), mosi(mosi)
			{

			}

			using Shared_Ptr<SPIDevice<T, bits>>::Shared;
			using Shared_Ptr<SPIDevice<T, bits>>::make;
		private:
			const Pin chipSelect, clock, miso, mosi;
			const unsigned int speed;
			const SPIMode mode;
			const SPIDataOrder order;
		};
	}
}
