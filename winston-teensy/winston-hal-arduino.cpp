#include "winston-hal-arduino.h"

#ifdef WINSTON_HAL_USE_STORAGE
SdFat SD;
StorageArduino::StorageArduino(const std::string filename, const size_t maxSize)
    : StorageInterface(maxSize), filename("/" + filename)
{
}

const winston::Result StorageArduino::init(const bool rebuildIfTooSmall)
{
#ifdef WINSTON_WITH_SDFAT
    if (!winston::runtimePersistence())
        return winston::Result::ExternalHardwareFailed;

    Serial.print("file "); Serial.print(this->filename.c_str());

    if (!SD.exists(this->filename.c_str()))
    {
        Serial.println(" does not exist, creating");
        auto file = SD.open(this->filename.c_str(), O_RDWR | O_CREAT);
        for (size_t i = 0; i < capacity; ++i)
            file.write('0');
        file.flush();
        Serial.print("  capacity: ");
        Serial.println(file.size());
        file.close();
    }
    else
    {
        auto file = SD.open(this->filename.c_str(), O_RDWR);
        if (rebuildIfTooSmall && file.size() < this->capacity)
        {
            Serial.print(" resizing from: ");
            Serial.print(file.size());
            Serial.print(" to ");
            Serial.print(this->capacity);
            for (size_t i = 0; i < capacity; ++i)
                file.write('0');
            file.flush();
            Serial.print(" resized: ");
            Serial.println(file.size());
        }
        else
            Serial.println(" found");
        file.close();
    }

    this->file = SD.open(this->filename.c_str(), O_RDWR);

    if (!this->file)
    {
        Serial.print("could not open file: ");
        Serial.println(this->filename.c_str());
        return winston::Result::NotFound;
    }
    else
    {
        Serial.print("Filesize of ");
		Serial.print(this->filename.c_str());
		Serial.print(": ");
        Serial.println(this->file.size());
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
        winston::logger.err("StorageArduino::readVector: storage too small: ");
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
        winston::logger.err("StorageArduino::readString: storage too small: ", this->filename.c_str());
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
        winston::logger.err("StorageArduino::read: storage too small: ", this->filename.c_str());
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
        winston::logger.err("StorageArduino::write: storage too small: ", this->filename.c_str());
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
        winston::logger.err("StorageArduino::writeVector: storage too small: ", this->filename.c_str());
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
        winston::logger.err("StorageArduino::writeString: storage too small: ", this->filename.c_str());
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
