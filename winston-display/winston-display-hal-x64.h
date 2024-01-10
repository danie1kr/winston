#pragma once
#include "../libwinston/WebClient.h"
#include "../libwinston/HAL.h"

class DisplayUXWin : public winston::hal::DisplayUX, public winston::Shared_Ptr<DisplayUXWin>
{
public:
	DisplayUXWin(const unsigned int width, const unsigned int height);
	virtual ~DisplayUXWin() = default;
	virtual const winston::Result init();
	virtual const winston::Result setCursor(unsigned int x, unsigned int y);
	virtual const bool getTouch(unsigned int& x, unsigned int& y);
	virtual const winston::Result draw(unsigned int x, unsigned int y, unsigned int w, unsigned int h, void* data);
	virtual const winston::Result brightness(unsigned char value);
	virtual const unsigned char brightness();
	virtual const unsigned int tick();
	using winston::Shared_Ptr<DisplayUXWin>::Shared;
	using winston::Shared_Ptr<DisplayUXWin>::make;
};
using DisplayUX = DisplayUXWin;

#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define _WEBSOCKETPP_CPP11_STL_
#define ASIO_STANDALONE
#include "../winston/external/websocketpp/config/asio_no_tls_client.hpp"
#include "../winston/external/websocketpp/client.hpp"

using ConnectionWSPP = websocketpp::connection_hdl;
class WebSocketClientWin : public winston::WebSocketClient<ConnectionWSPP>
{
public:
	using Client = ConnectionWSPP;
	WebSocketClientWin();
	~WebSocketClientWin() = default;

	void init(OnMessage onMessage, std::string uri);
	void send(const std::string message);
	void step();
	void shutdown();
	const bool connected();
	const size_t maxMessageSize();
private:
	void on_msg(Client hdl, websocketpp::config::asio_client::message_type::ptr msg);
	void on_fail(websocketpp::connection_hdl hdl);
	websocketpp::client<websocketpp::config::asio_client> client;
	websocketpp::client<websocketpp::config::asio_client>::connection_ptr connection;
};
using WebSocketClient = WebSocketClientWin;