#pragma once

//#define WINSTON_TEENSY_QNETHERNET

#define WEBSOCKETS_USE_ETHERNET     true
#ifdef WINSTON_TEENSY_QNETHERNET
#define USE_QN_ETHERNET				true
#else
#define USE_NATIVE_ETHERNET         true
#endif
#include <WebSockets2_Generic.h>
using namespace websockets2_generic;

#include "Signal.h"
#include "HAL.h"

#ifdef WINSTON_TEENSY_QNETHERNET
#include <QNEthernet.h>
using namespace qindesign::network;
#else
#include <NativeEthernet.h>
#endif

#include <SPI.h>


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
using UDPSocket = UDPSocketTeensy;

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

#include "../libwinston/WebServer.h"

class WebServerTeensy : public winston::WebServerProto<WebsocketsClient>
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
	virtual void newConnection(Client client);
	virtual void disconnect(Client client);
	virtual size_t maxMessageSize();
private:
	void advanceConnectionIterator();
	Connections::iterator it;
	WebsocketsServer server;
};
using WebServer = WebServerTeensy;

#include "HAL.h"
#include "Util.h"

#include <iostream>
#include <fstream>
#include <algorithm>


#include <Arduino.h>

#include <SD.h>
#include <SdFatConfig.h>
#include <SdFat.h>
#include <TeensyDebug.h>

//#include "winston-hal-teensy.h"

WebServerTeensy::WebServerTeensy() : winston::WebServerProto<Client>()
{

}

void WebServerTeensy::init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port)
{
    this->onHTTP = onHTTP;
    this->onMessage = onMessage;
    this->it = this->connections.begin();
    this->server.listen(port);
}

void WebServerTeensy::send(Client& connection, const std::string& data)
{
    connection.send(data.c_str());
}

void WebServerTeensy::step()
{
    if (server.poll())
    {
        // check for new client
        Client connection = server.accept();
        if (connection.available())
        {
            this->newConnection(connection);
            connection.onMessage([=](WebsocketsMessage message)
                {
                    this->onMessage(connection, std::string(message.data().c_str()));
                });

            connection.onEvent([=](WebsocketsEvent event, String data)
                {
                    if (event == WebsocketsEvent::ConnectionClosed)
                    {
                        this->disconnect(connection);
                    }
                });
        }

        if (this->connections.size() == 0)
            return;
    }
    this->advanceConnectionIterator();
    auto& client = this->it->second;
    client.poll();

    /* ask one client for nonblocking updates
    if (client.available())
    {
        Serial.println("Client connected");

        // Read message from client and log it.
        WebsocketsMessage msg = client.readBlocking();
        if(msg.isComplete())
            this->onMessage(client, msg.data());
    }*/
}

WebServerTeensy::Client WebServerTeensy::getClient(unsigned int clientId)
{
    return this->connections[clientId];
}

unsigned int WebServerTeensy::getClientId(Client client)
{
    return 0;
}

void WebServerTeensy::advanceConnectionIterator()
{
    ++this->it;
    if (this->it == this->connections.end())
        this->it = this->connections.begin();
}

void WebServerTeensy::newConnection(Client client)
{
    unsigned int id = this->newClientId();
    this->connections.insert({ id, client });
    if (this->it == this->connections.end())
        this->it = this->connections.begin();
}

void WebServerTeensy::disconnect(Client client)
{
    for (auto deleterator = this->connections.begin(); deleterator != this->connections.end();)
    {
        auto& client = deleterator->second;
        if (!client.available())
        {
            this->connections.erase(++deleterator);
            if (this->it == deleterator)
                this->advanceConnectionIterator();
        }
        else
            ++deleterator;
    }
}

void WebServerTeensy::shutdown()
{
}

size_t WebServerTeensy::maxMessageSize()
{
    return 1024;
}

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
    } return 0;
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

void teensyMAC(uint8_t* mac) { // there are 2 MAC addresses each 48bit 
    uint32_t m1 = HW_OCOTP_MAC1;
    uint32_t m2 = HW_OCOTP_MAC0;
    mac[0] = m1 >> 8;
    mac[1] = m1 >> 0;
    mac[2] = m2 >> 24;
    mac[3] = m2 >> 16;
    mac[4] = m2 >> 8;
    mac[5] = m2 >> 0;
}

namespace winston
{
    namespace hal {
        void init()
        {
            Serial.begin(115200);
            while (!Serial && millis() < 4000) {
                // Wait for Serial to initialize
            }
            //stdPrint = &Serial;  // Make printf work

            if (!sd.begin(BUILTIN_SDCARD)) {
                Serial.println("SD initialization failed!");
                return;
            }
            sd.chdir();
            ensureStorageFile();

#ifdef WINSTON_TEENSY_QNETHERNET
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
#else
            uint8_t mac[6];
            teensyMAC(mac);
            if (!Ethernet.begin(mac)) {
                printf("Failed to start Ethernet\n");
                return;
            }
#endif
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