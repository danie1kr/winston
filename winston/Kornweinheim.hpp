
#include "Kornweinheim.h"

#ifdef WINSTON_WITH_WEBSOCKET
// send a turnout state via websocket
void Kornweinheim::turnoutSendState(const std::string turnoutTrackId, const winston::Turnout::Direction dir)
{
    DynamicJsonDocument obj(200);
    obj["op"] = "turnoutState";
    auto data = obj.createNestedObject("data");
    data["id"] = turnoutTrackId;
    data["state"] = (int)dir;
    std::string json("");
    serializeJson(obj, json);
    webServer.broadcast(json);
}

// send a signal state via websocket
void Kornweinheim::signalSendState(const std::string trackId, const winston::Track::Connection connection, const winston::Signal::Aspects aspects)
{
    DynamicJsonDocument obj(200);
    obj["op"] = "signalState";
    auto data = obj.createNestedObject("data");
    data["parentTrack"] = trackId;
    data["guarding"] = winston::Track::ConnectionToString(connection);
    data["aspects"] = (int)aspects;
    std::string json("");
    serializeJson(obj, json);
    webServer.broadcast(json);
}

void Kornweinheim::locoSend(winston::Locomotive& loco)
{
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

    callbacks.dccDetectorCallback = [=](winston::Detector::Shared, const winston::Address address) -> winston::Result
    {
        // not used here
        return winston::Result::OK;
    };

    callbacks.nfcDetectorCallback = [=](winston::Detector::Shared, const winston::NFCAddress address) -> winston::Result
    {
        auto now = winston::hal::now();
        /*
        this->signalBox->order(winston::Command::make([track, connection, distance, address, now](const winston::TimePoint &created) -> const winston::State
            {
                // actual work as we do not want to block the receiver
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
        */
        /*
            loco = fromNFCAddress(address) 
            is now at
            at actual = track:connection - distance

            // update calculated distance and report error
            auto mismatch = updateLocoPos(loco) - acutal

            updateLocoPos(loco)
            {
                auto previous = loco.position();
                Duration timeOnTour = 0;
                auto pos = loco.drive(timeOnTour);   // updates loco position
                
                this->signalBox->order(winston::Command::make([=](const winston::TimePoint &created) -> const winston::State
                {
                    railway -> updateBlockOccupancy(previous, pos, timeOnTour, loco.trainLength() = 100);
                    return winston::State::Finished;
                }, __PRETTY_FUNCTION__));

                this->signalBox->order(winston::Command::make([signalBox](const winston::TimePoint &created) -> const winston::State
                {
                    signalbox -> updateSignalsAccordingToBlocks();
                    return winston::State::Finished;
                }, __PRETTY_FUNCTION__));
            }

            loco.drive(Duration &timeOnTour)
            {
                timeOnTour = inMilliseconds(this->lastUpdate - winston::hal::now());
                this->pos = this->pos + this->physicalSpeed() * timeOnTour;
                return this->pos;
            }
        */
        return winston::Result::OK;
    };

    return callbacks;
}

#ifdef WINSTON_WITH_WEBSOCKET
// Define a callback to handle incoming messages

void Kornweinheim::writeSignal(WebServer::HTTPConnection& connection, const winston::Track::Shared track, const winston::Track::Connection trackCon)
{
    if (auto signal = track->signalGuarding(trackCon))
    {
        unsigned int l = 0;
        size_t cnt = signal->lights().size();
        for (const auto& light : signal->lights())
        {
            std::string icon = "";
            switch (light.aspect)
            {
            default:
            case winston::Signal::Aspect::Off: icon = " &#9679;"; break;
            case winston::Signal::Aspect::Halt: icon = " &#128308;"; break;
            case winston::Signal::Aspect::ExpectHalt: icon = " &#128997;"; break;
            case winston::Signal::Aspect::Go: icon = " &#128994;"; break;
            case winston::Signal::Aspect::ExpectGo: icon = " &#129001;"; break;
            }

            icon += signal->shows(light.aspect) ? " on" : " off";

            if (l == 0)
                connection.body(winston::build("<tr><td rowspan=", cnt, ">", track->name(), "</td><td rowspan=", cnt, ">", winston::Track::ConnectionToString(trackCon), "</td><td>"));
            else
                connection.body(winston::build("<tr><td>"));
            ++l;
            connection.body(winston::build(l, icon, "</td><td>", light.port, "</td><td>", light.port, "</td></tr>"));
        }
    }
}

void Kornweinheim::on_http(WebServer::HTTPConnection& connection, const std::string& resource) {
    const std::string path_index("/");
    const std::string path_railway("/railway");
    const std::string path_log("/log");
    const std::string path_signals("/signals");

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
    else if (resource.compare(path_signals) == 0)
    {
        connection.status(200);
        connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
        connection.header("Connection"_s, "close"_s);
        connection.body("<html><head>winston signal list</head><body><table border=1><tr><th>track</th><th>connection</th><th>light</th><th>port</th></tr>"_s);
        for (unsigned int i = 0; i < railway->tracksCount(); ++i)
        {
            auto track = railway->track(i);
            switch (track->type())
            {
            case winston::Track::Type::Bumper:
                this->writeSignal(connection, track, winston::Track::Connection::DeadEnd);
                this->writeSignal(connection, track, winston::Track::Connection::A);
            case winston::Track::Type::Rail:
                this->writeSignal(connection, track, winston::Track::Connection::A);
                this->writeSignal(connection, track, winston::Track::Connection::B);
                break;
            default:
                break;
            }
        }
        connection.body("</table></body></html>\r\n"_s);
    }
}

void Kornweinheim::writeAttachedSignal(JsonArray& signals, winston::Track::Shared track, const winston::Track::Connection connection)
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

// Define a callback to handle incoming messages
void Kornweinheim::on_message(WebServer::Client& client, const std::string& message)
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
        std::string id = data["id"];
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
        std::string id = data["id"];
        auto turnout = std::static_pointer_cast<winston::Turnout>(railway->track(id));
        this->turnoutSendState(turnout->name(), turnout->direction());
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

                    writeAttachedSignal(signals, bumper, winston::Track::Connection::A);

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

                    writeAttachedSignal(signals, rail, winston::Track::Connection::A);
                    writeAttachedSignal(signals, rail, winston::Track::Connection::B);

                    break;
                }
                case winston::Track::Type::Turnout:
                {
                    winston::Turnout::Shared turnout = std::static_pointer_cast<winston::Turnout>(track);
                    winston::Track::Shared a, b, c;
                    turnout->connections(a, b, c);

                    auto track = tracks.createNestedObject();
                    track["a"] = a->name();
                    track["b"] = b->name();
                    track["c"] = c->name();
                    track["name"] = turnout->name();

                    writeAttachedSignal(signals, turnout, winston::Track::Connection::A);
                    writeAttachedSignal(signals, turnout, winston::Track::Connection::B);
                    writeAttachedSignal(signals, turnout, winston::Track::Connection::C);
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

        for(size_t detector = 0; detector < this->nfcDetectors.size(); ++detector)
        {
            auto d = detectors.createNestedObject();
            d["id"] = (int)detector;
            d["track"] = this->nfcDetectors[detector]->position().trackName();
            d["connection"] = winston::Track::ConnectionToString(this->nfcDetectors[detector]->position().connection());
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
            //Serial.write(json.c_str());
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
        this->storageLayout->read(address, data, 4);
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
        unsigned int id = data["id"];
        winston::Address address = data["address"];

        auto loco = this->locoFromAddress(address);
        if (loco)
            this->detectorUpdate(this->nfcDetectors[id], *loco);
        else
            winston::logger.err(winston::build("error: locomotive ", address, " not in shed"));
        /*
        unsigned int block = (unsigned int)data["block"].toInt();
        unsigned int loco = (unsigned int)data["loco"].toInt();
        // this->stationDebugInjector->injectBlockUpdate(block, loco);
        */
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
    this->signalInterfaceDevice = SignalInterfaceDevice::make(10, 20000000);
    auto TLC5947Off = Arduino_GPIOOutputPin::make(41, Arduino_GPIOOutputPin::State::High);
#else
    this->signalInterfaceDevice = SignalInterfaceDevice::make(3, 20000000);
    auto TLC5947Off = this->signalInterfaceDevice->getOutputPinDevice(4);
#endif
    this->signalInterfaceDevice->init();
    this->signalInterfaceDevice->skipSend(true);
    this->signalDevice = TLC5947_SignalDevice::make(24, this->signalInterfaceDevice, TLC5947Off);

    // storage
    this->storageLayout = Storage::make(std::string(this->name()).append(".").append("winston.storage"));
    if (this->storageLayout->init() != winston::Result::OK)
        winston::logger.err("Kornweinheim.init: Storage Layout Init failed");

    // detectors
#ifdef WINSTON_PLATFORM_TEENSY

#elif defined(WINSTON_PLATFORM_WIN_x64)
    this->serial = SerialDeviceWin::make();
    this->serial->init(5);
#endif
};


void Kornweinheim::setupSignals()
{
    auto signalUpdateCallback = [=](winston::Track::Shared track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
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

    // H
    unsigned int port = 0;
    this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::PBF1), winston::Track::Connection::B, 5U, port, signalUpdateCallback);
    this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::PBF2), winston::Track::Connection::B, 5U, port, signalUpdateCallback);
    this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::PBF3), winston::Track::Connection::B, 5U, port, signalUpdateCallback);
    /*this->railway->track(Y2021RailwayTracks::PBF1)->attachSignal(winston::SignalH::make([=](const winston::Signal::Aspects aspect)->const winston::State {
        return signalUpdateCallback(track, connection, aspect);
        }, 5U, port), winston::Track::Connection::B);
    port += winston::SignalH::lightsCount();
    this->railway->track(Y2021RailwayTracks::PBF2)->attachSignal(winston::SignalH::make(signalUpdateCallback, 5U, port), winston::Track::Connection::B);
    port += winston::SignalH::lightsCount();
    this->railway->track(Y2021RailwayTracks::PBF3)->attachSignal(winston::SignalH::make(signalUpdateCallback, 5U, port), winston::Track::Connection::B);
    port += winston::SignalH::lightsCount();*/

    // dummy
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::PBF1a), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::B3), winston::Track::Connection::B, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::B6), winston::Track::Connection::B, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::N1), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::N2), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF1), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF3a), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF3b), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF3b), winston::Track::Connection::B, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF2a), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF2b), winston::Track::Connection::A, 5U, port, signalUpdateCallback);
    port = 19; this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF2b), winston::Track::Connection::B, 5U, port, signalUpdateCallback);

    
/*
    this->railway->track(Y2021RailwayTracks::PBF1a)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::B3)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::B);
    this->railway->track(Y2021RailwayTracks::B6)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::B);
    this->railway->track(Y2021RailwayTracks::N1)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::N2)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::GBF1)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::GBF3a)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::GBF3b)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::GBF3b)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::B);
    this->railway->track(Y2021RailwayTracks::GBF2a)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::GBF2b)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::A);
    this->railway->track(Y2021RailwayTracks::GBF2b)->attachSignal(winston::SignalKS::make(signalUpdateCallback, 5U, 19), winston::Track::Connection::B);
*/
}

winston::Result Kornweinheim::detectorUpdate(winston::Detector::Shared detector, winston::Locomotive& loco)
{
    // theoretical delays:
    /*
    	speed					
	km/h	    25	     60	     80	    120	    160
	m/s	         6,94	 16,67	 22,22	 33,33	 44,44
	m/s @ 1:87	 0,08	  0,19	  0,26	  0,38	  0,51
	mm/s	    79,82	191,57	255,43	383,14	510,86
d	1	         0,08	  0,19	  0,26	  0,38	  0,51
e	2	         0,16	  0,38	  0,51	  0,77	  1,02
l	4	         0,32	  0,77	  1,02	  1,53	  2,04
a	8	         0,64	  1,53	  2,04	  3,07	  4,09
y	16	         1,28	  3,07	  4,09	  6,13	  8,17
ms	32	         2,55	  6,13	  8,17	 12,26	 16,35
    speed in mm/s * delay in ms / 1000 = overshoot
    */
    auto now = winston::hal::now(); // - offset (=2 ms Card->IC, +1ms IC->Teensy, +5ms Teensy->Teensy == 8ms <=> 2.04mm @ scaled 0.255 m/s @ 22.2m/s = 80 km/h
    // actual work as we do not want to block the receiver
    this->signalBox->order(winston::Command::make([detector, loco, now](const winston::TimePoint& created) -> const winston::State
        {
            // loco is now at
            auto current = detector->position();
           // auto deviation = loco.drive() - current;


            return winston::State::Finished;
        }, __PRETTY_FUNCTION__));
    /*
        loco = fromNFCAddress(address)
        is now at
        at actual = track:connection - distance

        // update calculated distance and report error
        auto mismatch = updateLocoPos(loco) - acutal

        updateLocoPos(loco)
        {
            auto previous = loco.position();
            Duration timeOnTour = 0;
            auto pos = loco.drive(timeOnTour);   // updates loco position

            this->signalBox->order(winston::Command::make([=](const winston::TimePoint &created) -> const winston::State
            {
                railway -> updateBlockOccupancy(previous, pos, timeOnTour, loco.trainLength() = 100);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));

            this->signalBox->order(winston::Command::make([signalBox](const winston::TimePoint &created) -> const winston::State
            {
                signalbox -> updateSignalsAccordingToBlocks();
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
        }

        loco.drive(Duration &timeOnTour)
        {
            timeOnTour = inMilliseconds(this->lastUpdate - winston::hal::now());
            this->pos = this->pos + this->physicalSpeed() * timeOnTour;
            return this->pos;
        }
    */
    return winston::Result::OK;
}

void Kornweinheim::setupDetectors()
{
    auto nfcDetectorCallback = [=](winston::Detector::Shared detector, const winston::NFCAddress address) -> winston::Result
    {
        auto loco = this->locoFromAddress(address);
        if (loco)
            return this->detectorUpdate(detector, *loco);
        else
            return winston::Result::NotFound;
    };
#ifdef WINSTON_PLATFORM_TEENSY

#elif defined(WINSTON_PLATFORM_WIN_x64)
    /*
    const auto pbf1 = this->railway->track(Y2021RailwayTracks::PBF1);
    const auto B = winston::Track::Connection::B;
    const auto R3 = winston::library::track::Roco::R3;
    this->nfcDetectors[0] = winston::NFCDetector::make(nfcDetectorCallback, pbf1, B, 2 * R3);
    this->pn532 = PN532_DetectorDevice::make(*this->nfcDetectors[0].get(), *this->serial.get());*/
    size_t i = 0;
    // teensy 1
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::B3), winston::Track::Connection::B, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::B6), winston::Track::Connection::B, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::PBF1a), winston::Track::Connection::A, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::N1), winston::Track::Connection::A, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::GBF1), winston::Track::Connection::A, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::GBF2b), winston::Track::Connection::A, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::GBF3b), winston::Track::Connection::A, 0);

    // teensy2
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::B1), winston::Track::Connection::A, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::B4), winston::Track::Connection::A, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::B2), winston::Track::Connection::A, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::B2), winston::Track::Connection::B, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::B5), winston::Track::Connection::B, 0);
    this->nfcDetectors[i++] = winston::NFCDetector::make(nfcDetectorCallback, this->railway->track(Y2021RailwayTracks::N2), winston::Track::Connection::A, 0);
#endif
}

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
    this->signalInterfaceDevice->skipSend(false);
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
    winston::Position pos(this->railway->track(Y2021RailwayTracks::N1), winston::Track::Connection::A, 100);
    this->addLocomotive(callbacks, 3, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 114", 0);
    this->addLocomotive(callbacks, 4, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 106", 1);
    this->addLocomotive(callbacks, 5, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 64", 2);
    this->addLocomotive(callbacks, 6, pos, winston::Locomotive::defaultThrottleSpeedMap, "E 11", 3);
    this->addLocomotive(callbacks, 7, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 218", 4);
}

