#include "winston-hal-arduino.h"

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
#ifdef WINSTON_PLATFORM_ESP32
    this->file = SD/*.sdfs*/.open(this->filename.c_str(), FILE_WRITE);
#else
    this->file = SD/*.sdfs*/.open(this->filename.c_str(), O_RDWR);
#endif
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
    this->file.write((const unsigned char*)content.data(), count);
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
