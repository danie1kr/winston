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

	void init(OnMessage onMessage, const winston::URI &uri);
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
	_connected{false}
{

}

void WebSocketClientESP32::init(OnMessage onMessageCallback, const winston::URI& uri)
{
	this->client.onMessage([onMessageCallback, this](WebsocketsMessage message) {
		std::string msg(message.data().c_str());
		onMessageCallback(this->client, msg);
		});

	this->_connected = this->client.connect(uri.host.c_str(), uri.port, uri.resource.c_str());
}

void WebSocketClientESP32::send(const std::string message)
{

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