#pragma once

#include <string>
#include <vector>
#include "WinstonTypes.h"

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace winston
{
	namespace hal
	{
		extern TimePoint now();
		extern void text(const std::string& text);
		extern void error(const std::string& error);
		extern void fatal(const std::string reason);

		extern void delay(const unsigned int ms);

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

		class SerialDevice : public Shared_Ptr<SerialDevice>, public ReadDevice<unsigned char>, public SendDevice<unsigned char>
		{
		public:
			enum class SerialDataBits {
				SERIAL_DATABITS_5, /**< 5 databits */
				SERIAL_DATABITS_6, /**< 6 databits */
				SERIAL_DATABITS_7, /**< 7 databits */
				SERIAL_DATABITS_8,  /**< 8 databits */
				SERIAL_DATABITS_16,  /**< 16 databits */
			};

			/**
			 * number of serial stop bits
			 */
			enum class SerialStopBits {
				SERIAL_STOPBITS_1, /**< 1 stop bit */
				SERIAL_STOPBITS_1_5, /**< 1.5 stop bits */
				SERIAL_STOPBITS_2, /**< 2 stop bits */
			};

			/**
			 * type of serial parity bits
			 */
			enum class SerialParity {
				SERIAL_PARITY_NONE, /**< no parity bit */
				SERIAL_PARITY_EVEN, /**< even parity bit */
				SERIAL_PARITY_ODD, /**< odd parity bit */
				SERIAL_PARITY_MARK, /**< mark parity */
				SERIAL_PARITY_SPACE /**< space bit */
			};

			using DataType = unsigned char;

			virtual bool init(const size_t portNumber, const size_t bauds = 115200,
				const SerialDataBits databits = SerialDataBits::SERIAL_DATABITS_8,
				const SerialParity parity = SerialParity::SERIAL_PARITY_NONE,
				const SerialStopBits stopbits = SerialStopBits::SERIAL_STOPBITS_1) = 0;

			using Shared_Ptr<SerialDevice>::Shared;
			using Shared_Ptr<SerialDevice>::make;
		};

		class StorageInterface : public Shared_Ptr<StorageInterface>
		{
		protected:
			StorageInterface(const size_t maxSize = 0);
			size_t maxSize;
		public:
			using Shared_Ptr<StorageInterface>::Shared;
			using Shared_Ptr<StorageInterface>::make;
			virtual const Result init() = 0;
			virtual const Result read(const size_t address, std::vector<unsigned char>& content, const size_t length = 1) = 0;
			virtual const Result read(const size_t address, std::string& content, const size_t length = 1) = 0;
			virtual const Result write(const size_t address, std::vector<unsigned char>& content, const size_t length = 0) = 0;
			virtual const Result write(const size_t address, unsigned char content) = 0;
			virtual const Result write(const size_t address, std::string& content, const size_t length = 0) = 0;
			virtual const Result sync() = 0;
		};
	}
}
