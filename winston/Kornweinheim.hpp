
#include "Kornweinheim.h"

#ifdef WINSTON_WITH_WEBSOCKET
// send a turnout state via websocket
void Kornweinheim::turnoutSendState(const std::string turnoutTrackId, const winston::Turnout::Direction dir)
{
#ifdef WINSTON_JSON_11
    Json obj = Json::object{
        { "op", "turnoutState" },
        { "data", Json::object{
            { "id", turnoutTrackId },
            { "state", (int)dir }
            }
        }
    };
    webServer.broadcast(obj.dump());
#elif defined(WINSTON_JSON_ARDUINO)
    DynamicJsonDocument obj(200);
    obj["op"] = "turnoutState";
    auto data = obj.createNestedObject("data");
    data["id"] = turnoutTrackId;
    data["state"] = (int)dir;
    std::string json("");
    serializeJson(obj, json);
    webServer.broadcast(json);
#endif
}

// send a signal state via websocket
void Kornweinheim::signalSendState(const std::string trackId, const winston::Track::Connection connection, const winston::Signal::Aspects aspects)
{
#ifdef WINSTON_JSON_11
    Json obj = Json::object{
        { "op", "signalState" },
        { "data", Json::object {
            { "parentTrack", trackId },
            { "guarding", winston::Track::ConnectionToString(connection) },
            { "aspects", (int)aspects }
            }
        }
    };
    webServer.broadcast(obj.dump());
#elif defined(WINSTON_JSON_ARDUINO)
    DynamicJsonDocument obj(200);
    obj["op"] = "signalState";
    auto data = obj.createNestedObject("data");
    data["parentTrack"] = trackId;
    data["guarding"] = winston::Track::ConnectionToString(connection);
    data["aspects"] = (int)aspects;
    std::string json("");
    serializeJson(obj, json);
    webServer.broadcast(json);
#endif
}

void Kornweinheim::locoSend(winston::Locomotive& loco)
{
#ifdef WINSTON_JSON_11
    Json obj = Json::object{
        { "op", "loco" },
        { "data", Json::object {
            { "address", loco.address() },
            { "name", loco.name().c_str() },
            { "light", loco.light() },
            { "forward", loco.forward() },
            { "speed", loco.speed() }
        }
        }
    };
    webServer.broadcast(obj.dump());
#elif defined(WINSTON_JSON_ARDUINO)
    DynamicJsonDocument obj(1024);
    obj["op"] = "loco";
    auto data = obj.createNestedObject("data");
    data["address"] = loco.address();
    data["name"] = loco.name().c_str();
    data["light"] = loco.light();
    data["forward"] = loco.forward();
    data["speed"] = loco.speed();
    std::string json("");
    serializeJson(obj, json);
    webServer.broadcast(json);
#endif
}

void Kornweinheim::locoSend(winston::Address address)
{
    if (auto loco = this->locoFromAddress(address))
    {
        locoSend(*loco);
    }
}
#endif
void Kornweinheim::initNetwork()
{
    // z21
    z21Socket = UDPSocket::make(z21IP, z21Port);

#ifdef WINSTON_WITH_WEBSOCKET
    // webServer
    this->webServer.init(
        [=](WebServer::HTTPConnection& client, const std::string& resource) {
            this->on_http(client, resource);
        },
        [=](WebServer::Client& client, const std::string& resource) {
            this->on_message(client, resource);
        },
            //        std::bind(&Kornweinheim::on_http, this, std::placeholders::_1, std::placeholders::_2),
              //      std::bind(&Kornweinheim::on_message, this, std::placeholders::_1, std::placeholders::_2),
            8080);
#endif
}

winston::DigitalCentralStation::Callbacks Kornweinheim::z21Callbacks()
{
    winston::DigitalCentralStation::Callbacks callbacks;

    //
    callbacks.systemInfoCallback = [=](const size_t id, const std::string name, const std::string content) {
        winston::logger.log(winston::build("Z21: ", name, ": ", content));
    };

    //
    callbacks.trackPowerStatusCallback = [=](const bool powerOn) {
        winston::logger.log(std::string("Z21: Power is ") + std::string(powerOn ? "on" : "off"));
    };

    // 
    callbacks.programmingTrackStatusCallback = [=](const bool programmingOn) {
        winston::logger.log(std::string("Z21: Programming is ") + std::string(programmingOn ? "on" : "off"));
    };

    // 
    callbacks.shortCircuitDetectedCallback = [=]() {
        winston::logger.log("Z21: Short circuit detected!");
    };

    // what to do when the digital central station updated a turnout
    callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
    {
        turnout->finalizeChangeTo(direction);
        return winston::State::Finished;
    };

    callbacks.locomotiveUpdateCallback = [=](winston::Locomotive& loco, bool busy, bool  forward, unsigned char  speed, uint32_t functions)
    {
        loco.update(busy, forward, speed, functions);


#ifdef WINSTON_WITH_WEBSOCKET
        locoSend(loco);
#endif    
    };

    return callbacks;
}

winston::Locomotive::Callbacks Kornweinheim::locoCallbacks()
{
    winston::Locomotive::Callbacks callbacks;

    callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward) {
        this->signalBox->order(winston::Command::make([this, address, speed, forward](const winston::TimePoint &created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoDrive(address, speed, forward);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
    };

    callbacks.functions = [=](const winston::Address address, const uint32_t functions) {
        this->signalBox->order(winston::Command::make([this, address, functions](const winston::TimePoint &created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoFunction(address, functions);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
    };

    return callbacks;
}

winston::Railway::Callbacks Kornweinheim::railwayCallbacks()
{
    winston::Railway::Callbacks callbacks;

    callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
    {
        // tell the signal box to update the signals
        this->signalBox->setSignalsFor(turnout);

#ifdef WINSTON_WITH_WEBSOCKET
        // tell the ui what happens
        turnoutSendState(turnout->name(), direction);
#endif
        winston::logger.info("Turnout ", turnout->name(), " set to direction ", winston::Turnout::DirectionToString(direction));

        return winston::State::Finished;
    };

    callbacks.signalUpdateCallback = [=](winston::Track::Shared track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
    {
#ifdef WINSTON_WITH_WEBSOCKET
        // send to web socket server
        signalSendState(track->name(), connection, aspects);
#endif

        // update physical light
        this->signalDevice->update(track->signalGuarding(connection));
        winston::logger.info("Signal at ", track->name(), "|", winston::Track::ConnectionToString(connection), " set to ", aspects);

        return winston::State::Finished;
    };

    return callbacks;
}

#ifdef WINSTON_WITH_WEBSOCKET
// Define a callback to handle incoming messages
void Kornweinheim::on_http(WebServer::HTTPConnection& connection, const std::string& resource) {
    const std::string path_index("/");
    const std::string path_railway("/railway");
    const std::string path_log("/log");

    if (resource.compare(path_index) == 0)
    {
        connection.status(200);
        connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
        connection.header("Connection"_s, "close"_s);
        connection.body("<html>winston</html>\r\n"_s);
    }
    else if (resource.compare(path_railway) == 0)
    {
        connection.status(200);
        connection.header("content-type"_s, "application/json; charset=UTF-8"_s);
        connection.header("Connection"_s, "close"_s);
        connection.body("<html>winston</html>\r\n"_s);
    }
    else if (resource.compare(path_log) == 0)
    {
        connection.status(200);
        connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
        connection.header("Connection"_s, "close"_s);
        connection.body("<html><head>winston</head><body><table><tr><th>timestamp</th><th>level</th><th>log</th></tr>"_s);
        for (const auto& entry : winston::logger.entries())
        {
            connection.body("<tr><td>");
            connection.body(winston::build(entry.timestamp)); connection.body("</td><td>");
            connection.body(entry.levelName()); connection.body("</td><td>");
            connection.body(entry.text); connection.body("</td></tr>\r\n");
        }

        connection.body("</table></body></html>\r\n"_s);
    }
}

void Kornweinheim::writeAttachedSignal(
#ifdef WINSTON_JSON_11
    Json::array& signals,
#elif defined(WINSTON_JSON_ARDUINO)
    JsonArray& signals,
#endif
    winston::Track::Shared track, const winston::Track::Connection connection)
{
    auto signal = track->signalGuarding(connection);
#ifdef WINSTON_JSON_11
    if (signal)
        signals.push_back(Json::object{
            { "parentTrack", track->name()} ,
            { "guarding", winston::Track::ConnectionToString(connection)},
            { "pre", signal->preSignal()},
            { "main", signal->mainSignal() }
            });
#elif defined(WINSTON_JSON_ARDUINO)
    if (signal)
    {
        auto data = signals.createNestedObject();
        data["parentTrack"] = track->name();
        data["guarding"] = winston::Track::ConnectionToString(connection);
        data["pre"] = signal->preSignal();
        data["main"] = signal->mainSignal();
    }
#endif
}

// Define a callback to handle incoming messages
void Kornweinheim::on_message(WebServer::Client& client, const std::string& message) {
#ifdef WINSTON_JSON_11
    std::string jsonParseError;
    Json m = Json::parse(message, jsonParseError);
    std::string op = m["op"].dump();
    Json data = m["data"];
#elif defined(WINSTON_JSON_ARDUINO)
    DynamicJsonDocument msg(32 * 1024);
    deserializeJson(msg, message);
    JsonObject obj = msg.as<JsonObject>();
    std::string op("\"");
    op.append(obj["op"].as<std::string>());
    op.append("\"");
    JsonObject data = obj["data"];
#endif

    if (std::string("\"doTurnoutToggle\"").compare(op) == 0)
    {
#ifdef WINSTON_JSON_11
        auto id = data["id"].string_value();
#elif defined(WINSTON_JSON_ARDUINO)
        std::string id = data["id"];
#endif
        auto turnout = std::static_pointer_cast<winston::Turnout>(railway->track(id));
        auto requestDir = winston::Turnout::otherDirection(turnout->direction());
        signalBox->order(winston::Command::make([this, id, turnout, requestDir](const winston::TimePoint &created) -> const winston::State
            {
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
                signalBox->order(winston::Command::make([this, turnout, requestDir](const winston::TimePoint &created) -> const winston::State
                    {
                        if (inMilliseconds((winston::hal::now() - created)) > WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
                        {
                            this->stationDebugInjector->injectTurnoutUpdate(turnout, requestDir);
                            return winston::State::Finished;
                        }
                        return winston::State::Delay;
                        }, __PRETTY_FUNCTION__));
#endif
                // tell the central station to trigger the turnout switch
                // update internal representation. will inform the UI in its callback, too
                return this->turnoutChangeTo(turnout, requestDir);
            }, __PRETTY_FUNCTION__));
    }
    else if (std::string("\"getTurnoutState\"").compare(op) == 0)
    {
#ifdef WINSTON_JSON_11
        auto id = data["id"].string_value();
#elif defined(WINSTON_JSON_ARDUINO)
        std::string id = data["id"];
#endif
        auto turnout = std::static_pointer_cast<winston::Turnout>(railway->track(id));
        this->turnoutSendState(turnout->name(), turnout->direction());
    }
    else if (std::string("\"getSignalState\"").compare(op) == 0)
    {
#ifdef WINSTON_JSON_11
        auto id = data["parentTrack"].string_value();
        auto guarding = data["guarding"].string_value();
#elif defined(WINSTON_JSON_ARDUINO)
        std::string id = data["parentTrack"];
        std::string guarding = data["guarding"];
#endif

        auto connection = winston::Track::ConnectionFromString(guarding);
        auto signal = this->railway->track(id)->signalGuarding(connection);

        this->signalSendState(id, connection, signal->aspect());
    }
    else if (std::string("\"getRailway\"").compare(op) == 0)
    {
#ifdef WINSTON_JSON_11
        auto tracks = Json::array();
        auto signals = Json::array();
        auto blocks = Json::array();
#elif defined(WINSTON_JSON_ARDUINO)
        DynamicJsonDocument railwayContent(32 * 1024);
        auto tracks = railwayContent.createNestedArray("tracks");
        auto signals = railwayContent.createNestedArray("signals");
        auto blocks = railwayContent.createNestedArray("blocks");
#endif

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

#ifdef WINSTON_JSON_11
                Json::object track = Json::object();
                track["a"] = a->name();
                track["name"] = bumper->name();
                tracks.push_back(track);
#elif defined(WINSTON_JSON_ARDUINO)
                auto track = tracks.createNestedObject();
                track["a"] = a->name();
                track["name"] = bumper->name();
#endif

                writeAttachedSignal(signals, bumper, winston::Track::Connection::A);

                break;
            }
            case winston::Track::Type::Rail:
            {
                winston::Rail::Shared rail = std::static_pointer_cast<winston::Rail>(track);
                winston::Track::Shared a, b;
                rail->connections(a, b);

#ifdef WINSTON_JSON_11
                Json::object track = Json::object();
                track["a"] = a->name();
                track["b"] = b->name();
                track["name"] = rail->name();
                tracks.push_back(track);
#elif defined(WINSTON_JSON_ARDUINO)
                auto track = tracks.createNestedObject();
                track["a"] = a->name();
                track["b"] = b->name();
                track["name"] = rail->name();
#endif

                writeAttachedSignal(signals, rail, winston::Track::Connection::A);
                writeAttachedSignal(signals, rail, winston::Track::Connection::B);

                break;
            }
            case winston::Track::Type::Turnout:
            {
                winston::Turnout::Shared turnout = std::static_pointer_cast<winston::Turnout>(track);
                winston::Track::Shared a, b, c;
                turnout->connections(a, b, c);

#ifdef WINSTON_JSON_11
                Json::object track = Json::object();
                track["a"] = a->name();
                track["b"] = b->name();
                track["c"] = c->name();
                track["name"] = turnout->name();
                tracks.push_back(track);
#elif defined(WINSTON_JSON_ARDUINO)
                auto track = tracks.createNestedObject();
                track["a"] = a->name();
                track["b"] = b->name();
                track["c"] = c->name();
                track["name"] = turnout->name();
#endif

                writeAttachedSignal(signals, turnout, winston::Track::Connection::A);
                writeAttachedSignal(signals, turnout, winston::Track::Connection::B);
                writeAttachedSignal(signals, turnout, winston::Track::Connection::C);
                break;
            }
            }
        }

        for (auto& block : this->railway->blocks())
        {
#ifdef WINSTON_JSON_11
            Json::object b = Json::object();
            b["address"] = block.first;
            auto bl = block.second;

            Json::array blockTracks = Json::array();
            for (auto& track : bl->tracks())
                blockTracks.push_back(track->name());
            b["tracks"] = blockTracks;
            blocks.push_back(b);
#elif defined(WINSTON_JSON_ARDUINO)
            auto b = blocks.createNestedObject();
            b["address"] = block.first;
            auto bl = block.second;

            auto blockTracks = b.createNestedArray("tracks");
            for (auto& track : bl->tracks())
                blockTracks.add(track->name());
#endif
        }

#ifdef WINSTON_JSON_11
        Json railwayMessage = Json::object({
            {"op", "railway"},
            {"data", Json::object{
                {"tracks", tracks},
                {"signals", signals},
                {"blocks", blocks},
                }
            }
            });

        this->webServer.send(client, railwayMessage.dump());
#elif defined(WINSTON_JSON_ARDUINO)
        std::string railwayContentJson("");
        serializeJson(railwayContent, railwayContentJson);

        const size_t chunkSize = 512;// this->webServer.maxMessageSize();
        
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
            //Serial.write(json.c_str());
            this->webServer.send(client, json);
        }
#endif
    }
    else if (std::string("\"storeRailwayLayout\"").compare(op) == 0)
    {
#ifdef WINSTON_JSON_11
        auto layout = data["layout"].string_value();
        auto offset = (size_t)data["offset"].int_value();
        auto fullSize = (size_t)data["fullSize"].int_value();
#elif defined(WINSTON_JSON_ARDUINO)
        std::string layout = data["layout"];
        size_t offset = (size_t)((unsigned int)data["offset"]);
        size_t fullSize = (size_t)((unsigned int)data["fullSize"]);
#endif
        size_t address = 0;
        auto length = layout.size();
        if (offset == 0)
        {
            winston::hal::storageWrite(address + 0, (fullSize >> 0) & 0xFF);
            winston::hal::storageWrite(address + 1, (fullSize >> 8) & 0xFF);
            winston::hal::storageWrite(address + 2, (fullSize >> 16) & 0xFF);
            winston::hal::storageWrite(address + 3, (fullSize >> 24) & 0xFF);
            address = 4;
        }
        else
        {
            address = 4 + offset;
        }
        for (auto s : layout)
            winston::hal::storageWrite(address++, s);

        winston::hal::storageCommit();

        if (offset == fullSize - length)
        {
#ifdef WINSTON_JSON_11
            Json successObject = Json::object{
                {"op", "storeRailwayLayoutSuccessful"},
                {"data", true}
            };
            this->webServer.send(client, successObject.dump());
#elif defined(WINSTON_JSON_ARDUINO)
            DynamicJsonDocument obj(200);
            obj["op"] = "storeRailwayLayoutSuccessful";
            obj["data"] = true;
            std::string json("");
            serializeJson(obj, json);
            this->webServer.send(client, json);
#endif
        }
    }
    else if (std::string("\"getRailwayLayout\"").compare(op) == 0)
    {
        size_t address = 0;
        size_t length = (winston::hal::storageRead(address + 0) << 0) |
            (winston::hal::storageRead(address + 1) << 8) |
            (winston::hal::storageRead(address + 2) << 16) |
            (winston::hal::storageRead(address + 3) << 24);
        address = 4;

        const size_t sizePerMessage = size_t(0.7f * webServer.maxMessageSize());
        size_t remaining = length;
        size_t offset = 0;

        while (remaining > 0)
        {
            size_t sent = remaining > sizePerMessage ? sizePerMessage : remaining;
            auto layout = std::string(sent, '0');

            for (size_t i = 0; i < sent; ++i)
                layout[i] = winston::hal::storageRead(address + offset + i);
#ifdef WINSTON_JSON_11
            Json successObject = Json::object{
                {"op", "layout"},
                {"data", Json::object{
                    {"offset", (int)offset},
                    {"fullSize", (int)length},
                    {"layout", layout}
                }}
            };
            this->webServer.send(client, successObject.dump());
#elif defined(WINSTON_JSON_ARDUINO)
            DynamicJsonDocument obj(sizePerMessage + 1024);
            obj["op"] = "layout";
            auto data = obj.createNestedObject("data");
            data["offset"] = (int)offset;
            data["fullSize"] = (int)length;
            data["layout"] = layout;
            std::string json("");
            serializeJson(obj, json);
            this->webServer.send(client, json);
#endif

            /*this->webServer.send(client, successObject.dump());
            JSON successObject = JSON::Make(JSON::Class::Object);
            successObject["op"] = "layout";
            auto data = JSON::Make(JSON::Class::Object);
            data["offset"] = (unsigned int)offset;
            data["fullSize"] = (unsigned int)length;
            data["layout"] = layout.c_str();
            successObject["data"] = data;*/

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
    else if (op.find(std::string("\"emu_z21_inject\"")) == 0)
    {
        if (std::string("\"emu_z21_inject_occupied\"").compare(op) == 0)
        {
            /*
            unsigned int block = (unsigned int)data["block"].toInt();
            unsigned int loco = (unsigned int)data["loco"].toInt();
            // this->stationDebugInjector->injectBlockUpdate(block, loco);
            */
        }
    }
#endif
    else
    {
        winston::hal::text("Received unknown message: ");
        winston::hal::text(message);
    }
}
#endif

// setup our model railway system
void Kornweinheim::systemSetup() {
    this->initNetwork();

    // the user defined railway and its address translator
    this->railway = RAILWAY_CLASS::make(railwayCallbacks());
    this->addressTranslator = RAILWAY_CLASS::AddressTranslator::make(railway);

    // the internal signal box
    this->signalBox = winston::SignalBox::make();

    // the system specific digital central station
    auto at = std::static_pointer_cast<winston::DigitalCentralStation::TurnoutAddressTranslator>(addressTranslator);
    auto udp = std::static_pointer_cast<winston::hal::UDPSocket>(this->z21Socket);
    this->digitalCentralStation = Z21::make(udp, at, *this, this->signalBox, z21Callbacks());

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    // a debug injector
    auto dcs = std::static_pointer_cast<winston::DigitalCentralStation>(this->digitalCentralStation);
    this->stationDebugInjector = winston::DigitalCentralStation::DebugInjector::make(dcs);
#endif

    // signals
#ifdef WINSTON_PLATFORM_TEENSY
    this->signalSPIDevice = SignalSPIDevice::make(10, 20000000);
#else
    this->signalSPIDevice = SignalSPIDevice::make(3, 20000000);
#endif
    this->signalSPIDevice->init();
    this->signalSPIDevice->skipSend(true);
    this->signalDevice = TLC5947_SignalDevice::make(1, 24, this->signalSPIDevice);
};

void Kornweinheim::systemSetupComplete()
{
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    //for (auto& kv : this->railway->turnouts())
    //auto turnouts = this->railway->turnouts();
    this->railway->turnouts([=](const Tracks track, winston::Turnout::Shared turnout) {
        this->stationDebugInjector->injectTurnoutUpdate(turnout, std::rand() % 2 ? winston::Turnout::Direction::A_B : winston::Turnout::Direction::A_C);
        });
    //for (auto it = turnouts.begin(); it != turnouts.end(); it++)
    //    this->stationDebugInjector->injectTurnoutUpdate(it->second, std::rand() % 2 ? winston::Turnout::Direction::A_B : winston::Turnout::Direction::A_C);
#endif
    this->signalSPIDevice->skipSend(false);
    this->signalDevice->flush();
    this->signalBox->order(this->signalDevice->flushCommand());
}

// accept new requests and loop over what the signal box has to do
bool Kornweinheim::systemLoop()
{
#ifdef WINSTON_WITH_WEBSOCKET
    {

#ifdef WINSTON_STATISTICS
        winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "webServer");
#endif
        this->webServer.step();
    }
#endif
#ifdef WINSTON_STATISTICS
    winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "signalBox");
#endif
    return this->signalBox->work();
}

void Kornweinheim::populateLocomotiveShed()
{
    auto callbacks = locoCallbacks();
    this->addLocomotive(callbacks, 3, "BR 114");
    this->addLocomotive(callbacks, 4, "BR 106");
    this->addLocomotive(callbacks, 5, "BR 64");
    this->addLocomotive(callbacks, 6, "E 11");
    this->addLocomotive(callbacks, 7, "BR 218");
}

