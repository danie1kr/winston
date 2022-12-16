#pragma once

#include <Arduino.h>

#include "Winston.h"
#include "Signal.h"
#include "HAL.h"
#include "Log.h"

#define WINSTON_WITH_HTTP
#define WINSTON_WITH_TEENSYDEBUG
#define WINSTON_WITH_SDFAT

//#define WINSTON_WITH_QNETHERNET

#ifdef WINSTON_WITH_QNETHERNET
#define USE_NATIVE_ETHERNET         false
#define USE_QN_ETHERNET             true
#include <QNEthernet.h>
using namespace qindesign::network;
#else
#define USE_NATIVE_ETHERNET         true
#define USE_QN_ETHERNET             false
#include <NativeEthernet.h>
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
constexpr const char* operator "" _s(const char* in, size_t len)
{
    return in;
}
#endif

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

class Arduino_GPIOOutputPin : public winston::GPIODigitalPinOutputDevice, public winston::Shared_Ptr<Arduino_GPIOOutputPin>
{
public:
    Arduino_GPIOOutputPin(const Pin pin, const State initial = State::Low);
    void set(const State value);

    using winston::Shared_Ptr<Arduino_GPIOOutputPin>::Shared;
    using winston::Shared_Ptr<Arduino_GPIOOutputPin>::make;
};

#ifdef WINSTON_WITH_SDFAT
//#define SDFAT_FILE_TYPE 2 //exfat only
#include <SD.h>
#endif
class StorageArduino: public winston::hal::StorageInterface, winston::Shared_Ptr<StorageArduino>
{
public:
	StorageArduino(const std::string filename, const size_t maxSize = 0);

	const winston::Result init();
	const winston::Result read(const size_t address, std::vector<unsigned char>& content, const size_t length = 1);
	const winston::Result read(const size_t address, std::string& content, const size_t length = 1);
	const winston::Result write(const size_t address, unsigned char content);
	const winston::Result write(const size_t address, std::vector<unsigned char>& content, const size_t length = 0);
	const winston::Result write(const size_t address, std::string& content, const size_t length = 0);
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

/*
#ifdef WINSTON_WITH_WEBSOCKET
class WebServerTeensy : public winston::WebServer<WebsocketsClient>
{
public:
	using Client = WebsocketsClient; 
    using HTTPClient = EthernetClient;

    class HTTPConnectionTeensy : public winston::WebServer<WebsocketsClient>::HTTPConnection
    {
    public:
        HTTPConnectionTeensy(HTTPClient& connection);
        bool status(const unsigned int HTTPStatus);
        bool header(const std::string& key, const std::string& value);
        bool body(const std::string& content);
        bool header(const __FlashStringHelper* key, const __FlashStringHelper* value);
        bool body(const __FlashStringHelper* content);
    private:
        HTTPClient& connection;
        unsigned char guard;
        
        enum class State : unsigned char
        {
            NEW = 0,
            STATUS = 0b1,
            HEADER = 0b10,
            BODY = 0b100
        };

        size_t bufferPopulated;
        unsigned char buffer[512];
    };
    using HTTPConnection = HTTPConnectionTeensy;
    using OnHTTP = std::function<void(HTTPConnection& client, const std::string& resource)>;

	WebServerTeensy();
	virtual ~WebServerTeensy() = default;
	virtual void init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port);
	virtual void send(Client& connection, const std::string& data);
	virtual void step();
	virtual void shutdown();
	virtual Client getClient(unsigned int clientId);
	virtual unsigned int getClientId(Client client);
	virtual void newConnection(Client client);
	virtual void disconnect(Client client);
	virtual size_t maxMessageSize();
private:
	void advanceConnectionIterator();
	Connections::iterator it;
	WebsocketsServer server;
    EthernetServer httpServer;
    OnHTTP onHTTP;
};
using WebServer = WebServerTeensy;
#endif*/