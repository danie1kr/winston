#pragma once

#include "Signal.h"
#include "HAL.h"

#include <QNEthernet.h>
#include <SPI.h>

using namespace qindesign::network;

class UDPSocketTeensy : public winston::hal::UDPSocket, winston::Shared_Ptr<UDPSocketTeensy>
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

class Arduino_SPIDevice : public winston::hal::SPIDevice<unsigned int, 12>, public winston::Shared_Ptr<Arduino_SPIDevice>
{
public:
	Arduino_SPIDevice(const Pin chipSelect, const unsigned int speed, const Pin xlat, SPIDataOrder order = SPIDataOrder::MSBFirst, SPIMode mode = SPIMode::SPI_0, const Pin clock = 0, const Pin mosi = 0, const Pin miso = 0);

	using winston::Shared_Ptr<Arduino_SPIDevice>::Shared;
	using winston::Shared_Ptr<Arduino_SPIDevice>::make;

	const winston::Result init();
	const winston::Result send(const std::span<DataType> data);

	static constexpr uint8_t BitOrder(const SPIDataOrder order);
	static constexpr uint8_t DataMode(const SPIMode mode);

private:
	const Pin xlat;
	SPISettings spiSettings;
};

using SignalSPIDevice = Arduino_SPIDevice;

/*
#include "../libwinston/WebServer.h"

#define WEBSOCKETS_USE_ETHERNET     true
#define USE_NATIVE_ETHERNET         true

#include <WebSockets2_Generic.h>
#include <TeensyID.h>

using namespace websockets2_generic;
class WebServerTeensy : public winston::WebServer<WebsocketsClient>
{
public:
	using Client = WebsocketsClient;

	WebServerTeensy();
	virtual ~WebServerTeensy() = default;
	virtual void init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port);
	virtual void send(Client& connection, const std::string& data);
	virtual void step();
	virtual void shutdown();
	virtual Client getClient(unsigned int clientId);
	virtual unsigned int getClientId(Client client);
	virtual size_t maxMessageSize();
private:

	byte mac[6];
	Connections::iterator it;
	WebsocketsServer server;
};*/