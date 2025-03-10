#include "winston-hal-teensy.h"

#ifdef WINSTON_TEENSY_FLASHSTRING
namespace winston
{
    std::string build(const __FlashStringHelper* fsh)
    {
        return hal::__FlashStorageStringtoStd(fsh);
    }
};
#else
const char* operator "" _s(const char* in, size_t len)
{
    return in;
}
#endif

#include "../libwinston/HAL.h"
#include "../libwinston/Util.h"

#ifdef WINSTON_WITH_TEENSYDEBUG
#include "TeensyDebug/TeensyDebug.h"
#endif

/*
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
*/

#ifdef WINSTON_HAL_USE_SOCKETS
UDPSocketTeensy::UDPSocketTeensy(const std::string ip, const unsigned short port) : winston::hal::Socket(), ip(ip), port(port)
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

TCPSocketTeensy::TCPSocketTeensy(const std::string ip, const unsigned short port) : winston::hal::Socket(), ip(ip), port(port)
{
    Tcp.connect(ip.c_str(), port);
}

const winston::Result TCPSocketTeensy::send(const std::vector<unsigned char> data)
{
    auto sz = (int)(data.size() * sizeof(unsigned char));
    Tcp.write(reinterpret_cast<const char*>(data.data()), sz);

    return winston::Result::OK;
}

const winston::Result TCPSocketTeensy::recv(std::vector<unsigned char>& data)
{
    int packetSize = Tcp.available();
    if (packetSize) {
        data.resize(packetSize);
        Tcp.read(data.data(), data.size());
    }
    return winston::Result::OK;
}
#endif

#ifdef WINSTON_HAL_USE_SPI
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
    pinMode(this->chipSelect, OUTPUT);
    digitalWrite(this->chipSelect, HIGH);
    return winston::Result::OK;
}
const winston::Result Arduino_SPIDevice::send(const std::vector<DataType> data)
{
    if (this->skip)
        return winston::Result::OK;

#ifdef WINSTON_TEENSY_SPI_DEBUG
    winston::logger.info("SPI send");
    std::string spi("");
    for (size_t i = 0; i < data.size(); ++i)
    {
        if (data[i] == 0xFF) spi += "FF";
        else if (data[i] == 0x0F) spi += "0F";
        else if (data[i] == 0xF0) spi += "F0";
        else if (data[i] == 0x00) spi += "00";
        else
            spi += "??";

        if (i % 2 == 0)
            spi += " ";
    }
    winston::logger.info(spi);
#endif

    const unsigned int spiDelay = 5;
    digitalWriteFast(this->chipSelect, LOW);
    delay(spiDelay);
    SPI.beginTransaction(this->spiSettings);
    SPI.transfer((unsigned char*)&data.front(), data.size() * sizeof(DataType));
    SPI.endTransaction();
    delay(spiDelay);
    digitalWriteFast(this->chipSelect, HIGH);
    delay(spiDelay);
    digitalWriteFast(this->chipSelect, LOW);
    return winston::Result::OK;
}
#endif

#ifdef WINSTON_HAL_USE_GPIO
Arduino_GPIOOutputPin::Arduino_GPIOOutputPin(const Pin pin, const State initial)
    : winston::GPIODigitalPinOutputDevice(pin, initial), winston::Shared_Ptr<Arduino_GPIOOutputPin>()
{
    pinMode(pin, OUTPUT);
    digitalWriteFast(pin, initial == State::Low ? LOW : HIGH);
}

void Arduino_GPIOOutputPin::Arduino_GPIOOutputPin::set(const State value)
{
    digitalWriteFast(pin, value == State::Low ? LOW : HIGH);
}
#endif

#ifdef WINSTON_HAL_USE_STORAGE
//SdFat SD;
StorageArduino::StorageArduino(const std::string filename, const size_t maxSize)
    : StorageInterface(maxSize), filename(filename)
{
}

const winston::Result StorageArduino::init()
{
#ifdef WINSTON_WITH_SDFAT
    if (!winston::runtimePersistence())
        return winston::Result::ExternalHardwareFailed;

    if (!SD.exists(this->filename.c_str()))
    {
        Serial.println("file does not exist, creating");
        auto file = SD.open(this->filename.c_str(), FILE_WRITE);
        for (size_t i = 0; i < capacity; ++i)
            file.write('0');
        file.flush();
        file.close();
    }
    else
        Serial.println("file found");

    //SD.sdfs.chdir();
    this->file = SD/*.sdfs*/.open(this->filename.c_str(), O_RDWR);
    if (!this->file)
    {
        Serial.println("could not open file!");
        return winston::Result::NotFound;
    }
#endif
    return winston::Result::OK;
}

const winston::Result StorageArduino::readVector(const size_t address, std::vector<unsigned char>& content, const size_t length)
{
#ifdef WINSTON_WITH_SDFAT
    if (!this->file)
        return winston::Result::NotInitialized;
    const size_t count = length == 0 ? this->file.size(): min((size_t)this->file.size(), length);
    if (this->file.size() < address + count)
    {
        winston::logger.err("storage too small");
        return winston::Result::OutOfBounds;
    }
    content.reserve(count);
    this->file.seek(address);
    this->file.read(content.data(), count);
#endif
    return winston::Result::OK;
}

const winston::Result StorageArduino::readString(const size_t address, std::string& content, const size_t length)
{
#ifdef WINSTON_WITH_SDFAT
    if (!this->file)
        return winston::Result::NotInitialized;
    const size_t count = length == 0 ? this->file.size() : min((size_t)this->file.size(), length);
    if (this->file.size() < address + count)
    {
        winston::logger.err("storage too small");
        return winston::Result::OutOfBounds;
    }
    content.clear();
    content.reserve(count);
    this->file.seek(address);
    for (size_t i = 0; i < count; ++i)
    {
        char byte;
        this->file.readBytes(&byte, 1);
        content.push_back(static_cast<unsigned char>(byte));
    }
#endif
    return winston::Result::OK;
}

const winston::Result StorageArduino::read(const size_t address, unsigned char& content)
{
#ifdef WINSTON_WITH_SDFAT
    if (!this->file)
        return winston::Result::NotInitialized;
    if (this->file.size() < address + 1)
    {
        winston::logger.err("storage too small");
        return winston::Result::OutOfBounds;
    }
    this->file.seek(address);
    char readByte;
    this->file.readBytes(&readByte, 1);
    content = (unsigned char)readByte;
#endif
    return winston::Result::OK;
}

const winston::Result StorageArduino::write(const size_t address, unsigned char content)
{
#ifdef WINSTON_WITH_SDFAT
    if (!this->file)
        return winston::Result::NotInitialized;
    if (this->file.size() < address + 1)
    {
        winston::logger.err("storage too small");
        return winston::Result::OutOfBounds;
    }
    this->file.seek(address);
    this->file.write(content);
#endif
    return winston::Result::OK;
}

const winston::Result StorageArduino::writeVector(const size_t address, const std::vector<unsigned char>& content, const size_t length)
{
#ifdef WINSTON_WITH_SDFAT
    if (!this->file)
        return winston::Result::NotInitialized;
    const size_t count = length == 0 ? content.size() : min((size_t)content.size(), length);
    if (this->file.size() < address + count)
    {
        winston::logger.err("storage too small");
        return winston::Result::OutOfBounds;
    }
    this->file.seek(address);
    this->file.write(content.data(), count);
#endif
    return winston::Result::OK;
}

const winston::Result StorageArduino::writeString(const size_t address, const std::string& content, const size_t length)
{
#ifdef WINSTON_WITH_SDFAT
    if (!this->file)
        return winston::Result::NotInitialized;
    const size_t count = length == 0 ? content.size() : min(content.size(), length);
    if (this->file.size() < address + count)
    {
        winston::logger.err("storage too small");
        return winston::Result::OutOfBounds;
    }
    this->file.seek(address);
    this->file.write(content.data(), count);
#endif
    return winston::Result::OK;
}

const winston::Result StorageArduino::sync()
{
#ifdef WINSTON_WITH_SDFAT
    if (!this->file)
        return winston::Result::NotInitialized;
    this->file.flush();
#endif
    return winston::Result::OK;
}
#endif

#ifdef WINSTON_PLATFORM_TEENSY
void teensyMAC(uint8_t* mac) { // there are 2 MAC addresses each 48bit 
    const unsigned int m1 = HW_OCOTP_MAC1;
    const unsigned int m2 = HW_OCOTP_MAC0;
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
#ifdef WINSTON_WITH_TEENSYDEBUG
            //SerialUSB1.begin(115200);
            while (!SerialUSB1) {}    // Wait for Debugger connect
            debug.begin(SerialUSB1);
#endif

            Serial.begin(115200);
            if (Serial)
                winston::runtimeEnableSerial();
            
            /*
            while (!Serial) { //}&& millis() < 2000) {
                // Wait for Serial to initialize
            }*/
            text("Winston Teensy Init Hello");

            SPI.begin();
            text("Winston Teensy SPI done");

#ifdef WINSTON_WITH_SDFAT
            if (!SD.begin(BUILTIN_SDCARD)) {
                SD.sdfs.printSdError(&Serial);
            }
            else
                winston::runtimeEnablePersistence();
            text("Winston Teensy SD done");
            SD.sdfs.chdir();

            if (winston::runtimePersistence() && CrashReport)
            {
                File log = SD.open("crash.log", FILE_WRITE);
                if (log)
                {
                    log.println("===");
                    log.print(CrashReport);
                    log.close();
                }
            }
#endif
            uint8_t mac[6];
            teensyMAC(mac);
#if defined(WINSTON_ETHERNET_IP) && defined(WINSTON_ETHERNET_DNS) && defined(WINSTON_ETHERNET_GATEWAY) && defined(WINSTON_ETHERNET_SUBNET)
            #ifdef WINSTON_WITH_QNETHERNET
            Ethernet.begin(WINSTON_ETHERNET_IP, WINSTON_ETHERNET_SUBNET, WINSTON_ETHERNET_GATEWAY);
#else
            Ethernet.begin(mac, WINSTON_ETHERNET_IP, WINSTON_ETHERNET_DNS, WINSTON_ETHERNET_GATEWAY, WINSTON_ETHERNET_SUBNET);
#endif
#else

            if (!Ethernet.begin(mac))
                error("Failed to start Ethernet\n"_s);
            else
 #endif
                winston::runtimeEnableNetwork();
            text("Winston Teensy Ethernet done");

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
            if(winston::runtimeSerial())
                Serial.println(text.c_str());
        }
        
        void error(const std::string& error)
        {
            logger.err(error);
        }
        
        void fatal(const std::string reason)
        {
            logger.log(reason, Logger::Entry::Level::Fatal);
            //exit(-1);
        }

        void delay(const unsigned int ms)
        {
            ::delay(ms);
        }

        TimePoint now()
        {
            return TimePoint(std::chrono::milliseconds(millis()));
        }
        /*
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
        }*/
    }
}
#endif
