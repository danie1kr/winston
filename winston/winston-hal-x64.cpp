#include "../libwinston/Winston.h"
#include "../libwinston/HAL.h"
#include "../libwinston/Util.h"
#include "../libwinston/Log.h"

#include <iostream>
#include <fstream>

#include "winston-hal-x64.h"

#ifdef WINSTON_HAL_USE_SOCKETS
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <windows.h>
#endif

StatusDisplay statusDisplay;

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
#ifdef WINSTON_HAL_USE_SOCKETS
                WSADATA wsaData;
                if (!WSAStartup(MAKEWORD(1, 1), &wsaData))
                    runtimeEnableNetwork();
#endif
            }
#ifdef WINSTON_WITH_STATUSDISPLAY
            statusDisplay.init();
#endif
        }

        void text(const std::string& text)
        {
            std::cout << text << std::endl;
        }

        void error(const std::string& error)
        {
            LOG_ERROR(error);
        }

        void fatal(const std::string reason)
        {
            LOG(reason, Logger::Entry::Level::Fatal);
            throw std::exception(reason.c_str());
            exit(-1);
        }

#ifdef WINSTON_TEST
        unsigned int sleepyTime = 0;
        TimePoint initialNow = winston::hal::now();
        void delay(const unsigned int ms)
        {
            sleepyTime += ms;
        }

        void delayReset()
        {
            sleepyTime = 0;
        }

        TimePoint now()
        {
            //return std::chrono::system_clock::now() + std::chrono::milliseconds(sleepyTime);
            return initialNow + std::chrono::milliseconds(sleepyTime);
        }
#else
        void delay(const unsigned int ms)
        {
            Sleep(ms);
        }

        TimePoint now()
        {
            return std::chrono::system_clock::now();
        }
#endif
    }
}

#ifdef WINSTON_HAL_USE_WEBSERVER
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

WebServerWSPP::HTTPConnectionWSPP::HTTPConnectionWSPP(HTTPClient& connection) 
    : HTTPConnection(), connection(connection) 
{
};

bool WebServerWSPP::HTTPConnectionWSPP::status(const unsigned int HTTPStatus)
{
    this->connection->set_status(websocketpp::http::status_code::value(HTTPStatus));
    return true;
}
bool WebServerWSPP::HTTPConnectionWSPP::header(const std::string& key, const std::string& value)
{
    this->connection->append_header(key, value);
    return true;
}
bool WebServerWSPP::HTTPConnectionWSPP::body(const std::string& content)
{
    this->fullBody += content;
    return true;
}
bool WebServerWSPP::HTTPConnectionWSPP::body(const unsigned char* content, size_t length, size_t chunked)
{
    std::string string(reinterpret_cast<const char*>(content));
    this->fullBody += string;
    return true;
}

void WebServerWSPP::HTTPConnectionWSPP::submit()
{
    this->connection->set_body(this->fullBody);
}

WebServerWSPP::WebServerWSPP() : winston::WebServer<ConnectionWSPP>()
{

}

void WebServerWSPP::init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port)
{
    if (!winston::runtimeNetwork())
        return;

    this->onHTTP = onHTTP;
    this->onMessage = onMessage;
    this->server.init_asio();

#ifdef WINSTON_WEBSERVER_WITH_DEBUG
    this->server.set_access_channels(websocketpp::log::alevel::all);
    this->server.set_error_channels(websocketpp::log::elevel::all);
#else
    this->server.clear_access_channels(websocketpp::log::alevel::all);
    this->server.clear_error_channels(websocketpp::log::elevel::all);
#endif
    this->server.set_http_handler(websocketpp::lib::bind(&WebServerWSPP::on_http, this, websocketpp::lib::placeholders::_1));
    this->server.set_message_handler(websocketpp::lib::bind(&WebServerWSPP::on_msg, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
    this->server.set_open_handler(websocketpp::lib::bind(&WebServerWSPP::on_open, this, websocketpp::lib::placeholders::_1));
    this->server.set_fail_handler(websocketpp::lib::bind(&WebServerWSPP::on_fail, this, websocketpp::lib::placeholders::_1));
    this->server.set_close_handler(websocketpp::lib::bind(&WebServerWSPP::on_close, this, websocketpp::lib::placeholders::_1));

    this->server.listen(port);

    this->server.start_accept();
}

void WebServerWSPP::send(Client& connection, const std::string &data)
{
    try
    {
        this->server.send(connection, data, websocketpp::frame::opcode::text);
    }
    catch (std::exception e)
    {
       // LOG_ERROR("WebServerWSPP::send exception, umlaut in payload? ", data);
       // connection.reset();
    }
}

void WebServerWSPP::step()
{
    if (!winston::runtimeNetwork())
        return;

    this->server.poll_one();
}

WebServerWSPP::Client& WebServerWSPP::getClient(const unsigned int clientId)
{
    return this->connections[clientId];
}
const unsigned int WebServerWSPP::getClientId(Client client)
{
    auto result = std::find_if(
        this->connections.begin(),
        this->connections.end(),
        [client](const auto& it) {return it.second.lock() == client.lock(); });

    if (result != this->connections.end())
        this->connections.erase(result->first);
    return 0;
}

void WebServerWSPP::disconnect(Client client)
{
    auto id = this->getClientId(client);

    if (id)
        this->connections.erase(id);
}

void WebServerWSPP::on_http(Client hdl)
{
    auto con = this->server.get_con_from_hdl(hdl);
    HTTPConnectionWSPP connection(con);
    this->onHTTP(connection, winston::HTTPMethod::_from_string(con->get_request().get_method().c_str()), con->get_resource());

    /*
    auto con = this->server.get_con_from_hdl(hdl);
    const auto response = this->onHTTP(hdl, con->get_resource());

    for (auto const& kv: response.headers)
        con->append_header(kv.first, kv.second);
    con->set_status(websocketpp::http::status_code::value(response.status));
    con->set_body(response.body);
    */
}

void WebServerWSPP::on_msg(Client hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg)
{
    this->onMessage(hdl, msg->get_payload());
}

void WebServerWSPP::on_open(Client hdl) {
    this->newConnection(hdl);
}

void WebServerWSPP::on_fail(Client hdl) {
    this->disconnect(hdl);
}

void WebServerWSPP::on_close(Client hdl) {
    this->disconnect(hdl);
}

void WebServerWSPP::shutdown()
{
    this->server.stop();
}

const size_t WebServerWSPP::maxMessageSize()
{
    return this->server.get_max_message_size();
}
#endif

/*
UDPSocketWinSock::UDPSocketWinSock(const std::string ip, const unsigned short port) : winston::hal::UDPSocket(ip, port)
{
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &this->addr.sin_addr.s_addr); 
    
    u_long nMode = 1; // 1: NON-BLOCKING
    ioctlsocket(this->udpSocket, FIONBIO, &nMode);

    this->connect();
}

const winston::Result UDPSocketWinSock::connect()
{
    if (!winston::runtimeNetwork())
        return winston::Result::NotInitialized;

    closesocket(this->udpSocket);
    this->udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    this->state = State::Connecting;
    return winston::Result::OK;
}

const winston::Result UDPSocketWinSock::send(const std::vector<unsigned char> data)
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

const winston::Result UDPSocketWinSock::recv(std::vector<unsigned char>& data)
{
    sockaddr_in from;
    int size = (int)sizeof(from);
    data.resize(1000);
    
    fd_set fds;
    struct timeval tv;

    // Set up the file descriptor set.
    FD_ZERO(&fds);
    FD_SET(this->udpSocket, &fds);

    // Set up the struct timeval for the timeout.
    tv.tv_sec = 0;
    tv.tv_usec = 0; // anything > 0 is at least 14ms

    // Wait until timeout or data received.
    if (select(0, &fds, NULL, NULL, &tv) > 0) {
        int ret = recvfrom(this->udpSocket, reinterpret_cast<char*>(data.data()), (int)data.size(), 0, reinterpret_cast<SOCKADDR*>(&from), &size);
        if (ret < 0)
            return winston::Result::ReceiveFailed;

        // make the buffer zero terminated
        data.resize(ret);
    }
    else
        data.resize(0);
    return winston::Result::OK;
}
*/
#ifdef WINSTON_HAL_USE_SERIAL
SerialDeviceWin::SerialDeviceWin()
	: winston::hal::SerialDevice(), winston::Shared_Ptr<SerialDeviceWin>(), serialHandle(INVALID_HANDLE_VALUE), timeouts()
{

}

bool SerialDeviceWin::init(const size_t portNumber, const size_t bauds,
    const SerialDataBits databits,
    const SerialParity parity,
    const SerialStopBits stopbits)
{

    std::string port = "COM";
    port.append(winston::build(portNumber));

    this->serialHandle = CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (this->serialHandle == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return false; // Device not found

        // Error while opening the device
        return false;
    }

    // Set parameters

    // Structure for the port parameters
    DCB dcbSerialParams;
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    // Get the port parameters
    if (!GetCommState(this->serialHandle, &dcbSerialParams)) return false;

    // Set the speed (Bauds)
    switch (bauds)
    {
    case 110:      dcbSerialParams.BaudRate = CBR_110; break;
    case 300:      dcbSerialParams.BaudRate = CBR_300; break;
    case 600:      dcbSerialParams.BaudRate = CBR_600; break;
    case 1200:     dcbSerialParams.BaudRate = CBR_1200; break;
    case 2400:     dcbSerialParams.BaudRate = CBR_2400; break;
    case 4800:     dcbSerialParams.BaudRate = CBR_4800; break;
    case 9600:     dcbSerialParams.BaudRate = CBR_9600; break;
    case 14400:    dcbSerialParams.BaudRate = CBR_14400; break;
    case 19200:    dcbSerialParams.BaudRate = CBR_19200; break;
    case 38400:    dcbSerialParams.BaudRate = CBR_38400; break;
    case 56000:    dcbSerialParams.BaudRate = CBR_56000; break;
    case 57600:    dcbSerialParams.BaudRate = CBR_57600; break;
    case 115200:   dcbSerialParams.BaudRate = CBR_115200; break;
    case 128000:   dcbSerialParams.BaudRate = CBR_128000; break;
    case 256000:   dcbSerialParams.BaudRate = CBR_256000; break;
    default: return false;
    }

    switch (databits) {
    case SerialDataBits::SERIAL_DATABITS_5: dcbSerialParams.ByteSize = 5; break;
    case SerialDataBits::SERIAL_DATABITS_6: dcbSerialParams.ByteSize = 6; break;
    case SerialDataBits::SERIAL_DATABITS_7: dcbSerialParams.ByteSize = 7; break;
    case SerialDataBits::SERIAL_DATABITS_8: dcbSerialParams.ByteSize = 8; break;
    case SerialDataBits::SERIAL_DATABITS_16: dcbSerialParams.ByteSize = 16; break;
    default: return false;
    }

    switch (stopbits) {
    case SerialStopBits::SERIAL_STOPBITS_1: dcbSerialParams.StopBits = ONESTOPBIT; break;
    case SerialStopBits::SERIAL_STOPBITS_1_5: dcbSerialParams.StopBits = ONE5STOPBITS; break;
    case SerialStopBits::SERIAL_STOPBITS_2: dcbSerialParams.StopBits = TWOSTOPBITS; break;
    default: return false;
    }

    switch (parity) {
    case SerialParity::SERIAL_PARITY_NONE: dcbSerialParams.Parity = NOPARITY; break;
    case SerialParity::SERIAL_PARITY_EVEN: dcbSerialParams.Parity = EVENPARITY; break;
    case SerialParity::SERIAL_PARITY_ODD: dcbSerialParams.Parity = ODDPARITY; break;
    case SerialParity::SERIAL_PARITY_MARK: dcbSerialParams.Parity = MARKPARITY; break;
    case SerialParity::SERIAL_PARITY_SPACE: dcbSerialParams.Parity = SPACEPARITY; break;
    default: return false;
    }

    // Write the parameters
    if (!SetCommState(this->serialHandle, &dcbSerialParams)) return false;

    // Set TimeOut

    // Set the Timeout parameters
    this->timeouts.ReadIntervalTimeout = 0;
    // No TimeOut
    this->timeouts.ReadTotalTimeoutConstant = MAXDWORD;
    this->timeouts.ReadTotalTimeoutMultiplier = 0;
    this->timeouts.WriteTotalTimeoutConstant = MAXDWORD;
    this->timeouts.WriteTotalTimeoutMultiplier = 0;

    // Write the parameters
    if (!SetCommTimeouts(this->serialHandle, &this->timeouts)) return false;

    // Opening successfull
    return true;
}

const size_t SerialDeviceWin::available()
{
    DWORD commErrors;
    COMSTAT commStatus;
    ClearCommError(this->serialHandle, &commErrors, &commStatus);
    return commStatus.cbInQue;
}

const SerialDeviceWin::DataType SerialDeviceWin::read()
{
    unsigned char retVal = 0;
    DWORD dwBytesRead = 0;

    // Set the TimeOut
    //timeouts.ReadTotalTimeoutConstant = timeOut_ms;

    // Write the parameters, return -1 if an error occured
    //if (!SetCommTimeouts(this->serialHandle, &timeouts)) return -1;

    // Read the byte, return -2 if an error occured
    if (!ReadFile(this->serialHandle, &retVal, 1, &dwBytesRead, NULL)) return -2;

    // Return 0 if the timeout is reached
    if (dwBytesRead == 0) return -1;

#ifdef DEBUG_SERIAL
    printf("RX: %02hhx \n", (unsigned char)retVal);
#endif
    // The byte is read
    return retVal;
}

const size_t SerialDeviceWin::read(std::vector<DataType>& content, size_t upTo)
{
    char retVal = 0;
    DWORD dwBytesRead = 0;

    // Set the TimeOut
    //timeouts.ReadTotalTimeoutConstant = timeOut_ms;

    // Write the parameters, return -1 if an error occured
    //if (!SetCommTimeouts(this->serialHandle, &timeouts)) return -1;

    // Read the byte, return -2 if an error occured
    content.resize(upTo);
    if (!ReadFile(this->serialHandle, content.data(), (DWORD)upTo, &dwBytesRead, NULL))
        return 0;

#ifdef DEBUG_SERIAL
    if (dwBytesRead)
    {
        printf("RX: ");
        for (int i = 0; i < dwBytesRead; ++i)
            printf("%02hhx ", (unsigned char)data[i]);
        printf("\n");
    }
#endif

    return dwBytesRead;
}

const winston::Result SerialDeviceWin::send(const std::vector<DataType> data)
{
    DWORD dwBytesWritten = 0;
    if (!WriteFile(this->serialHandle, data.data(), (DWORD)data.size(), &dwBytesWritten, NULL))
        return winston::Result::SendFailed;
#ifdef DEBUG_SERIAL
    printf("TX: ");
    for (int i = 0; i < dwBytesWritten; ++i)
        printf("%02hhx ", (unsigned char)data[i]);
    printf("\n");
#endif
    return dwBytesWritten == (DWORD)data.size() ? winston::Result::OK : winston::Result::SendFailed;
}
#endif

#ifdef WINSTON_HAL_USE_STORAGE
StorageWin::StorageWin(const std::string filename, const size_t capacity)
    : StorageInterface(capacity), filename(filename)
{
}

const winston::Result StorageWin::init()
{
    std::ifstream testIfExists(this->filename);
    if (!testIfExists.good())
    {
        std::ofstream file(this->filename);
        std::string s(capacity, 0);
        file << s;
    }

    std::error_code error;
    this->mmap = mio::make_mmap_sink(this->filename, 0, mio::map_entire_file, error);
    if (error)
        this->handleError(error);
    else
        winston::runtimeEnablePersistence();
    return winston::Result::OK;
}

const winston::Result StorageWin::readVector(const size_t address, std::vector<unsigned char>& content, const size_t length)
{
    const size_t count = length == 0 ? this->mmap.size() : min(this->mmap.size(), length);
    if (!this->mmap.is_open())
        return winston::Result::NotInitialized;
    content.reserve(count);
    const size_t maxI = min(address + count, this->mmap.size());
    for (size_t i = address; i < maxI; ++i)
        content.push_back(this->mmap[i]);

    return winston::Result::OK;
}

const winston::Result StorageWin::readString(const size_t address, std::string& content, const size_t length)
{
    const size_t count = length == 0 ? this->mmap.size() : min(this->mmap.size(), length);
    if (!this->mmap.is_open())
        return winston::Result::NotInitialized;
    content.reserve(count);
    const size_t maxI = min(address + count, this->mmap.size());
    for (size_t i = address; i < maxI; ++i)
        content.push_back(this->mmap[i]);

    return winston::Result::OK;
}

const winston::Result StorageWin::read(const size_t address, unsigned char& content)
{
    if (!this->mmap.is_open())
        return winston::Result::NotInitialized;

    if (this->mmap.size() < address + 1)
    {
        LOG_ERROR("storage too small");
        return winston::Result::OutOfBounds;
    }
    content = this->mmap[address];

    return winston::Result::OK;
}

const winston::Result StorageWin::write(const size_t address, unsigned char content)
{
    if (!this->mmap.is_open())
        return winston::Result::NotInitialized;
    if (this->mmap.size() < address + 1)
    {
        LOG_ERROR("storage too small");
        return winston::Result::OutOfBounds;
    }
    this->mmap[address] = content;

    return winston::Result::OK;
}

const winston::Result StorageWin::writeVector(const size_t address, const std::vector<unsigned char>& content, const size_t length)
{
    const size_t count = length == 0 ? content.size() : min(content.size(), length);
    if (!this->mmap.is_open())
        return winston::Result::NotInitialized;
    if (this->mmap.size() < address + count)
    {
        LOG_ERROR("storage too small");
        return winston::Result::OutOfBounds;
    }
    for (size_t i = 0; i < count; ++i)
        this->mmap[address + i] = content[i];

    return winston::Result::OK;
}

const winston::Result StorageWin::writeString(const size_t address, const std::string& content, const size_t length)
{
    const size_t count = length == 0 ? content.size() : min(content.size(), length);
    if (!this->mmap.is_open())
        return winston::Result::NotInitialized;
    if (this->mmap.size() < address + count)
    {
        LOG_ERROR("storage too small");
        return winston::Result::OutOfBounds;
    }
    for (size_t i = 0; i < count; ++i)
        this->mmap[address + i] = content[i];

    return winston::Result::OK;
}

const winston::Result StorageWin::sync()
{
    if (!this->mmap.is_open())
        return winston::Result::NotInitialized;

    std::error_code error;
    this->mmap.sync(error);
    if (error)
    {
        this->handleError(error);
        return winston::Result::InternalError;
    }
    return winston::Result::OK;
}

const int StorageWin::handleError(const std::error_code& error) const
{
    winston::error(error.message());
    return error.value();
}
#endif

#ifdef WINSTON_HAL_USE_DISPLAYUX
#include "..\winston-display\external\lvgl\lvgl.h"
DisplayUXWin::DisplayUXWin(const unsigned int width, const unsigned int height)
    : winston::hal::DisplayUX(width, height)
{

}

const winston::Result DisplayUXWin::init(const std::string title)
{
    lv_init();
    std::wstring wTitle(title.begin(), title.end());
    lv_display_t* display = lv_windows_create_display(
        wTitle.c_str(),
        this->width,
        this->height,
        100,
        false,
        false);
    if (!display)
    {
        return winston::Result::InternalError;
    }

    lv_indev_t* pointer_device = lv_windows_acquire_pointer_indev(display);
    if (!pointer_device)
    {
        return winston::Result::InternalError;
    }

    lv_indev_t* keypad_device = lv_windows_acquire_keypad_indev(display);
    if (!keypad_device)
    {
        return winston::Result::InternalError;
    }

    lv_indev_t* encoder_device = lv_windows_acquire_encoder_indev(display);
    if (!encoder_device)
    {
        return winston::Result::InternalError;
    }

    return winston::Result::OK;
}

const winston::Result DisplayUXWin::setCursor(unsigned int x, unsigned int y)
{
    return winston::Result::NotImplemented;
}

const bool DisplayUXWin::getTouch(unsigned int& x, unsigned int& y)
{
    return false;
}

void DisplayUXWin::displayLoadingScreen()
{
}

const winston::Result DisplayUXWin::draw(unsigned int x, unsigned int y, unsigned int w, unsigned int h, void* data)
{
    return winston::Result::NotImplemented;
}


const winston::Result DisplayUXWin::brightness(unsigned char value)
{
    return winston::Result::OK;
}

const unsigned char DisplayUXWin::brightness()
{
    return 24;
}

const unsigned int DisplayUXWin::tick()
{
    return lv_timer_handler();
}
#endif

#ifdef WINSTON_HAL_USE_WEBSOCKETCLIENT
WebSocketClientWin::WebSocketClientWin()
{
}

const winston::Result WebSocketClientWin::init(OnMessage onMessage)
{
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;

    this->onMessage = onMessage;

    this->client.init_asio();
#ifdef WINSTON_WEBSERVER_WITH_DEBUG
    this->client.set_access_channels(websocketpp::log::alevel::all);
    this->client.set_error_channels(websocketpp::log::elevel::all);
#else
    this->client.clear_access_channels(websocketpp::log::alevel::all);
    this->client.clear_error_channels(websocketpp::log::elevel::all);
#endif
    this->client.set_message_handler(websocketpp::lib::bind(&WebSocketClientWin::on_msg, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
    this->client.set_fail_handler(websocketpp::lib::bind(&WebSocketClientWin::on_fail, this, websocketpp::lib::placeholders::_1));

    return winston::Result::OK;
}

const winston::Result WebSocketClientWin::connect(const winston::URI& uri)
{
    websocketpp::lib::error_code ec;
    this->connection = this->client.get_connection(uri.toString(), ec);
    if (ec) {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    this->client.connect(this->connection);

    //while (!this->connected())
    WHILE_SAFE(!this->connected(),
    {
        this->client.poll_one();
    });
    return winston::Result::OK;
}

void WebSocketClientWin::send(const std::string message)
{
    if (this->connected())
        this->client.send(this->connection->get_handle(), message, websocketpp::frame::opcode::text);
}

const winston::Result WebSocketClientWin::loop()
{
    if (this->connected())// && this->client.poll_one())
        this->client.poll_one();

    return winston::Result::OK;
}

void WebSocketClientWin::shutdown()
{
    this->client.close(this->connection->get_handle(), websocketpp::close::status::normal, "thank you!");
}

const bool WebSocketClientWin::connected()
{
    return this->connection && this->connection->get_state() == websocketpp::session::state::open;
}

const size_t WebSocketClientWin::maxMessageSize()
{
    return this->client.get_max_message_size();
}

void WebSocketClientWin::on_msg(Client hdl, websocketpp::config::asio_client::message_type::ptr msg)
{
    this->onMessage(hdl, msg->get_payload());
}

void WebSocketClientWin::on_fail(websocketpp::connection_hdl hdl)
{
    auto con = this->client.get_con_from_hdl(hdl);

    std::cout << "WebSocketClientWin Fail handler" << std::endl;
    std::cout << con->get_state() << std::endl;
    std::cout << con->get_local_close_code() << std::endl;
    std::cout << con->get_local_close_reason() << std::endl;
    std::cout << con->get_remote_close_code() << std::endl;
    std::cout << con->get_remote_close_reason() << std::endl;
    std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
}
#endif
/*
DisplayWin::DisplayWin()
{

}

const winston::Result DisplayWin::send(const std::vector<DataType> data)
{
    for (auto& line : data)
        winston::hal::text(winston::build(line, "\n"));
    return winston::Result::OK;
}
*/

#ifdef WINSTON_WITH_STATUSDISPLAY
StatusDisplay::StatusDisplay()
    : winston::StatusDisplay<20, 8>()
    , display(200, 160)
	, labelStatus{ nullptr }
{

}

const winston::Result StatusDisplay::init()
{
	display.init("Winston Status Display");

    if(labelStatus)
		lv_obj_delete(labelStatus);

	labelStatus = lv_label_create(lv_screen_active());
    lv_obj_align(labelStatus, LV_ALIGN_CENTER, 0, 0);

    return winston::Result::OK;
}

void StatusDisplay::tick()
{
    this->display.tick();
}

const winston::Result StatusDisplay::update()
{
    std::string text;
	text.reserve(this->columns * this->rows + this->rows);

    for(size_t r = 0; r < this->rows; ++r)
        for (size_t c = 0; c < this->columns; ++c)
        {
			text.push_back(this->status[r][c]);
			if (c == this->columns - 1)
				text.push_back('\n');
        }

    lv_label_set_text(labelStatus, text.c_str());
	return winston::Result::OK;
}
#endif