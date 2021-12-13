#include "../libwinston/HAL.h"
#include "../libwinston/Util.h"

#include <iostream>
#include <fstream>

#include "winston-hal-teensy.h"
/*
WebServerTeensy::WebServerTeensy() : winston::WebServer<Client>()
{

}

void WebServerTeensy::init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port)
{
    this->onHTTP = onHTTP;
    this->onMessage = onMessage;
    this->server.listen(port);
}

void WebServerTeensy::send(WebsocketsClient& connection, const std::string& data)
{
    connection.send(data);;
}

void WebServerTeensy::step()
{
    // check for new client
    WebsocketsClient newClient = server.accept();
    if(newClient)
        this->connections.insert({ this->newClientId(), newClient });

    // ask one client for nonblocking updates
    this->server.poll_one();
}

WebsocketsClient WebServerTeensy::getClient(unsigned int clientId)
{
    return this->connections[clientId];
}
unsigned int WebServerTeensy::getClientId(WebsocketsClient client)
{
    auto result = std::find_if(
        this->connections.begin(),
        this->connections.end(),
        [client](const auto& it) {return it.second.lock() == client.lock(); });

    if (result != this->connections.end())
        this->connections.erase(result->first);
    return 0;
}

void WebServerTeensy::on_http(ConnectionWSPP hdl)
{
    auto con = this->server.get_con_from_hdl(hdl);
    const auto response = this->onHTTP(hdl, con->get_resource());

    for (auto const& kv : response.headers)
        con->append_header(kv.first, kv.second);
    con->set_status(websocketpp::http::status_code::value(response.status));
    con->set_body(response.body);
}

void WebServerTeensy::on_msg(ConnectionWSPP hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg)
{
    this->onMessage(hdl, msg->get_payload());
}

void WebServerTeensy::on_open(ConnectionWSPP hdl) {
    this->connections.insert({ this->newClientId(), hdl });
}

void WebServerTeensy::on_close(ConnectionWSPP hdl) {
    auto id = this->getClientId(hdl);

    if (id)
        this->connections.erase(id);
}

void WebServerTeensy::shutdown()
{
    this->server.stop();
}

size_t WebServerTeensy::maxMessageSize()
{
    return this->server.get_max_message_size();
}
*/
static const std::string constWinstonStoragePath = "winston.storage";
static std::string winstonStoragePath = constWinstonStoragePath;
static const auto winstonStorageSize = 128 * 1024;
mio::mmap_sink winstonStorage;

int handle_error(const std::error_code& error)
{
    winston::error(error.message());
    return error.value();
}

void setStoragePath(const std::string prefix)
{
    winstonStoragePath = std::string(prefix).append(".").append(constWinstonStoragePath);
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

UDPSocketLWIP::UDPSocketLWIP(const std::string ip, const unsigned short port) : winston::hal::UDPSocket(ip, port), ip(ip), port(port)
{
    Udp.begin(port);
}

const winston::Result UDPSocketLWIP::send(const std::vector<unsigned char> data)
{
    auto sz = (int)(data.size() * sizeof(unsigned char));
    Udp.beginPacket(this->ip, this->port);
    Udp.write(reinterpret_cast<const char*>(data.data()), sz);
    Udp.endPacket();

    return winston::Result::OK;
}

namespace winston
{
    namespace hal {
        void init()
        {
            // mac from teensy id
            teensyMAC(mac);
            // Connect to ethernet.
            if (Ethernet.begin(mac))
            {
                Serial.println("Ethernet connected");
            }
            else {
                Serial.println("Ethernet failed");
            }

            { WSADATA wsaData; WSAStartup(MAKEWORD(1, 1), &wsaData); }

            ensureStorageFile();
            std::error_code error;
            winstonStorage = mio::make_mmap_sink(winstonStoragePath, 0, mio::map_entire_file, error);
            if (error) { handle_error(error); }
        }

        void text(const std::string& error)
        {
            std::cout << error << std::endl;
        }

        void fatal(const std::string text)
        {
            throw std::exception(text.c_str());
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