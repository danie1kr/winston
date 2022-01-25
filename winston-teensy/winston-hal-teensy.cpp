#include "winston-hal-teensy.h"

#ifdef WINSTON_TEENSY_FLASHSTRING
namespace winston
{
    std::string build(const __FlashStringHelper* fsh)
    {
        return hal::__FlashStorageStringtoStd(fsh);
    }
};
#endif

#include "HAL.h"
#include "Util.h"

#undef WINSTON_WITH_WEBSOCKET

#ifdef WINSTON_WITH_SDFAT
//#define SDFAT_FILE_TYPE 2 //exfat only
#include <SD.h>
#endif

#ifdef WINSTON_WITH_TEENSYDEBUG
#include "TeensyDebug/TeensyDebug.h"
#endif

#ifdef WINSTON_WITH_WEBSOCKET

WebServerTeensy::HTTPConnectionTeensy::HTTPConnectionTeensy(HTTPClient& connection)
    : connection(connection), guard((unsigned char)HTTPConnectionTeensy::State::NEW), bufferPopulated(0)
{

}

bool WebServerTeensy::HTTPConnectionTeensy::status(const unsigned int HTTPStatus)
{
    if (this->guard & (unsigned char)HTTPConnectionTeensy::State::STATUS)
        return false;
    std::string line(winston::build("HTTP/1.1 ", HTTPStatus, " OK\r\n"));
    this->connection.write(line.c_str());
    Serial.print(line.c_str());
    this->guard |= (unsigned char)HTTPConnectionTeensy::State::STATUS;
    return true;
}

bool WebServerTeensy::HTTPConnectionTeensy::header(const std::string& key, const std::string& value)
{
    if (!(this->guard & (unsigned char)HTTPConnectionTeensy::State::STATUS) || (this->guard & (unsigned char)HTTPConnectionTeensy::State::BODY))
        return false;
    std::string line(winston::build(key, ": ", value, "\r\n"));
    this->connection.write(line.c_str());
    Serial.print(line.c_str());
    this->guard |= (unsigned char)HTTPConnectionTeensy::State::HEADER;
    return true;
}

bool WebServerTeensy::HTTPConnectionTeensy::body(const std::string& content)
{
    if (!(this->guard & (unsigned char)HTTPConnectionTeensy::State::HEADER))
        return false;
    if (!(this->guard & (unsigned char)HTTPConnectionTeensy::State::BODY))
    {
        this->connection.write("\r\n");
        Serial.print("\r\n");
    }
    this->connection.write(content.c_str());
    Serial.print(content.c_str());
    this->guard |= (unsigned char)HTTPConnectionTeensy::State::BODY;
    return true;
}

#ifdef WINSTON_TEENSY_FLASHSTRING
bool WebServerTeensy::HTTPConnectionTeensy::header(const __FlashStringHelper* key, const __FlashStringHelper* value)
{
    if (!(this->guard & (unsigned char)HTTPConnectionTeensy::State::STATUS) || (this->guard & (unsigned char)HTTPConnectionTeensy::State::BODY))
        return false;

    auto target = [=](uint8_t c) { 
        Serial.write(c);
        return connection.write(c); 
    };

    winston::hal::stream(key, target);
    this->connection.write(": ");
    Serial.write(": ");
    winston::hal::stream(value, target);
    this->connection.write("\r\n");
    Serial.write("\r\n");

    this->guard |= (unsigned char)HTTPConnectionTeensy::State::HEADER;
    return true;
}
bool WebServerTeensy::HTTPConnectionTeensy::body(const __FlashStringHelper* content)
{
    if (!(this->guard & (unsigned char)HTTPConnectionTeensy::State::HEADER))
        return false;
    if (!(this->guard & (unsigned char)HTTPConnectionTeensy::State::BODY))
    {
        this->connection.write("\r\n");
        Serial.print("\r\n");
    }
    //*
    auto target = [=](uint8_t c) {
        Serial.write(c);
        return connection.write(c);
    };
    winston::hal::stream(content, target);
    /*/

    auto target = [=](uint8_t c) {
        buffer[bufferPopulated++] = c;
        if (bufferPopulated == 512)
        {
            connection.write(buffer, bufferPopulated);
            Serial.write(buffer, bufferPopulated);
            bufferPopulated = 0;
        }
        return 1;
    };
    winston::hal::stream(content, target);
    if (bufferPopulated > 0)
    {
        connection.write(buffer, bufferPopulated);
        Serial.write(buffer, bufferPopulated);
    }
    */

    this->guard |= (unsigned char)HTTPConnectionTeensy::State::BODY;
    return true;
}
#endif

WebServerTeensy::WebServerTeensy() : winston::WebServer<Client>()
{
}

void WebServerTeensy::init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port)
{
    if (!winston::runtimeNetwork())
        return;

    this->onHTTP = onHTTP;
    this->onMessage = onMessage;
    this->it = this->connections.begin();
    this->server.listen(port + 1);
    this->httpServer.begin(port);
}

void WebServerTeensy::send(Client& connection, const std::string& data)
{
    connection.send(data.c_str());
}

void WebServerTeensy::step()
{
    if (!winston::runtimeNetwork())
        return;

    if (auto httpClient = this->httpServer.available())
    {
        // An http request ends with a blank line.
        bool currentLineIsBlank = true;
        bool firstLine = true;
        std::string line(""), resource("");

        while (httpClient.connected()) {
            if (httpClient.available()) {
                char c = httpClient.read();

                if (firstLine)
                {
                    line += c;
                }

                if (c == '\n' && currentLineIsBlank) {
                    // If we've gotten to the end of the line (received a newline
                    // character) and the line is blank, the http request has ended,
                    // so we can send a reply.
                    HTTPConnectionTeensy connection(httpClient);
                    this->onHTTP(connection, resource);
                    break;
                }
                else if (c == '\n') {
                    // Starting a new line.
                    if (firstLine)
                    {
                        firstLine = false;

                        std::string method("");
                        size_t i = 0;
                        for (; i < line.length() && line[i] != ' '; ++i)
                            method += line[i];
                        if (method.compare("GET"))
                            break;
                        ++i;

                        for (; i < line.length() && line[i] != ' '; ++i)
                            resource += line[i];
                        line.erase();
                    }
                    currentLineIsBlank = true;
                }
                else if (c != '\r') {
                    // Read a character on the current line.
                    currentLineIsBlank = false;
                }
            }
        }
        httpClient.stop();
    }
    if (server.poll())
    {
        // check for new client
        Client connection = server.accept();
        if (connection.available())
        {
            this->newConnection(connection);
            connection.onMessage([=, &connection](WebsocketsMessage message)
                {
                    const auto msg = std::string(message.data().c_str());
                    this->onMessage(connection, msg);
                });

            connection.onEvent([=](WebsocketsEvent event, String data)
                {
                    if (event == WebsocketsEvent::ConnectionClosed)
                    {
                        this->disconnect(connection);
                    }
                });
        }


    }
    if (this->connections.size() == 0)
        return;
    this->advanceConnectionIterator();
    auto& client = this->it->second;
    client.poll();
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
    return 2048;
}
#endif

static const std::string constWinstonStoragePath = "winston.storage";
static std::string winstonStoragePath = constWinstonStoragePath;
static const auto winstonStorageSize = 128 * 1024;

#ifdef WINSTON_WITH_SDFAT
static File winstonStorage;
#endif

void ensureStorageFile()
{
#ifdef WINSTON_WITH_SDFAT
    if (!winston::runtimePersistence())
        return;

    if (!SD.exists(winstonStoragePath.c_str()))
    {
        auto file = SD.open(winstonStoragePath.c_str(), FILE_WRITE);
        for(size_t i = 0; i < winstonStorageSize; ++i)
            file.write('0');
        file.flush();
        file.close();
    }

    winstonStorage = SD.open(winstonStoragePath.c_str(), FILE_WRITE_BEGIN);
#endif
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

Arduino_SPIDevice::Arduino_SPIDevice(const Pin chipSelect, const unsigned int speed, SPIDataOrder order, SPIMode mode, const Pin clock, const Pin mosi, const Pin miso)
    : SPIDevice<unsigned char>(chipSelect, speed, order, mode, clock, mosi, miso), spiSettings(speed, Arduino_SPIDevice::BitOrder(order), Arduino_SPIDevice::DataMode(mode))
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
    pinMode(this->chipSelect, OUTPUT);
    digitalWrite(this->chipSelect, HIGH);
    return winston::Result::OK;
}
const winston::Result Arduino_SPIDevice::send(const std::span<DataType> data)
{
    if (this->skip)
        return winston::Result::OK;

    digitalWrite(this->chipSelect, LOW);
    SPI.beginTransaction(this->spiSettings);
    SPI.transfer((unsigned char*)&data.front(), data.size() * sizeof(DataType));
    SPI.endTransaction();
    digitalWrite(this->chipSelect, HIGH);

    return winston::Result::OK;
}

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
            while (!Serial) { //}&& millis() < 2000) {
                // Wait for Serial to initialize
            }
            text("Winston Teensy Init Hello");

#ifdef WINSTON_WITH_TEENSYDEBUG
            debug.begin(SerialUSB1);
#endif

#ifdef WINSTON_WITH_SDFAT
            if (!SD.begin(BUILTIN_SDCARD)) {
                SD.sdfs.printSdError(&Serial);
            }
            else
                winston::runtimeEnablePersistence();
            SD.sdfs.chdir();
            ensureStorageFile();
#endif
            uint8_t mac[6];
            teensyMAC(mac);
#if defined(WINSTON_ETHERNET_IP) && defined(WINSTON_ETHERNET_DNS) && defined(WINSTON_ETHERNET_GATEWAY) && defined(WINSTON_ETHERNET_SUBNET)
            Ethernet.begin(mac, WINSTON_ETHERNET_IP, WINSTON_ETHERNET_DNS, WINSTON_ETHERNET_GATEWAY, WINSTON_ETHERNET_SUBNET);
#else
            if (!Ethernet.begin(mac))
                error("Failed to start Ethernet\n"_s);
            else
 #endif
                winston::runtimeEnableNetwork();

            logRuntimeStatus();
        }
#ifdef WINSTON_TEENSY_FLASHSTRING
        const std::string __FlashStorageStringtoStd(const __FlashStringHelper* fsh)
        {
            PGM_P p = reinterpret_cast<PGM_P>(fsh);
            std::string ret;
            while (1) {
                unsigned char c = pgm_read_byte(p++);
                if (c == 0) break;
                ret += c;
            }
            return ret;
        }
        
        size_t stream(const __FlashStringHelper* fsh, std::function<size_t(uint8_t)> target)
        {
            PGM_P p = reinterpret_cast<PGM_P>(fsh);
            size_t ret = 0;
            while (1) {
                unsigned char c = pgm_read_byte(p++);
                if (c == 0) break;
                ret += target(c);
            }
            return ret;
        }

        void text(const __FlashStringHelper* fsh)
        {
            text(__FlashStorageStringtoStd(fsh));
        }
        void error(const __FlashStringHelper* fsh)
        {
            logger.err(__FlashStorageStringtoStd(fsh));
        }
        void fatal(const __FlashStringHelper* fsh)
        {
            logger.log(__FlashStorageStringtoStd(fsh), Logger::Entry::Level::Fatal);
            exit(-1);
        }
#endif

        void text(const std::string& text)
        {
            Serial.println(text.c_str());
        }
        
        void error(const std::string& error)
        {
            logger.err(error);
        }
        
        void fatal(const std::string reason)
        {
            logger.log(reason, Logger::Entry::Level::Fatal);
            exit(-1);
        }

        void delay(const unsigned int ms)
        {
            ::delay(ms);
        }

        TimePoint now()
        {
            return TimePoint(std::chrono::milliseconds(millis()));
        }

        void storageSetFilename(std::string filename)
        {
            winstonStoragePath = std::string(filename).append(".").append(constWinstonStoragePath);
        }

        const uint8_t storageRead(const size_t address)
        {
#ifdef WINSTON_WITH_SDFAT
            if (!winston::runtimePersistence())
                return 0;

            if (winstonStorage.size() > address)
            {
                winstonStorage.seek(address);
                return winstonStorage.read();
            }
            else
#endif
                return 0;
        }

        void storageWrite(const size_t address, const uint8_t data)
        {
#ifdef WINSTON_WITH_SDFAT
            if (!winston::runtimePersistence())
                return;

            if (winstonStorage.size() > address)
            {
                winstonStorage.seek(address);
                winstonStorage.write(data);
            }
#endif
        }

        bool storageCommit()
        {
#ifdef WINSTON_WITH_SDFAT
            if (!winston::runtimePersistence())
                return true;
            winstonStorage.flush();
#endif
            return true;
        }
    }
}