#pragma once

#include <functional>
#include <string>

namespace winston {
    template<class _WebSocketClientConnection>
    class WebSocketClient
    {
    public:
        WebSocketClient() : onMessage(), connection() { };
        ~WebSocketClient() = default;
        using OnMessage = std::function<void(_WebSocketClientConnection& client, const std::string& message)>;
        using Connection = _WebSocketClientConnection;

        virtual const Result init(OnMessage onMessage) = 0;
        virtual const Result connect(const URI& uri) = 0;
        virtual void send(const std::string message) = 0;
        virtual void step() = 0;
        virtual void shutdown() = 0;
        virtual const size_t maxMessageSize() = 0;

        virtual const bool connected() = 0;

    protected:

        OnMessage onMessage;
        Connection connection;
    };
};