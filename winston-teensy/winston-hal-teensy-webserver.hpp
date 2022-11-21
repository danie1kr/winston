#pragma once
#include "winston-hal-teensy.h"

#include <Arduino.h>

#include "Winston.h"
#include "Signal.h"
#include "HAL.h"
#include "Log.h"

#define WEBSOCKETS_USE_ETHERNET     true
#define USE_NATIVE_ETHERNET         true

#define WINSTON_WEBSOCKETS_WebSockets2_Generic
//#define WINSTON_WEBSOCKETS_ArduinoWebsockets
#ifdef WINSTON_WITH_WEBSOCKET
#ifdef WINSTON_WEBSOCKETS_ArduinoWebsockets
#include <ArduinoWebsockets.h>
using namespace websockets;
#endif
#ifdef WINSTON_WEBSOCKETS_WebSockets2_Generic
#include <WebSockets2_Generic.h>
using namespace websockets2_generic;
#endif
#endif


// move to https://github.com/khoih-prog/AsyncWebServer_Teensy41 ?
// #include <AsyncWebServer_Teensy41.h>

#ifdef WINSTON_WITH_WEBSOCKET
class WebServerTeensy : public winston::WebServer<WebsocketsClient>
{
public:
    using Client = WebsocketsClient;
    using HTTPClient = EthernetClient;

    class HTTPConnectionTeensy : public winston::WebServer<WebsocketsClient>::HTTPConnection
    {
    public:
        HTTPConnectionTeensy(HTTPClient& connection);
        bool status(const unsigned int HTTPStatus);
        bool header(const std::string& key, const std::string& value);
        bool body(const std::string& content);
#ifdef WINSTON_TEENSY_FLASHSTRING
        bool header(const __FlashStringHelper* key, const __FlashStringHelper* value);
        bool body(const __FlashStringHelper* content);
#endif
    private:
        HTTPClient& connection;
        unsigned char guard;

        enum class State : unsigned char
        {
            NEW = 0,
            STATUS = 0b1,
            HEADER = 0b10,
            BODY = 0b100
        };

        size_t bufferPopulated;
        unsigned char buffer[512];
    };
    using HTTPConnection = HTTPConnectionTeensy;
    using OnHTTP = std::function<void(HTTPConnection& client, const std::string& resource)>;

    WebServerTeensy();
    virtual ~WebServerTeensy() = default;
    virtual void init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port);
    virtual void send(Client& connection, const std::string& data);
    virtual void step();
    virtual void shutdown();
    virtual Client& getClient(const unsigned int clientId);
    virtual const unsigned int getClientId(Client client);
    virtual const unsigned int newConnection(Client& client);
    virtual void disconnect(Client client);
    virtual const size_t maxMessageSize();
private:
    void advanceConnectionIterator();
    Connections::iterator it;
    WebsocketsServer server;
    EthernetServer httpServer;
    OnHTTP onHTTP;
};
using WebServer = WebServerTeensy;
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
    //*/

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
                    httpClient.close();
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
            //auto client = this->getClient(id);
            connection.onMessage([=](WebsocketsClient& client, WebsocketsMessage message)
                {
                    Serial.print(message.c_str());
                    const auto msg = std::string(message.data().c_str());
                    this->onMessage(client, msg);
                });

            connection.onEvent([=](WebsocketsClient& client, WebsocketsEvent event, String data)
                {
                    if (event == WebsocketsEvent::ConnectionClosed)
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
                        //this->disconnect(this->getClient(id));
                    }
                });
            this->newConnection(connection);
        }


    }
    if (this->connections.size() == 0)
        return;
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

WebServerTeensy::Client& WebServerTeensy::getClient(const unsigned int clientId)
{
    return this->connections[clientId];
}

const unsigned int WebServerTeensy::getClientId(Client client)
{
    // unused
    return 0;
}

void WebServerTeensy::advanceConnectionIterator()
{
    ++this->it;
    if (this->it == this->connections.end())
        this->it = this->connections.begin();
}

const unsigned int WebServerTeensy::newConnection(Client& client)
{
    const unsigned int id = this->newClientId();
    this->connections.emplace(id, client);
    if (this->it == this->connections.end())
        this->it = this->connections.begin();

    return id;
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

const size_t WebServerTeensy::maxMessageSize()
{
    return 512;
}
#endif