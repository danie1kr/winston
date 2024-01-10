
#include "../libwinston/HAL.h"
#include "../libwinston/Util.h"
#include "../libwinston/Log.h"

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

const char* operator "" _s(const char* in, size_t len)
{
    return in;
}

namespace winston
{
    namespace hal {
        void init()
        {
            {
                WSADATA wsaData;
                if (!WSAStartup(MAKEWORD(1, 1), &wsaData))
                    runtimeEnableNetwork();
            }
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

        TimePoint now()
        {
            return std::chrono::system_clock::now();
        }
    }
}