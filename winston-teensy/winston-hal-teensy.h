#pragma once
#include "../libwinston/WinstonConfig.h"

#include <Arduino.h>
#include "../libwinston/HAL.h"
#include "../libwinston/Log.h"

#ifdef WINSTON_PLATTFORM_TEENSY
#include "../libwinston/Winston.h"
#include "../libwinston/Signal.h"

#define WINSTON_WITH_HTTP
#define WINSTON_WITH_SDFAT

//#define WINSTON_WITH_QNETHERNET
#ifdef WINSTON_WITH_QNETHERNET
#define USE_NATIVE_ETHERNET         false
#define USE_QN_ETHERNET             true
#include <QNEthernet.h>
using namespace qindesign::network;
#warning untested
#else
#define USE_NATIVE_ETHERNET         true
#define USE_QN_ETHERNET             false
#include <NativeEthernet.h>
#endif
#endif
/*
#define WINSTON_WITH_WEBSOCKET
#define WEBSOCKETS_USE_ETHERNET     true
#define USE_NATIVE_ETHERNET         true

#define WINSTON_WEBSOCKETS_WebSockets2_Generic
//#define WINSTON_WEBSOCKETS_ArduinoWebsockets
#ifdef WINSTON_WITH_WEBSOCKET
#ifdef WINSTON_WEBSOCKETS_ArduinoWebsockets
#include <ArduinoWebsockets.h>
using namespace websockets;
#endif
#ifdef WINSTON_WEBSOCKETS_WebSockets2_Generic
#include <WebSockets2_Generic.h>
using namespace websockets2_generic;
#endif
#endif
*/
#include <SPI.h>

#ifdef WINSTON_TEENSY_FLASHSTRING
namespace winston
{
    namespace hal {
        const std::string __FlashStorageStringtoStd(const __FlashStringHelper* fsh);
        void text(const __FlashStringHelper* fsh);
        void error(const __FlashStringHelper* fsh);
        void fatal(const __FlashStringHelper* fsh);
        size_t stream(const __FlashStringHelper* fsh, std::function<size_t(uint8_t)> target);
    }

    std::string build(const __FlashStringHelper* fsh);
};

constexpr const __FlashStringHelper* operator "" _s(const char* in, size_t len)
{
    return ((const __FlashStringHelper*)(in));
}
#else
/*
const char* operator "" _s(const char* in, size_t len)
{
	return in;
}
*/
#endif

#ifdef WINSTON_HAL_USE_SOCKETS
class UDPSocketTeensy : public winston::hal::Socket, winston::Shared_Ptr<UDPSocketTeensy>
{
public:
	using winston::Shared_Ptr<UDPSocketTeensy>::Shared;
	using winston::Shared_Ptr<UDPSocketTeensy>::make;

	UDPSocketTeensy(const std::string ip, const unsigned short port);
	const winston::Result send(const std::vector<unsigned char> data);
	const winston::Result recv(std::vector<unsigned char>& data);
private:

	EthernetUDP Udp;
	const std::string ip;
	const unsigned short port;
};
using UDPSocket = UDPSocketTeensy;

class TCPSocketTeensy : public winston::hal::Socket, winston::Shared_Ptr<TCPSocketTeensy>
{
public:
	using winston::Shared_Ptr<TCPSocketTeensy>::Shared;
	using winston::Shared_Ptr<TCPSocketTeensy>::make;

	TCPSocketTeensy(const std::string ip, const unsigned short port);
	const winston::Result send(const std::vector<unsigned char> data);
	const winston::Result recv(std::vector<unsigned char>& data);
private:

	EthernetClient Tcp;
	const std::string ip;
	const unsigned short port;
};
using TCPSocket = TCPSocketTeensy;
#endif

#ifdef WINSTON_HAL_USE_SPI
class Arduino_SPIDevice : public winston::hal::SPIDevice<unsigned char>, public winston::Shared_Ptr<Arduino_SPIDevice>
{
public:
	Arduino_SPIDevice(const Pin chipSelect, const unsigned int speed, SPIDataOrder order = SPIDataOrder::MSBFirst, SPIMode mode = SPIMode::SPI_0, const Pin clock = 0, const Pin mosi = 0, const Pin miso = 0);

	using winston::Shared_Ptr<Arduino_SPIDevice>::Shared;
	using winston::Shared_Ptr<Arduino_SPIDevice>::make;

	const winston::Result init();
	const winston::Result send(const std::vector<DataType> data);

	static constexpr uint8_t BitOrder(const SPIDataOrder order);
	static constexpr uint8_t DataMode(const SPIMode mode);

private:
	SPISettings spiSettings;
};
using SignalInterfaceDevice = Arduino_SPIDevice;
#endif

#ifdef WINSTON_HAL_USE_GPIO
class Arduino_GPIOOutputPin : public winston::GPIODigitalPinOutputDevice, public winston::Shared_Ptr<Arduino_GPIOOutputPin>
{
public:
    Arduino_GPIOOutputPin(const Pin pin, const State initial = State::Low);
    void set(const State value);

    using winston::Shared_Ptr<Arduino_GPIOOutputPin>::Shared;
    using winston::Shared_Ptr<Arduino_GPIOOutputPin>::make;
};
#endif
/*
#ifdef WINSTON_HAL_USE_STORAGE
#include <SD.h>
//extern SdFat SD;
class StorageArduino: public winston::hal::StorageInterface, winston::Shared_Ptr<StorageArduino>
{
public:
	StorageArduino(const std::string filename, const size_t maxSize = 0);

	const winston::Result init();
	const winston::Result readVector(const size_t address, std::vector<unsigned char>& content, const size_t length = 1);
	const winston::Result readString(const size_t address, std::string& content, const size_t length = 1);
	const winston::Result read(const size_t address, unsigned char& content);
	const winston::Result writeVector(const size_t address, const std::vector<unsigned char>& content, const size_t length = 0);
	const winston::Result writeString(const size_t address, const std::string& content, const size_t length = 0);
	const winston::Result write(const size_t address, const unsigned char content);
	const winston::Result sync();

	using Shared_Ptr<StorageArduino>::Shared;
	using Shared_Ptr<StorageArduino>::make;
private:
	std::string filename;
#ifdef WINSTON_WITH_SDFAT
	File file;
#endif
};
using Storage = StorageArduino;
#endif
*/

#include "winston-hal-arduino.h"

/*
class DisplayArduino : public winston::TaskConfirm::Display, public winston::Shared_Ptr<DisplayArduino>
{
public:
	DisplayArduino();
	virtual ~DisplayArduino() = default;
	virtual const winston::Result send(const std::vector<DataType> data);
	using winston::Shared_Ptr<DisplayArduino>::Shared;
	using winston::Shared_Ptr<DisplayArduino>::make;
};
using Display = DisplayArduino;*/
