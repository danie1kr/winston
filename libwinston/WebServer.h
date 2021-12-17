#pragma once

#include <string>
#include <map>
#include <functional>

namespace winston
{
	template<class _Connection>
	class WebServerProto
	{
	public:
		struct HTTPResponse
		{
			std::map<const std::string, const std::string> headers;
			std::string body;
			unsigned int status;
		};

		using OnHTTP = std::function<HTTPResponse(_Connection client, std::string resource)>;
		using OnMessage = std::function<void(_Connection client, std::string message)>;

		WebServerProto()
		{
		}

		virtual ~WebServerProto()
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

		virtual void send(_Connection& connection, const std::string& data) = 0;
		virtual _Connection getClient(unsigned int clientId) = 0;
		virtual unsigned int getClientId(_Connection client) = 0;
		virtual void newConnection(_Connection client)
		{
			this->connections.insert({ this->newClientId(), client });
		}
		virtual void disconnect(_Connection client) = 0;
		unsigned int clients()
		{
			return this->connections.size();
		}

		virtual void init(OnHTTP onHTTP, OnMessage onMessage, unsigned int port) = 0;
		virtual void step() = 0;
		virtual void shutdown() = 0;
		virtual size_t maxMessageSize() = 0;

	protected:
		using Connections = std::map<unsigned int, _Connection>;
		Connections connections;

		unsigned int nextClientId = 1;
		unsigned int newClientId() { return nextClientId++; };

		OnHTTP onHTTP;
		OnMessage onMessage;
	};

};