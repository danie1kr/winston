
#include "winston-display-hal-x64.h"
#include "external/lvgl-win32drv.h"

DisplayUXWin::DisplayUXWin(const unsigned int width, const unsigned int height)
    : winston::hal::DisplayUX(width, height)
{

}

const winston::Result DisplayUXWin::init()
{
    lv_init();

    lv_tick_set_cb([]() -> uint32_t { return GetTickCount(); });

    if (!lv_windows_init(
        GetModuleHandleW(NULL),
        SW_SHOW,
        this->width,
        this->height,
        0))//LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_LVGL_WINDOWS))))
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

WebSocketClientWin::WebSocketClientWin()
{
}

void WebSocketClientWin::init(OnMessage onMessage, std::string uri) 
{
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;

    this->onMessage = onMessage;

    this->client.init_asio();
    this->client.set_access_channels(websocketpp::log::alevel::all);
    this->client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    this->client.set_message_handler(websocketpp::lib::bind(&WebSocketClientWin::on_msg, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
    this->client.set_fail_handler(websocketpp::lib::bind(&WebSocketClientWin::on_fail, this, websocketpp::lib::placeholders::_1));

    websocketpp::lib::error_code ec;
    this->connection = this->client.get_connection(uri, ec);
    if (ec) {
        std::cout << "Echo failed because: " << ec.message() << std::endl;
    }
    this->client.connect(this->connection);

    while (!this->connected())
        this->client.run_one();
}

    
void WebSocketClientWin::send(const std::string message)
{
    if (this->connected())
        this->client.send(this->connection->get_handle(), message, websocketpp::frame::opcode::text);
}

void WebSocketClientWin::step()
{
    if(this->connected() && this->client.poll_one())
        this->client.run_one();
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