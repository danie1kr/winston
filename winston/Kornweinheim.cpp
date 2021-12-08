#include "Kornweinheim.h"


// send a turnout state via websocket
void Kornweinheim::turnoutSendState(const unsigned int turnoutTrackId, const winston::Turnout::Direction dir)
{
    JSON obj({
        "op", "turnoutState",
        "data", JSON({
            "id", turnoutTrackId,
            "state", (int)dir}
        )
        });
    webServer.broadcast(obj.ToString());
}

// send a signal state via websocket
void Kornweinheim::signalSendState(const unsigned int trackId, const winston::Track::Connection connection, const winston::Signal::Aspects aspects)
{
    JSON obj({
        "op", "signalState",
        "data", JSON({
            "parentTrack", trackId,
            "guarding", winston::Track::ConnectionToString(connection),
            "aspects", aspects
        })
        });
    webServer.broadcast(obj.ToString());
}

void Kornweinheim::locoSend(winston::Locomotive::Shared& loco)
{
    JSON obj({
        "op", "loco",
        "data", JSON({
            "address", loco->address(),
            "name", loco->name().c_str(),
            "light", loco->light(),
            "forward", loco->forward(),
            "speed", loco->speed()
        })
        });
    webServer.broadcast(obj.ToString());
}

void Kornweinheim::locoSend(winston::Address address)
{
    if (auto loco = this->get(address))
    {
        locoSend(loco);
    }
}

void Kornweinheim::initNetwork()
{
    // z21
    z21Socket = UDPSocketLWIP::make(z21IP, z21Port);

    // webServer
    this->webServer.init(
        std::bind(&Kornweinheim::on_http, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&Kornweinheim::on_message, this, std::placeholders::_1, std::placeholders::_2),
        8080);
}

winston::DigitalCentralStation::Callbacks Kornweinheim::z21Callbacks()
{
    winston::DigitalCentralStation::Callbacks callbacks;

    // what to do when the digital central station updated a turnout
    callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
    {
        //auto id = this->railway->trackIndex(turnout);
        //turnoutSendState(id, direction);
        turnout->finalizeChangeTo(direction);
        return winston::State::Finished;
    };

    callbacks.locomotiveUpdateCallback = [=](winston::Locomotive::Shared loco, bool busy, bool  forward, unsigned char  speed, uint32_t functions)
    {
        loco->update(busy, forward, speed, functions);
        locoSend(loco->address());
    };

    return callbacks;
}

winston::Locomotive::Callbacks Kornweinheim::locoCallbacks()
{
    winston::Locomotive::Callbacks callbacks;

    callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward) {
        this->signalBox->order(winston::Command::make([this, address, speed, forward](const unsigned long long& created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoDrive(address, speed, forward);
                return winston::State::Finished;
            }));
    };

    callbacks.functions = [=](const winston::Address address, const uint32_t functions) {
        this->signalBox->order(winston::Command::make([this, address, functions](const unsigned long long& created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoFunction(address, functions);
                return winston::State::Finished;
            }));
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

        // tell the ui what happens
        auto id = this->railway->trackIndex(turnout);
        turnoutSendState(id, direction);
        return winston::State::Finished;
    };

    callbacks.signalUpdateCallback = [=](winston::Track::Shared track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
    {
        // send to web socket server
        auto id = this->railway->trackIndex(track);
        signalSendState(id, connection, aspects);

        // update physical light
        this->signalDevice->update(track->signalGuarding(connection));

        return winston::State::Finished;
    };

    return callbacks;
}

// Define a callback to handle incoming messages
WebServerWSPP::HTTPResponse Kornweinheim::on_http(WebServerWSPP::Client client, std::string resource) {
    const std::string path_index("/");
    const std::string path_railway("/railway");

    const std::string header_html("\r\ncontent-type: text/html; charset=UTF-8\r\n");
    const std::string header_json("\r\ncontent-type: application/json; charset=UTF-8\r\n");
    WebServerWSPP::HTTPResponse response;
    if (resource.compare(path_index) == 0)
    {
        response.headers = { {"content-type", "text/html; charset=UTF-8"} };
        response.body = "<html>winston</html>";
    }
    else if (resource.compare(path_index) == 0)
    {
        response.headers = { {"content-type", "application/json; charset=UTF-8"} };
        response.body = "{}";
    }
    response.status = 200;

    return response;
}

void Kornweinheim::writeAttachedSignal(JSON& signals, winston::Track::Shared track, winston::Track::Connection connection)
{
    auto signal = track->signalGuarding(connection);
    if (signal)
        signals.append(JSON({
            "parentTrack", railway->trackIndex(track),
            "guarding", winston::Track::ConnectionToString(connection),
            "pre", signal->preSignal(),
            "main", signal->mainSignal() }));
}

// Define a callback to handle incoming messages
void Kornweinheim::on_message(WebServerWSPP::Client client, std::string message) {
    JSON m = JSON::Load(message);
    std::string op = m["op"].ToString();
    JSON data = m["data"];

    if (std::string("doTurnoutToggle").compare(op) == 0)
    {
        unsigned int id = (unsigned int)data["id"].ToInt();
        auto turnout = std::dynamic_pointer_cast<winston::Turnout>(railway->track(id));
        auto requestDir = winston::Turnout::otherDirection(turnout->direction());
        signalBox->order(winston::Command::make([this, id, turnout, requestDir](const unsigned long long& created) -> const winston::State
            {
#ifdef RAILWAY_DEBUG_INJECTOR
                signalBox->order(winston::Command::make([this, turnout, requestDir](const unsigned long long& created) -> const winston::State
                    {
                        if (winston::hal::now() - created > RAILWAY_DEBUG_INJECTOR_DELAY)
                        {
                            this->stationDebugInjector->injectTurnoutUpdate(turnout, requestDir);
                            return winston::State::Finished;
                        }
                        return winston::State::Running;
                    }));
#endif
                // tell the central station to trigger the turnout switch
                // update internal representation. will inform the UI in its callback, too
                return this->turnoutChangeTo(turnout, requestDir);
            }));
    }
    else if (std::string("getTurnoutState").compare(op) == 0)
    {
        unsigned int id = (unsigned int)data["id"].ToInt();
        auto turnout = std::dynamic_pointer_cast<winston::Turnout>(railway->track(id));
        this->turnoutSendState(id, turnout->direction());
    }
    else if (std::string("getSignalState").compare(op) == 0)
    {
        unsigned int id = (unsigned int)data["parentTrack"].ToInt();
        std::string guarding = data["guarding"].ToString();

        auto connection = winston::Track::ConnectionFromString(guarding);
        auto signal = this->railway->track(id)->signalGuarding(connection);

        this->signalSendState(id, connection, signal->aspect());
    }
    else if (std::string("getRailway").compare(op) == 0)
    {
        JSON railwayMessage = JSON::Make(JSON::Class::Object);
        railwayMessage["op"] = "railway";
        railwayMessage["data"] = JSON::Make(JSON::Class::Object);
        auto& tracks = railwayMessage["data"]["tracks"] = JSON::Make(JSON::Class::Array);
        auto& signals = railwayMessage["data"]["signals"] = JSON::Make(JSON::Class::Array);
        auto& blocks = railwayMessage["data"]["blocks"] = JSON::Make(JSON::Class::Array);

        for (unsigned int i = 0; i < railway->tracksCount(); ++i)
        {
            auto track = railway->track(i);
            switch (track->type())
            {
            case winston::Track::Type::Bumper:
            {
                winston::Bumper::Shared bumper = std::dynamic_pointer_cast<winston::Bumper>(track);
                winston::Track::Shared a;
                bumper->connections(a);

                JSON track = JSON::Make(JSON::Class::Object);
                track["a"] = railway->trackIndex(a);
                tracks.append(track);

                writeAttachedSignal(signals, bumper, winston::Track::Connection::A);

                break;
            }
            case winston::Track::Type::Rail:
            {
                winston::Rail::Shared rail = std::dynamic_pointer_cast<winston::Rail>(track);
                winston::Track::Shared a, b;
                rail->connections(a, b);

                JSON track = JSON::Make(JSON::Class::Object);
                track["a"] = railway->trackIndex(a);
                track["b"] = railway->trackIndex(b);
                tracks.append(track);

                writeAttachedSignal(signals, rail, winston::Track::Connection::A);
                writeAttachedSignal(signals, rail, winston::Track::Connection::B);

                break;
            }
            case winston::Track::Type::Turnout:
            {
                winston::Turnout::Shared turnout = std::dynamic_pointer_cast<winston::Turnout>(track);
                winston::Track::Shared a, b, c;
                turnout->connections(a, b, c);

                JSON track = JSON::Make(JSON::Class::Object);
                track["a"] = railway->trackIndex(a);
                track["b"] = railway->trackIndex(b);
                track["c"] = railway->trackIndex(c);
                tracks.append(track);

                writeAttachedSignal(signals, turnout, winston::Track::Connection::A);
                writeAttachedSignal(signals, turnout, winston::Track::Connection::B);
                writeAttachedSignal(signals, turnout, winston::Track::Connection::C);
                break;
            }
            }
        }

        for (auto& block : this->railway->blocks())
        {
            JSON b = JSON::Make(JSON::Class::Object);
            b["address"] = block.first;
            auto bl = block.second;

            auto& tracks = b["tracks"] = JSON::Make(JSON::Class::Array);
            for (auto& track : bl->tracks())
                tracks.append(this->railway->trackIndex(track));

            blocks.append(b);
        }

        this->webServer.send(client, railwayMessage.ToString());
    }
    else if (std::string("storeRailwayLayout").compare(op) == 0)
    {
        unsigned int address = 0;
        auto layout = data.ToUnescapedString();
        auto length = layout.size();

        winston::hal::storageWrite(address + 0, (length >> 0) & 0xFF);
        winston::hal::storageWrite(address + 1, (length >> 8) & 0xFF);
        winston::hal::storageWrite(address + 2, (length >> 16) & 0xFF);
        winston::hal::storageWrite(address + 3, (length >> 24) & 0xFF);

        address = 4;
        for (auto s : layout)
            winston::hal::storageWrite(address++, s);

        winston::hal::storageCommit();

        JSON successObject = JSON::Make(JSON::Class::Object);
        successObject["op"] = "storeRailwayLayoutSuccessful";
        successObject["data"] = true;
        this->webServer.send(client, successObject.ToString());
    }
    else if (std::string("getRailwayLayout").compare(op) == 0)
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

            JSON successObject = JSON::Make(JSON::Class::Object);
            successObject["op"] = "layout";
            auto data = JSON::Make(JSON::Class::Object);
            data["offset"] = (unsigned int)offset;
            data["fullSize"] = (unsigned int)length;
            data["layout"] = layout.c_str();
            successObject["data"] = data;
            this->webServer.send(client, successObject.ToString());

            offset += sent;
            remaining -= sent;
        }

    }
    else if (std::string("getLocoShed").compare(op) == 0)
    {
        for (auto& loco : this->locomotiveShed)
            this->locoSend(loco);
    }
    else if (std::string("doControlLoco").compare(op) == 0)
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
                    signalBox->order(winston::Command::make([this, loco, light](const unsigned long long& created) -> const winston::State
                        {
#ifdef RAILWAY_DEBUG_INJECTOR
                            signalBox->order(winston::Command::make([this, loco, light](const unsigned long long& created) -> const winston::State
                                {
                                    if (winston::hal::now() - created > RAILWAY_DEBUG_INJECTOR_DELAY)
                                    {
                                        this->stationDebugInjector->injectLocoUpdate(loco, false, loco->forward(), loco->speed(), light ? 1 : 0);
                                        return winston::State::Finished;
                                    }
                                    return winston::State::Running;
                                }));
#endif
                            return this->locoFunction(loco->address(), light ? 1 : 0);
                        }));
                }

                if (loco->forward() != forward || loco->speed() != speed128)
                {
                    signalBox->order(winston::Command::make([this, loco, speed128, forward](const unsigned long long& created) -> const winston::State
                        {
#ifdef RAILWAY_DEBUG_INJECTOR
                            signalBox->order(winston::Command::make([this, loco, speed128, forward](const unsigned long long& created) -> const winston::State
                                {
                                    if (winston::hal::now() - created > RAILWAY_DEBUG_INJECTOR_DELAY)
                                    {
                                        this->stationDebugInjector->injectLocoUpdate(loco, false, forward, speed128, loco->light() ? 1 : 0);
                                        return winston::State::Finished;
                                    }
                                    return winston::State::Running;
                                }));
#endif
                            return this->locoDrive(loco->address(), speed128, forward);
                        }));
                }
            }
        }*/
    }
#ifdef RAILWAY_DEBUG_INJECTOR
    else if (op.starts_with(std::string("emu_z21_inject")))
    {
        if (std::string("emu_z21_inject_occupied").compare(op) == 0)
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

// setup our model railway system
void Kornweinheim::systemSetup() {
    this->initNetwork();

    // the user defined railway and its address translator
    this->railway = RAILWAY_CLASS::make(railwayCallbacks());
    this->addressTranslator = RAILWAY_CLASS::AddressTranslator::make(railway);

    // the internal signal box
    this->signalBox = winston::SignalBox::make(nullMutex);

    // the system specific digital central station
    auto at = std::dynamic_pointer_cast<winston::DigitalCentralStation::AddressTranslator>(addressTranslator);
    auto udp = std::dynamic_pointer_cast<winston::hal::UDPSocket>(this->z21Socket);
    this->digitalCentralStation = Z21::make(udp, at, this->signalBox, z21Callbacks());

#ifdef RAILWAY_DEBUG_INJECTOR
    // a debug injector
    auto dcs = std::dynamic_pointer_cast<winston::DigitalCentralStation>(this->digitalCentralStation);
    this->stationDebugInjector = winston::DigitalCentralStation::DebugInjector::make(dcs);
#endif

    // signals
    this->signalSPIDevice = SignalSPIDevice::make(3, 20000000, 4);
    this->signalSPIDevice->init();
    this->signalSPIDevice->skipSend(true);
    this->signalDevice = TLC5947_SignalDevice::make(1, 24, this->signalSPIDevice);
};

void Kornweinheim::systemSetupComplete()
{
#ifdef RAILWAY_DEBUG_INJECTOR
    for (auto& [key, turnout] : this->railway->turnouts())
        this->stationDebugInjector->injectTurnoutUpdate(turnout, std::rand() % 2 ? winston::Turnout::Direction::A_B : winston::Turnout::Direction::A_C);
#endif
    this->signalSPIDevice->skipSend(false);
    this->signalDevice->flush();
    this->signalBox->order(this->signalDevice->flushCommand(40));
}

// accept new requests and loop over what the signal box has to do
bool Kornweinheim::systemLoop() {
    this->webServer.step();
    return this->signalBox->work();
}

void Kornweinheim::populateLocomotiveShed()
{
    auto callbacks = locoCallbacks();
    this->addLocomotive(callbacks, 3, "BR 114");
    this->addLocomotive(callbacks, 4, "BR 106");
    this->addLocomotive(callbacks, 5, "BR 64");
    this->addLocomotive(callbacks, 6, "E 11");
}

