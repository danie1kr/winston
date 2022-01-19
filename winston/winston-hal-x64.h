#pragma once

#include <WinSock2.h>
#include "../libwinston/Signal.h"
#include "../libwinston/HAL.h"
#include "FT232_SPIDevice.h"

const char* operator "" _s(const char* in, size_t len);

class UDPSocketWinSock : public winston::hal::UDPSocket, winston::Shared_Ptr<UDPSocketWinSock>
{
public:
	using winston::Shared_Ptr<UDPSocketWinSock>::Shared;
	using winston::Shared_Ptr<UDPSocketWinSock>::make;

	UDPSocketWinSock(const std::string ip, const unsigned short port);
	const winston::Result send(const std::vector<unsigned char> data);
	const winston::Result recv(std::vector<unsigned char>& data);
private:

	const winston::Result connect();

	SOCKET udpSocket;
	SOCKADDR_IN addr;
};

using UDPSocket = UDPSocketWinSock;

#include "../libwinston/WebServer.h"

#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define _WEBSOCKETPP_CPP11_STL_
#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using ConnectionWSPP = websocketpp::connection_hdl;

class WebServerWSPP : public winston::WebServer<ConnectionWSPP>
{
public:
	using Client = ConnectionWSPP;
	using HTTPClient = websocketpp::server<websocketpp::config::asio>::connection_ptr;

	class HTTPConnectionWSPP : public HTTPConnection
	{
	public:
		HTTPConnectionWSPP(HTTPClient& connection);
		bool status(const unsigned int HTTPStatus);
		bool header(const std::string& key, const std::string& value);
		bool body(const std::string& content);
	private:
		HTTPClient& connection;
		std::string fullBody;
	};

	WebServerWSPP();
	virtual ~WebServerWSPP() = default;
	virtual void init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port);
	virtual void send(Client& connection, const std::string &data);
	virtual void step();
	virtual void shutdown();
	virtual Client& getClient(const unsigned int clientId);
	virtual const unsigned int getClientId(Client client);
	virtual void disconnect(Client client);
	virtual const size_t maxMessageSize();
private:

	void on_http(Client hdl);
	void on_msg(Client hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg);
	void on_open(Client hdl);
	void on_close(Client hdl);

	websocketpp::server<websocketpp::config::asio> server;
};
using WebServer = WebServerWSPP;

using SignalSPIDevice = FT232_SPIDevice;