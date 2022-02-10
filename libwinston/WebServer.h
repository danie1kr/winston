#pragma once

#include <string>
#include <map>
#include <functional>

namespace winston
{
	template<class _WebSocketConnection>
	class WebServer
	{
	public:
		class HTTPConnection
		{
		public:
			virtual bool status(const unsigned int HTTPStatus) = 0;
			virtual bool header(const std::string& key, const std::string& value) = 0;
			virtual bool body(const std::string& content) = 0;
		};

		using OnHTTP = std::function<void(HTTPConnection &client, const std::string &resource)>;
		using OnMessage = std::function<void(_WebSocketConnection &client, const std::string &message)>;

		WebServer()
		{
		}

		virtual ~WebServer()
		{
		}

		void broadcast(std::string data)
		{
			for (auto const& ic : this->connections)
				this->send(ic.first, data);
		}

		virtual void send(unsigned int client, const std::string& data)
		{
			this->send(this->connections[client], data);
		}

		virtual void send(_WebSocketConnection& connection, const std::string& data) = 0;
		virtual _WebSocketConnection& getClient(unsigned int clientId) = 0;
		virtual const unsigned int getClientId(_WebSocketConnection client) = 0;
		virtual const unsigned int newConnection(_WebSocketConnection& client)
		{
			const unsigned int id = this->newClientId();
			this->connections.emplace(id, client);
			return id;
		}
		virtual void disconnect(_WebSocketConnection client) = 0;
		unsigned int clients()
		{
			return this->connections.size();
		}

		virtual void init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port) { };
		virtual void step() = 0;
		virtual void shutdown() = 0;
		virtual const size_t maxMessageSize() = 0;

	protected:
		using Connections = std::map<unsigned int, _WebSocketConnection>;
		Connections connections;

		unsigned int nextClientId = 1;
		unsigned int newClientId() { return nextClientId++; };

		OnHTTP onHTTP;
		OnMessage onMessage;
	};

};