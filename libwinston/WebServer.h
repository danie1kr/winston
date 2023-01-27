#pragma once

#include <string>
#include <map>
#include <functional>
#include "external/better_enum.hpp"
#include "Railway.h"
#include "Util.h"

#ifdef __GNUC__ 
#pragma GCC push_options
#pragma GCC optimize("Os")
#endif
#define ARDUINOJSON_ENABLE_STD_STRING 1
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#include "external/ArduinoJson-v6.19.0.hpp"
#ifdef __GNUC__ 
#pragma GCC pop_options
#endif
using namespace ArduinoJson;

namespace winston
{
	BETTER_ENUM(HTTPMethod, unsigned char, GET, PUT);

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
            virtual bool body(const unsigned char* content, size_t length, size_t chunked) = 0;
		};

		using OnHTTP = std::function<Result(HTTPConnection &client, const HTTPMethod method, const std::string &resource)>;
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

	template<class _WebServer, class _Railway, class _Storage>
	class WebUI
	{
    public:
        using TurnoutToggleCallback = std::function<Result(const std::string id)>;
        using RouteSetCallback = std::function<Result(const int id, const bool set)>;
        using LocoControlCallback = std::function<Result()>;
    private:
        TurnoutToggleCallback turnoutToggle;
        LocoControlCallback locoControl;
        RouteSetCallback routeSet;
        typename _Railway::Shared railway;
        LocomotiveShed locomotiveShed;
        typename _Railway::AddressTranslator::Shared addressTranslator;
        typename _Storage::Shared storageLayout;
        DigitalCentralStation::Shared digitalCentralStation;
	public:

        WebUI()
        {

        }

        inline void step()
        {
            this->webServer.step();
        }

		Result init(typename _Railway::Shared railway, LocomotiveShed locomotiveShed, typename _Storage::Shared storageLayout, typename _Railway::AddressTranslator::Shared addressTranslator, DigitalCentralStation::Shared digitalCentralStation, const unsigned int port, typename _WebServer::OnHTTP onHTTP, TurnoutToggleCallback turnoutToggle, RouteSetCallback routeSet)
		{
            this->railway = railway;
            this->locomotiveShed = locomotiveShed;
            this->turnoutToggle = turnoutToggle;
            this->routeSet = routeSet;
            this->storageLayout = storageLayout;
            this->addressTranslator = addressTranslator;
            this->digitalCentralStation = digitalCentralStation;
            //this->locoControl = locoControl;

			// webServer
			this->webServer.init(
				[=](typename _WebServer::HTTPConnection& client, const winston::HTTPMethod method, const std::string& resource) -> Result {
					auto result = this->on_http(client, method, resource);
					if(result == Result::NotFound)
						return onHTTP(client, method, resource);
					return result;
				},
				[=](typename _WebServer::Client& client, const std::string& resource) {
					this->on_message(client, resource);
				},
				port);

            return Result::OK;
		}

    private:
        void turnoutSendState(const std::string turnoutTrackId, const int dir)
        {
            DynamicJsonDocument obj(200);
            obj["op"] = "turnoutState";
            auto data = obj.createNestedObject("data");
            data["id"] = turnoutTrackId;
            data["state"] = (int)dir;
            std::string json("");
            serializeJson(obj, json);
            this->webServer.broadcast(json);
        }
    public:
        // send a turnout state via websocket
        void turnoutSendState(const std::string turnoutTrackId, const winston::Turnout::Direction dir)
        {
            turnoutSendState(turnoutTrackId, (int)dir);
        }

        // send a double slip state via websocket
        void doubleSlipSendState(const std::string turnoutTrackId, const winston::DoubleSlipTurnout::Direction dir)
        {
            turnoutSendState(turnoutTrackId, (int)dir);
        }

        // send route state via websocket
        void routeState(const Route &route, const Route::State state)
        {
            DynamicJsonDocument obj(200);
            obj["op"] = "routeState";
            auto data = obj.createNestedObject("data");
            data["id"] = route.id;
            data["state"] = (int)state;
            std::string json("");
            serializeJson(obj, json);
            this->webServer.broadcast(json);
        }

		// send a signal state via websocket
		void signalSendState(const std::string trackId, const winston::Track::Connection connection, const winston::Signal::Aspects aspects)
		{
			DynamicJsonDocument obj(200);
			obj["op"] = "signalState";
			auto data = obj.createNestedObject("data");
			data["parentTrack"] = trackId;
			data["guarding"] = winston::Track::ConnectionToString(connection);
			data["aspects"] = (int)aspects;
			std::string json("");
			serializeJson(obj, json);
			this->webServer.broadcast(json);
		}

		void locoSend(winston::Locomotive::Shared loco)
		{
			DynamicJsonDocument obj(1024+256*loco->functions().size());
			obj["op"] = "loco";
			auto data = obj.createNestedObject("data");
			data["address"] = loco->address();
			data["name"] = loco->name();
			data["light"] = loco->light();
			data["forward"] = loco->forward();
			data["speed"] = loco->speed();
            auto functions = data.createNestedArray("functions");
            for (auto const & function : loco->functions())
            {
                auto func = functions.createNestedObject();
                func["id"] = function.id;
                func["name"] = function.name;
            }
			std::string json("");
			serializeJson(obj, json);
			this->webServer.broadcast(json);
		}

		void locoSend(winston::Address address)
		{
			if (auto loco = this->locoFromAddress(address))
				locoSend(*loco);
		}

        void log(const Logger::Entry &entry)
        {
            DynamicJsonDocument obj(256 + entry.text.size());
            obj["op"] = "log";
            auto data = obj.createNestedObject("data");
            data["level"] = entry.levelName();
            data["timestamp"] = inSeconds(entry.timestamp.time_since_epoch());
            data["text"] = entry.text;
            std::string json("");
            serializeJson(obj, json);
            this->webServer.broadcast(json);
        }

        void statusSend()
        {
            DynamicJsonDocument obj(256);
            obj["op"] = "status";
            auto data = obj.createNestedObject("data");
            data["connected"] = this->digitalCentralStation->connected();
            std::string json("");
            serializeJson(obj, json);
            this->webServer.broadcast(json);
        }

	private:

		// Define a callback to handle incoming messages
        Result on_http(typename _WebServer::HTTPConnection& connection, const winston::HTTPMethod method, const std::string& resource)
        {
            const std::string path_index("/");

            if (resource.compare(path_index) == 0)
            {
                connection.status(200);
                connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
                connection.header("Connection"_s, "close"_s);
                connection.body("<html>winston</html>\r\n"_s);
            }
            else
                return Result::NotFound;

            return Result::OK;
        }

		// Define a callback to handle incoming messages
		void on_message(typename _WebServer::Client& client, const std::string& message)
		{
            DynamicJsonDocument msg(32 * 1024);
            deserializeJson(msg, message);
            JsonObject obj = msg.as<JsonObject>();
            std::string op("\"");
            op.append(obj["op"].as<std::string>());
            op.append("\"");
            JsonObject data = obj["data"];

            if (std::string("\"doTurnoutToggle\"").compare(op) == 0)
            {
                const std::string id = data["id"];
                this->turnoutToggle(id);
            }
            else if (std::string("\"doRouteSet\"").compare(op) == 0)
            {
                const int id = data["id"];
                const bool set = data["set"];
                this->routeSet(id, set);
            }
            else if (std::string("\"getTurnoutState\"").compare(op) == 0)
            {
                const std::string id = data["id"];
                auto track = railway->track(id);
                if (track->type() == Track::Type::Turnout)
                {
                    auto turnout = std::static_pointer_cast<winston::Turnout>(track);
                    this->turnoutSendState(turnout->name(), turnout->direction());
                }
                else if(track->type() == Track::Type::DoubleSlipTurnout)
                {
                    auto turnout = std::static_pointer_cast<winston::DoubleSlipTurnout>(track);
                    this->doubleSlipSendState(turnout->name(), turnout->direction());
                }
            }
            else if (std::string("\"getSignalState\"").compare(op) == 0)
            {
                std::string id = data["parentTrack"];
                std::string guarding = data["guarding"];

                auto connection = winston::Track::ConnectionFromString(guarding);
                auto signal = this->railway->track(id)->signalGuarding(connection);

                this->signalSendState(id, connection, signal->aspect());
            }
            else if (std::string("\"getRailway\"").compare(op) == 0)
            {
                DynamicJsonDocument railwayContent(32 * 1024);
                auto tracks = railwayContent.createNestedArray("tracks");
                auto signals = railwayContent.createNestedArray("signals");
                auto blocks = railwayContent.createNestedArray("blocks");
                auto routes = railwayContent.createNestedArray("routes");
                auto detectors = railwayContent.createNestedArray("detectors");

                for (unsigned int i = 0; i < railway->tracksCount(); ++i)
                {
                    auto track = railway->track(i);
                    switch (track->type())
                    {
                    case winston::Track::Type::Bumper:
                    {
                        winston::Bumper::Shared bumper = std::static_pointer_cast<winston::Bumper>(track);
                        winston::Track::Shared a;
                        bumper->connections(a);

                        auto track = tracks.createNestedObject();
                        track["a"] = a->name();
                        track["name"] = bumper->name();
                        track["length"] = bumper->length();

                        writeAttachedSignal(signals, bumper, winston::Track::Connection::A);
                        writeAttachedSignal(signals, bumper, winston::Track::Connection::DeadEnd);

                        break;
                    }
                    case winston::Track::Type::Rail:
                    {
                        winston::Rail::Shared rail = std::static_pointer_cast<winston::Rail>(track);
                        winston::Track::Shared a, b;
                        rail->connections(a, b);

                        auto track = tracks.createNestedObject();
                        track["a"] = a->name();
                        track["b"] = b->name();
                        track["name"] = rail->name();
                        track["length"] = rail->length();

                        writeAttachedSignal(signals, rail, winston::Track::Connection::A);
                        writeAttachedSignal(signals, rail, winston::Track::Connection::B);

                        break;
                    }
                    case winston::Track::Type::Turnout:
                    {
                        winston::Turnout::Shared turnout = std::static_pointer_cast<winston::Turnout>(track);
                        winston::Track::Shared a, b, c;
                        turnout->connections(a, b, c);

                        auto address = this->addressTranslator->address(turnout)+1;

                        auto track = tracks.createNestedObject();
                        track["a"] = a->name();
                        track["b"] = b->name();
                        track["c"] = c->name();
                        track["name"] = turnout->name();
                        track["address"] = winston::build(address);
                        auto length = track.createNestedObject("lengths");
                        length["a_b"] = turnout->lengthOnDirection(winston::Turnout::Direction::A_B);
                        length["a_c"] = turnout->lengthOnDirection(winston::Turnout::Direction::A_C);

                        writeAttachedSignal(signals, turnout, winston::Track::Connection::A);
                        writeAttachedSignal(signals, turnout, winston::Track::Connection::B);
                        writeAttachedSignal(signals, turnout, winston::Track::Connection::C);
                        break;
                    }
                    case winston::Track::Type::DoubleSlipTurnout:
                    {
                        winston::DoubleSlipTurnout::Shared turnout = std::static_pointer_cast<winston::DoubleSlipTurnout>(track);
                        winston::Track::Shared a, b, c, d;
                        turnout->connections(a, b, c, d);
                        auto address = this->addressTranslator->address(turnout)+1;

                        auto track = tracks.createNestedObject();
                        track["a"] = a->name();
                        track["b"] = b->name();
                        track["c"] = c->name();
                        track["d"] = d->name();
                        track["name"] = turnout->name();
                        track["address"] = winston::build(address, " & ", address + 1);
                        auto length = track.createNestedObject("lengths");
                        length["a_b"] = turnout->lengthOnDirection(winston::DoubleSlipTurnout::Direction::A_B);
                        length["a_c"] = turnout->lengthOnDirection(winston::DoubleSlipTurnout::Direction::A_C);
                        length["b_d"] = turnout->lengthOnDirection(winston::DoubleSlipTurnout::Direction::B_D);
                        length["c_d"] = turnout->lengthOnDirection(winston::DoubleSlipTurnout::Direction::C_D);

                        writeAttachedSignal(signals, turnout, winston::DoubleSlipTurnout::Connection::A);
                        writeAttachedSignal(signals, turnout, winston::DoubleSlipTurnout::Connection::B);
                        writeAttachedSignal(signals, turnout, winston::DoubleSlipTurnout::Connection::C);
                        writeAttachedSignal(signals, turnout, winston::DoubleSlipTurnout::Connection::D);
                        break;
                    }
                    }
                }

                for (auto& block : this->railway->blocks())
                {
                    auto b = blocks.createNestedObject();
                    b["address"] = block.first;
                    auto bl = block.second;

                    auto blockTracks = b.createNestedArray("tracks");
                    for (auto& track : bl->tracks())
                        blockTracks.add(track->name());
                }

                if (this->railway->supportsRoutes())
                {
                    this->railway->eachRoute([=](const Route::Shared& route) {
                        auto r = routes.createNestedObject();
                        r["id"] = route->id;
                        r["name"] = route->name;
                        r["start"] = Track::ConnectionToString(route->start());
                        auto routePath = r.createNestedArray("path");
                        for (auto& path : route->path)
                        {
                            auto p = routePath.createNestedObject();
                            switch (path->type)
                            {
                            case Route::Track::Type::Track:
                                p["type"] = "Track";
                                p["track"] = path->track()->name();
                                break;
                            case Route::Track::Type::Turnout:
                                p["type"] = "Turnout";
                                p["track"] = path->track()->name();
                                p["direction"] = (int)(std::dynamic_pointer_cast<Route::Turnout>(path))->direction;
                                break;
                            case Route::Track::Type::DoubleSlipTurnout:
                                p["type"] = "DoubleSlipTurnout";
                                p["track"] = path->track()->name();
                                p["direction"] = (int)(std::dynamic_pointer_cast<Route::DoubleSlipTurnout>(path))->direction;
                                break;
                            }
                        }
                    });
                }

                std::string railwayContentJson("");
                serializeJson(railwayContent, railwayContentJson);

                const size_t chunkSize = this->webServer.maxMessageSize();

                for (size_t i = 0; i < railwayContentJson.length(); i += chunkSize)
                {
                    DynamicJsonDocument railwayMessage(chunkSize + 256);
                    railwayMessage["op"] = "railway";
                    auto data = railwayMessage.createNestedObject("data");
                    data["fullSize"] = (unsigned int)railwayContentJson.length();
                    data["offset"] = (unsigned int)i;

                    auto part = railwayContentJson.substr(i, chunkSize);
                    data["railway"] = part;

                    std::string json("");
                    serializeJson(railwayMessage, json);
                    this->webServer.send(client, json);
                }
            }
            else if (std::string("\"storeRailwayLayout\"").compare(op) == 0)
            {
                std::string layout = data["layout"];
                size_t offset = (size_t)((unsigned int)data["offset"]);
                size_t fullSize = (size_t)((unsigned int)data["fullSize"]);
                size_t address = 0;
                auto length = layout.size();
                if (offset == 0)
                {
                    this->storageLayout->write(address + 0, (fullSize >> 0) & 0xFF);
                    this->storageLayout->write(address + 1, (fullSize >> 8) & 0xFF);
                    this->storageLayout->write(address + 2, (fullSize >> 16) & 0xFF);
                    this->storageLayout->write(address + 3, (fullSize >> 24) & 0xFF);
                    address = 4;
                }
                else
                {
                    address = 4 + offset;
                }
                this->storageLayout->write(address, layout);

                this->storageLayout->sync();

                if (offset == fullSize - length)
                {
                    DynamicJsonDocument obj(200);
                    obj["op"] = "storeRailwayLayoutSuccessful";
                    obj["data"] = true;
                    std::string json("");
                    serializeJson(obj, json);
                    this->webServer.send(client, json);
                }
            }
            else if (std::string("\"getRailwayLayout\"").compare(op) == 0)
            {
                size_t address = 0;
                std::vector<unsigned char> data;
                auto result = this->storageLayout->read(address, data, 4);
                if (result != winston::Result::OK)
                {
                    winston::logger.err(winston::build("getRailwayLayout could not read layout file."));
                    return;
                }
                size_t length = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                address = 4;

                const size_t sizePerMessage = size_t(0.7f * webServer.maxMessageSize());
                size_t remaining = length;
                size_t offset = 0;

                while (remaining > 0)
                {
                    size_t sent = remaining > sizePerMessage ? sizePerMessage : remaining;
                    std::string layout;
                    this->storageLayout->read(address + offset, layout, sent);

                    DynamicJsonDocument obj(sizePerMessage + 1024);
                    obj["op"] = "layout";
                    auto data = obj.createNestedObject("data");
                    data["offset"] = (int)offset;
                    data["fullSize"] = (int)length;
                    data["layout"] = layout;
                    std::string json("");
                    serializeJson(obj, json);
                    this->webServer.send(client, json);

                    offset += sent;
                    remaining -= sent;
                }

            }
            else if (std::string("\"getLocoShed\"").compare(op) == 0)
            {
                for (auto& loco : this->locomotiveShed)
                    this->locoSend(loco);
            }
            else if (std::string("\"doControlLoco\"").compare(op) == 0)
            {
                /*
                {
                    address
                    light
                    forward
                    speed
                }
                *
                * 
                * 
                
                this->locoControl();
                
                unsigned int addr;
                bool light, forward;
                unsigned int speed;
                JVal_get(value, error, "{dbbd}", "address", &addr, "light", &light, "forward", &forward, "speed", &speed);

                if (JErr_isError(error) == false)
                {
                    winston::Address address = (uint16_t)addr;
                    if (auto loco = this->get(address))
                    {
                        unsigned char speed128 = (unsigned char)(speed & 0xFF);
                        if (loco->light() != light)
                        {
                            signalBox->order(winston::Command::make([this, loco, light](const TimePoint &created) -> const winston::State
                                {
        #ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
                                    signalBox->order(winston::Command::make([this, loco, light](const TimePoint &created) -> const winston::State
                                        {
                                            if (winston::hal::now() - created > WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
                                            {
                                                this->stationDebugInjector->injectLocoUpdate(loco, false, loco->forward(), loco->speed(), light ? 1 : 0);
                                                return winston::State::Finished;
                                            }
                                            return winston::State::Running;
                                        }, __PRETTY_FUNCTION__));
        #endif
                                    return this->locoFunction(loco->address(), light ? 1 : 0);
                                }, __PRETTY_FUNCTION__));
                        }

                        if (loco->forward() != forward || loco->speed() != speed128)
                        {
                            signalBox->order(winston::Command::make([this, loco, speed128, forward](const TimePoint &created) -> const winston::State
                                {
        #ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
                                    signalBox->order(winston::Command::make([this, loco, speed128, forward](const TimePoint &created) -> const winston::State
                                        {
                                            if (winston::hal::now() - created > WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
                                            {
                                                this->stationDebugInjector->injectLocoUpdate(loco, false, forward, speed128, loco->light() ? 1 : 0);
                                                return winston::State::Finished;
                                            }
                                            return winston::State::Running;
                                        }, __PRETTY_FUNCTION__));
        #endif
                                    return this->locoDrive(loco->address(), speed128, forward);
                                }, __PRETTY_FUNCTION__));
                        }
                    }
                }*/
            }
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
            else if (std::string("\"emu_dcs_inject_detector\"").compare(op) == 0)
            {
            /*    unsigned int id = data["id"];
                winston::Address address = data["address"];

                auto loco = this->locoFromAddress(address);
                // TODO: detector update
                //if (loco)
                //    this->detectorUpdate(this->nf[id], *loco);
                //else
                //    winston::logger.err(winston::build("error: locomotive ", address, " not in shed"));
                
                unsigned int block = (unsigned int)data["block"].toInt();
                unsigned int loco = (unsigned int)data["loco"].toInt();
                // this->stationDebugInjector->injectBlockUpdate(block, loco);
                */
            }
#endif
            else if (std::string("\"toggleDCSstop\"").compare(op) == 0)
            {
                this->digitalCentralStation->requestEmergencyStop(!this->digitalCentralStation->isEmergencyStop());
            }
            else if (std::string("\"getLogs\"").compare(op) == 0)
            {
                for (const auto& entry : winston::logger.entries())
                    this->log(entry);
            }
            else if (std::string("\"getStatus\"").compare(op) == 0)
            {
                this->statusSend();
            }
            else
            {
                winston::hal::text("Received unknown message: ");
                winston::hal::text(message);
            }
		}
		
		void writeAttachedSignal(JsonArray& signals, winston::Track::Shared track, const winston::Track::Connection connection)
		{
			auto signal = track->signalGuarding(connection);
			if (signal)
			{
				auto data = signals.createNestedObject();
				data["parentTrack"] = track->name();
				data["guarding"] = winston::Track::ConnectionToString(connection);
				data["pre"] = signal->preSignal();
				data["main"] = signal->mainSignal();
				auto lights = data.createNestedArray("lights");
				for (const auto& signalLight : signal->lights())
				{
					auto light = lights.createNestedObject();
					light["aspect"] = (unsigned int)signalLight.aspect;
					light["port"] = (unsigned int)signalLight.port;
				}
			}
		}
		
		/* websocket */
		_WebServer webServer;
	};
};