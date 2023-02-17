#pragma once

#include <WinSock2.h>
#include "../libwinston/Signal.h"
#include "../libwinston/HAL.h"

#include "external/mio.hpp"

#include "FT232_Device.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
template<winston::hal::Socket::Type SocketType>
class SocketWinSock : public winston::hal::Socket, winston::Shared_Ptr<SocketWinSock<SocketType>>
{
public:
	using winston::Shared_Ptr<SocketWinSock<SocketType>>::Shared;
	using winston::Shared_Ptr<SocketWinSock<SocketType>>::make;

	SocketWinSock(const std::string ip, const unsigned short port) : winston::hal::Socket(ip, port)
	{
		this->addr.sin_family = AF_INET;
		this->addr.sin_port = htons(port);
		inet_pton(AF_INET, ip.c_str(), &this->addr.sin_addr.s_addr);

		u_long nMode = 1; // 1: NON-BLOCKING
		ioctlsocket(this->socket, FIONBIO, &nMode);

		this->connect();
	}

	const winston::Result send(const std::vector<unsigned char> data)
	{
		auto sz = (int)(data.size() * sizeof(unsigned char));

		auto result = sendto(this->socket, reinterpret_cast<const char*>(data.data()), sz, 0, (SOCKADDR*)&this->addr, (int)sizeof(SOCKADDR_IN));

		if (result == sz)
		{
			this->state = State::Connected;
			return winston::Result::OK;
		}
		else
		{
			this->state = State::NotConnected;
			if (result == -1)
				this->connect();

			return winston::Result::SendFailed;
		}
	}

	const winston::Result recv(std::vector<unsigned char>& data)
	{
		sockaddr_in from;
		int size = (int)sizeof(from);
		data.resize(1000);

		fd_set fds;
		struct timeval tv;

		// Set up the file descriptor set.
		FD_ZERO(&fds);
		FD_SET(this->socket, &fds);

		// Set up the struct timeval for the timeout.
		tv.tv_sec = 0;
		tv.tv_usec = 0; // anything > 0 is at least 14ms

		// Wait until timeout or data received.
		if (select(0, &fds, NULL, NULL, &tv) > 0) {
			int ret = recvfrom(this->socket, reinterpret_cast<char*>(data.data()), (int)data.size(), 0, reinterpret_cast<SOCKADDR*>(&from), &size);
			if (ret < 0)
				return winston::Result::ReceiveFailed;

			// make the buffer zero terminated
			data.resize(ret);
		}
		else
			data.resize(0);
		return winston::Result::OK;
	}
private:

	const winston::Result connect()
	{
		if (!winston::runtimeNetwork())
			return winston::Result::NotInitialized;

		closesocket(this->socket);
		this->socket = ::socket(AF_INET, SocketType == winston::hal::Socket::Type::UDP ? SOCK_DGRAM : SOCK_STREAM, 0);
		this->state = State::Connecting;
		return winston::Result::OK;
	}

	SOCKET socket;
	SOCKADDR_IN addr;
};
using UDPSocket = SocketWinSock<winston::hal::Socket::Type::UDP>;
using TCPSocket = SocketWinSock<winston::hal::Socket::Type::TCP>;

class SerialDeviceWin : public winston::hal::SerialDevice, winston::Shared_Ptr<SerialDeviceWin>
{
public:
	SerialDeviceWin();
	bool init(const size_t portNumber, const size_t bauds = 115200,
		const SerialDataBits databits = SerialDataBits::SERIAL_DATABITS_8,
		const SerialParity parity = SerialParity::SERIAL_PARITY_NONE,
		const SerialStopBits stopbits = SerialStopBits::SERIAL_STOPBITS_1);
	
	const size_t available();
	const DataType read();
	const size_t read(std::vector<DataType>& content, size_t upTo);

	const winston::Result send(const std::vector<DataType> data);
	using Shared_Ptr<SerialDeviceWin>::Shared;
	using Shared_Ptr<SerialDeviceWin>::make;
private:
	HANDLE serialHandle;
	COMMTIMEOUTS    timeouts;
};

class StorageWin : public winston::hal::StorageInterface, winston::Shared_Ptr<StorageWin>
{
public:
	StorageWin(const std::string filename, const size_t maxSize = 0);

	const winston::Result init();
	const winston::Result read(const size_t address, std::vector<unsigned char>& content, const size_t length = 1);
	const winston::Result read(const size_t address, std::string& content, const size_t length = 1);
	const winston::Result write(const size_t address, unsigned char content);
	const winston::Result write(const size_t address, std::vector<unsigned char>& content, const size_t length = 0);
	const winston::Result write(const size_t address, std::string& content, const size_t length = 0);
	const winston::Result sync();

	using Shared_Ptr<StorageWin>::Shared;
	using Shared_Ptr<StorageWin>::make;
private:

	const int handleError(const std::error_code& error) const;

	std::string filename;
	mio::mmap_sink mmap;
};
using Storage = StorageWin;

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
		bool body(const unsigned char* content, size_t length, size_t chunked);
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

class DisplayWin : public winston::TaskConfirm::Display, public winston::Shared_Ptr<DisplayWin>
{
public:
	DisplayWin();
	virtual ~DisplayWin() = default;
	virtual const winston::Result send(const std::vector<DataType> data);
	using winston::Shared_Ptr<DisplayWin>::Shared;
	using winston::Shared_Ptr<DisplayWin>::make;
};
using Display = DisplayWin;