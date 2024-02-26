#pragma once
#include "../libwinston/WinstonConfig.h"

#include <WebSockets2_Generic.h>
using namespace websockets2_generic;

#include "../libwinston/WebClient.h"
#include "../libwinston/WinstonSharedTypes.h"

class WebSocketClientESP32 : public winston::WebSocketClient<websockets2_generic::WebsocketsClient>
{
public:
	using Client = websockets2_generic::WebsocketsClient;
	WebSocketClientESP32();
	~WebSocketClientESP32() = default;

	const winston::Result init(OnMessage onMessage);
	const winston::Result connect(const winston::URI& uri);
	void send(const std::string message);
	void step();
	void shutdown();
	const bool connected();
	const size_t maxMessageSize();
private:
	Client client;
	bool _connected;
};
using WebSocketClient = WebSocketClientESP32;

WebSocketClientESP32::WebSocketClientESP32()
	: winston::WebSocketClient<websockets2_generic::WebsocketsClient>(),
	client(), _connected{false}
{

}

const winston::Result WebSocketClientESP32::init(OnMessage onMessageCallback)
{
	this->onMessage = onMessageCallback;
	this->client.onMessage([onMessageCallback](WebsocketsClient& client, WebsocketsMessage message) {
		const std::string msg(message.data().c_str());
		onMessageCallback(client, msg);
		});
	this->client.onEvent([this](WebsocketsEvent event, String data)
		{
			(void)data;

			if (event == WebsocketsEvent::ConnectionOpened)
			{
				Serial.println("Connnection Opened");
			}
			else if (event == WebsocketsEvent::ConnectionClosed)
			{
				this->_connected = false;
				Serial.println("Connnection Closed");
			}
			else if (event == WebsocketsEvent::GotPing)
			{
				Serial.println("Got a Ping!");
			}
			else if (event == WebsocketsEvent::GotPong)
			{
				Serial.println("Got a Pong!");
			}
		}
	);
	return winston::Result::OK;
}

const winston::Result WebSocketClientESP32::connect(const winston::URI& uri)
{
	this->_connected = this->client.connect(uri.host.c_str(), uri.port, uri.resource.c_str());
	return winston::Result::OK;
}

void WebSocketClientESP32::send(const std::string message)
{
	this->client.send(message.c_str(), message.length());
}

void WebSocketClientESP32::step()
{
	this->client.poll();
}

void WebSocketClientESP32::shutdown()
{
	this->client.close(CloseReason_NormalClosure);
}

const bool WebSocketClientESP32::connected()
{
	return this->_connected;
}

const size_t WebSocketClientESP32::maxMessageSize()
{
	return WINSTON_WEBSOCKET_MAXSIZE;
}