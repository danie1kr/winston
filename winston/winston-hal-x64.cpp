#include "../libwinston/HAL.h"
#include "../libwinston/Util.h"

#include <iostream>
#include <fstream>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")

#include "mio.hpp"

static const auto winstonStoragePath = "winston.storage";
static const auto winstonStorageSize = 32 * 1024;
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
}