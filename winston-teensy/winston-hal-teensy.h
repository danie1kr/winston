#pragma once

#include "../libwinston/Signal.h"
#include "../libwinston/HAL.h"

#include <QNEthernet.h>

class UDPSocketLWIP : public winston::hal::UDPSocket, winston::Shared_Ptr<UDPSocketLWIP>
{
public:
	using winston::Shared_Ptr<UDPSocketLWIP>::Shared;
	using winston::Shared_Ptr<UDPSocketLWIP>::make;

	UDPSocketLWIP(const std::string ip, const unsigned short port);
	const winston::Result send(const std::vector<unsigned char> data);
private:

	EthernetUDP Udp;
	const std::string ip;
	const unsigned short port;
};

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