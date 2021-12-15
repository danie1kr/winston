#include "HAL.h"
#include "Util.h"

#include <iostream>
#include <fstream>

#include <SD.h>
#include <sdios.h>
#include <SdFatConfig.h>
#include <SdFat.h>

#include <TeensyDebug.h>

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

static SdFat sd;
static SdFile winstonStorage;

void ensureStorageFile()
{
    if (!sd.exists(winstonStoragePath.c_str()))
    {
        auto file = sd.open(winstonStoragePath.c_str(), O_READ | O_WRITE | O_CREAT);
        std::string s(winstonStorageSize, 0);
        file.write(s.c_str(), s.length());
        file.flush();
        file.close();
    }
}

UDPSocketTeensy::UDPSocketTeensy(const std::string ip, const unsigned short port) : winston::hal::UDPSocket(ip, port), ip(ip), port(port)
{
    Udp.begin(port);
}

const winston::Result UDPSocketTeensy::send(const std::vector<unsigned char> data)
{
    auto sz = (int)(data.size() * sizeof(unsigned char));
    Udp.beginPacket(this->ip.c_str(), this->port);
    Udp.write(reinterpret_cast<const char*>(data.data()), sz);
    Udp.endPacket();

    return winston::Result::OK;
}

const winston::Result UDPSocketTeensy::recv(std::vector<unsigned char>& data)
{
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        data.resize(packetSize);
        Udp.read(data.data(), data.size());
    }
    return winston::Result::OK;
}

Arduino_SPIDevice::Arduino_SPIDevice(const Pin chipSelect, const unsigned int speed, const Pin xlat, SPIDataOrder order, SPIMode mode, const Pin clock, const Pin mosi, const Pin miso)
    : SPIDevice<unsigned int, 12>(chipSelect, speed, order, mode, clock, mosi, miso), xlat(xlat), spiSettings(speed, Arduino_SPIDevice::BitOrder(order), Arduino_SPIDevice::DataMode(mode))
{
}

constexpr uint8_t Arduino_SPIDevice::BitOrder(const SPIDataOrder order)
{
    return order == SPIDataOrder::LSBFirst ? LSBFIRST : MSBFIRST;
}

constexpr uint8_t Arduino_SPIDevice::DataMode(const SPIMode mode)
{
    switch (mode)
    {
    default:
    case SPIMode::SPI_0: return SPI_MODE0;
    case SPIMode::SPI_1: return SPI_MODE1;
    case SPIMode::SPI_2: return SPI_MODE2;
    case SPIMode::SPI_3: return SPI_MODE3;
    }
}

const winston::Result Arduino_SPIDevice::init()
{
    SPI.begin();
    return winston::Result::OK;
}
const winston::Result Arduino_SPIDevice::send(const std::span<DataType> data)
{
    if (this->skip)
        return winston::Result::OK;

    digitalWrite(this->xlat, 0);
    SPI.beginTransaction(this->spiSettings);
    SPI.transfer((unsigned char*)&data.front(), data.size() * sizeof(DataType));
    SPI.endTransaction();
    digitalWrite(this->xlat, 1);
    digitalWrite(this->xlat, 0);

    return winston::Result::OK;
}

constexpr uint32_t DHCPTimeOut = 10000;  // 10 seconds
IPAddress staticIP{ 0, 0, 0, 0 };//{192, 168, 1, 101};
IPAddress subnetMask{ 255, 255, 255, 0 };
IPAddress gateway{ 192, 168, 1, 1 };

namespace winston
{
    namespace hal {
        void init()
        {
            Serial.begin(115200);
            while (!Serial && millis() < 4000) {
                // Wait for Serial to initialize
            }
            stdPrint = &Serial;  // Make printf work

            if (!sd.begin(BUILTIN_SDCARD)) {
                Serial.println("SD initialization failed!");
                return;
            }
            sd.chdir();

            // Listen for link changes, for demonstration
            Ethernet.onLinkState([](bool state) {
                printf("Ethernet: Link %s\n", state ? "ON" : "OFF");
                });

            // Listen for address changes
            Ethernet.onAddressChanged([]() {
                IPAddress ip = Ethernet.localIP();
                bool hasIP = !(ip == INADDR_NONE);  // IPAddress has no operator!=()
                if (hasIP) {
                    printf("Ethernet: Address changed:\n");

                    IPAddress ip = Ethernet.localIP();
                    printf("    Local IP = %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
                    ip = Ethernet.subnetMask();
                    printf("    Subnet   = %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
                    ip = Ethernet.gatewayIP();
                    printf("    Gateway  = %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
                    ip = Ethernet.dnsServerIP();
                    if (!(ip == INADDR_NONE)) {  // May happen with static IP
                        printf("    DNS      = %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
                    }
                }
                else {
                    printf("Ethernet: Address changed: No IP address\n");
                }
                });


            if (staticIP == INADDR_NONE) {
                printf("Starting Ethernet with DHCP...\n");
                if (!Ethernet.begin()) {
                    printf("Failed to start Ethernet\n");
                    return;
                }
                if (!Ethernet.waitForLocalIP(DHCPTimeOut)) {
                    printf("Failed to get IP address from DHCP\n");
                    // We may still get an address later, after the timeout,
                    // so continue instead of returning
                }
            }
            else {
                printf("Starting Ethernet with static IP...\n");
                Ethernet.begin(staticIP, subnetMask, gateway);
            }

            ensureStorageFile();
        }

        void text(const std::string& error)
        {
            Serial.println(error.c_str());
        }

        void fatal(const std::string err)
        {
            text(err);
            exit(-1);
        }

        void delay(const unsigned int ms)
        {
            delay(ms);
        }

        unsigned long long now()
        {
            return millis();
        }

        void storageSetFilename(std::string filename)
        {
            winstonStoragePath = std::string(filename).append(".").append(constWinstonStoragePath);
        }

        const uint8_t storageRead(const size_t address)
        {
            if (winstonStorage.size() > address)
            {
                winstonStorage.seek(address);
                return winstonStorage.read();
            }
            else
                return 0;
        }

        void storageWrite(const size_t address, const uint8_t data)
        {
            if (winstonStorage.size() > address)
            {
                winstonStorage.seek(address);
                winstonStorage.write(data);
                //winstonStorage[address] = data;
            }
        }

        bool storageCommit()
        {
            winstonStorage.sync();
            return true;
        }
    }
}