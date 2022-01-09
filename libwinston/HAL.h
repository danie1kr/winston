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
		extern void error(const std::string& error);
		extern void fatal(const std::string reason);

		extern void delay(const unsigned int ms);

		extern void storageSetFilename(std::string filename);
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
			virtual const Result recv(std::vector<unsigned char>& data) = 0;

			const bool isConnected();

		protected:
			State state;
		};


		template<typename T>
		class SPIDevice : public Device, public SendDevice<T>, public Shared_Ptr<SPIDevice<T>>
		{
		public:
			using SendDevice<T>::DataType;
			using Pin = unsigned int;
			enum class SPIMode
			{
				SPI_0, SPI_1, SPI_2, SPI_3
			};
			enum class SPIDataOrder
			{
				MSBFirst, LSBFirst
			};

			SPIDevice(const Pin chipSelect, const unsigned int speed, SPIDataOrder order = SPIDataOrder::MSBFirst, SPIMode mode = SPIMode::SPI_0, const Pin clock = 0, const Pin mosi = 0, const Pin miso = 0) :
				Device(), SendDevice<T>(), chipSelect(chipSelect), clock(clock), miso(miso), mosi(mosi), speed(speed), mode(mode), order(order)
			{

			}

			using Shared_Ptr<SPIDevice<T>>::Shared;
			using Shared_Ptr<SPIDevice<T>>::make;
		protected:
			const Pin chipSelect, clock, miso, mosi;
			const unsigned int speed;
			const SPIMode mode;
			const SPIDataOrder order;
		};
	}
}
