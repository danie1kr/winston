#include "../libwinston/HAL.h"
#include "../libwinston/Util.h"
#include "../libwinston/Log.h"

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <iostream>
#include <fstream>

#include "mio.hpp"
#include "winston-hal-x64.h"

#pragma comment(lib, "ws2_32.lib")

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

WebServerWSPP::WebServerWSPP() : winston::WebServer<ConnectionWSPP>()
{

}

void WebServerWSPP::init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port)
{
    this->onHTTP = onHTTP;
    this->onMessage = onMessage;
    this->server.init_asio();

    this->server.set_http_handler(websocketpp::lib::bind(&WebServerWSPP::on_http, this, websocketpp::lib::placeholders::_1));
    this->server.set_message_handler(websocketpp::lib::bind(&WebServerWSPP::on_msg, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
    this->server.set_open_handler(websocketpp::lib::bind(&WebServerWSPP::on_open, this, websocketpp::lib::placeholders::_1));
    this->server.set_close_handler(websocketpp::lib::bind(&WebServerWSPP::on_close, this, websocketpp::lib::placeholders::_1));

    this->server.listen(port);

    this->server.start_accept();
}

void WebServerWSPP::send(Client& connection, const std::string &data)
{
    this->server.send(connection, data, websocketpp::frame::opcode::text);
}

void WebServerWSPP::step()
{
    this->server.poll_one();
}

WebServerWSPP::Client WebServerWSPP::getClient(unsigned int clientId)
{
    return this->connections[clientId];
}
unsigned int WebServerWSPP::getClientId(Client client)
{
    auto result = std::find_if(
        this->connections.begin(),
        this->connections.end(),
        [client](const auto& it) {return it.second.lock() == client.lock(); });

    if (result != this->connections.end())
        this->connections.erase(result->first);
    return 0;
}

void WebServerWSPP::disconnect(Client client)
{
    auto id = this->getClientId(client);

    if (id)
        this->connections.erase(id);
}

void WebServerWSPP::on_http(ConnectionWSPP hdl)
{
    auto con = this->server.get_con_from_hdl(hdl);
    const auto response = this->onHTTP(hdl, con->get_resource());

    for (auto const& kv: response.headers)
        con->append_header(kv.first, kv.second);
    con->set_status(websocketpp::http::status_code::value(response.status));
    con->set_body(response.body);
}

void WebServerWSPP::on_msg(ConnectionWSPP hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg)
{
    this->onMessage(hdl, msg->get_payload());
}

void WebServerWSPP::on_open(ConnectionWSPP hdl) {
    this->newConnection(hdl);
}

void WebServerWSPP::on_close(ConnectionWSPP hdl) {
    this->disconnect(hdl);
}

void WebServerWSPP::shutdown()
{
    this->server.stop();
}

size_t WebServerWSPP::maxMessageSize()
{
    return this->server.get_max_message_size();
}

static const std::string constWinstonStoragePath = "winston.storage";
static std::string winstonStoragePath = constWinstonStoragePath;
static const auto winstonStorageSize = 128 * 1024;
mio::mmap_sink winstonStorage;

int handle_error(const std::error_code& error)
{
    winston::error(error.message());
    return error.value();
}

void ensureStorageFile()
{
    std::ifstream testIfExists(winstonStoragePath);
    if (!testIfExists.good())
    {
        std::ofstream file(winstonStoragePath);
        std::string s(winstonStorageSize, 0);
        file << s;
    }
}

UDPSocketWinSock::UDPSocketWinSock(const std::string ip, const unsigned short port) : winston::hal::UDPSocket(ip, port)
{
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &this->addr.sin_addr.s_addr); 
    
    u_long nMode = 1; // 1: NON-BLOCKING
    ioctlsocket(this->udpSocket, FIONBIO, &nMode);

    this->connect();
}

const winston::Result UDPSocketWinSock::connect()
{
    closesocket(this->udpSocket);
    this->udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    this->state = State::Connecting;
    return winston::Result::OK;
}

const winston::Result UDPSocketWinSock::send(const std::vector<unsigned char> data)
{
    auto sz = (int)(data.size() * sizeof(unsigned char));

    auto result = sendto(this->udpSocket, reinterpret_cast<const char*>(data.data()), sz, 0, (SOCKADDR*)&this->addr, (int)sizeof(SOCKADDR_IN));

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

const winston::Result UDPSocketWinSock::recv(std::vector<unsigned char>& data)
{
    sockaddr_in from;
    int size = (int)sizeof(from);
    data.resize(1000);
    
    fd_set fds;
    struct timeval tv;

    // Set up the file descriptor set.
    FD_ZERO(&fds);
    FD_SET(this->udpSocket, &fds);

    // Set up the struct timeval for the timeout.
    tv.tv_sec = 0;
    tv.tv_usec = 50;

    // Wait until timeout or data received.
    if (select(0, &fds, NULL, NULL, &tv) > 0) {
        int ret = recvfrom(this->udpSocket, reinterpret_cast<char*>(data.data()), data.size(), 0, reinterpret_cast<SOCKADDR*>(&from), &size);
        if (ret < 0)
            return winston::Result::ReceiveFailed;

        // make the buffer zero terminated
        data.resize(ret);
    }
    else
        data.resize(0);
    return winston::Result::OK;
}

namespace winston
{
    namespace hal {
        void init()
        {
            { WSADATA wsaData; WSAStartup(MAKEWORD(1, 1), &wsaData); }

            ensureStorageFile();
            std::error_code error;
            winstonStorage = mio::make_mmap_sink(winstonStoragePath, 0, mio::map_entire_file, error);
            if (error) { handle_error(error); }
        }

        void text(const std::string& text)
        {
            std::cout << text << std::endl;
        }

        void error(const std::string& error)
        {
            logger.err(error);
        }

        void fatal(const std::string reason)
        {
            logger.log(reason, Logger::Entry::Level::Fatal);
            throw std::exception(reason.c_str());
            exit(-1);
        }

        void delay(const unsigned int ms)
        {
            Sleep(ms);
        }

        unsigned long long now()
        {
            return GetTickCount64();
        }

        void storageSetFilename(std::string filename)
        {
            winstonStoragePath = std::string(filename).append(".").append(constWinstonStoragePath);
        }

        const uint8_t storageRead(const size_t address)
        {
            if (winstonStorage.size() > address)
                return winstonStorage[address];
            else
                return 0;
        }

        void storageWrite(const size_t address, const uint8_t data)
        {
            if (winstonStorage.size() > address)
            {
                winstonStorage[address] = data;
            }
            else
                error("storage too small for full layout");
        }

        bool storageCommit()
        {
            std::error_code error;
            winstonStorage.sync(error);
            if (error)
            {
                handle_error(error);
                return false;
            }
            return true;
        }

        bool send(const std::string& ip, const unsigned short& port, std::vector<unsigned char>& data)
        {
            return true;
        }
    }
}