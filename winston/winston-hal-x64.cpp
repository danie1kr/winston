#include "../libwinston/HAL.h"
#include "../libwinston/Util.h"

#include <iostream>
#include <fstream>

#include "mio.hpp"
#include "winston-hal-x64.h"

#pragma comment(lib, "ws2_32.lib")

static const std::string constWinstonStoragePath = "winston.storage";
static std::string winstonStoragePath = constWinstonStoragePath;
static const auto winstonStorageSize = 32 * 1024;
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

UDPSocketLWIP::UDPSocketLWIP(const std::string ip, const unsigned short port) : winston::hal::UDPSocket(ip, port)
{
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    this->addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

const winston::Result UDPSocketLWIP::connect()
{
    closesocket(this->udpSocket);
    this->udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    this->state = State::Connecting;
    return winston::Result::OK;
}

const winston::Result UDPSocketLWIP::send(const std::vector<unsigned char> data)
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
namespace winston::hal
{
    void init()
    {
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

    void delay(const unsigned int ms)
    {
        Sleep(ms);
    }
    
    unsigned long now()
    {
        return GetTickCount();
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