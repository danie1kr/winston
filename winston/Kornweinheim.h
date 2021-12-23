#pragma once

#include <functional>

#include "../libwinston/Winston.h"
#include "../libwinston/Log.h"

#include "winston-main.h"
#include "railways.h"

#include "external/json11/json11.hpp"

#ifdef WINSTON_PLATFORM_WIN_x64
#include "winston-hal-x64.h"
#define WINSTON_WITH_WEBSOCKETS
#endif

#ifdef WINSTON_PLATFORM_TEENSY
#include "../winston-teensy/winston-hal-teensy.h"
#endif

#include "external/central-z21/Z21.h"
#include "TLC5947_SignalDevice.h"

constexpr auto FRAME_SLEEP = 50;

constexpr auto RAILWAY_DEBUG_INJECTOR_DELAY = 1000;
//#define RAILWAY_CLASS RailwayWithSiding
//#define RAILWAY_CLASS TimeSaverRailway
//#define RAILWAY_CLASS Y2020Railway
//#define RAILWAY_CLASS SignalRailway
#define RAILWAY_CLASS Y2021Railway

using namespace json11;

class Kornweinheim : public winston::ModelRailwaySystem<RAILWAY_CLASS::Shared, RAILWAY_CLASS::AddressTranslator::Shared, Z21::Shared>
{
private:

    // send a turnout state via websocket
    void turnoutSendState(const std::string turnoutTrackId, const winston::Turnout::Direction dir);

    // send a signal state via websocket
    void signalSendState(const std::string trackId, const winston::Track::Connection connection, const winston::Signal::Aspects aspects);

    void locoSend(winston::Locomotive& loco);

    void locoSend(winston::Address address);

    void initNetwork();

    winston::DigitalCentralStation::Callbacks z21Callbacks();

    winston::Locomotive::Callbacks locoCallbacks();

    winston::Railway::Callbacks railwayCallbacks();

    /* websocket */
    WebServer webServer;

    // Define a callback to handle incoming messages
    WebServer::HTTPResponse on_http(WebServer::Client client, std::string resource);

    void writeAttachedSignal(Json::array& signals, winston::Track::Shared track, const winston::Track::Connection connection);

    // Define a callback to handle incoming messages
    void on_message(WebServer::Client client, std::string message);

    // setup our model railway system
    void systemSetup();

    void systemSetupComplete();

    // accept new requests and loop over what the signal box has to do
    bool systemLoop();

    void populateLocomotiveShed();


    /* z21 */
    UDPSocket::Shared z21Socket;

    const std::string z21IP = { "192.168.188.100" };
    const unsigned short z21Port = 21105;

    /* Signal Device */
    SignalSPIDevice::Shared signalSPIDevice;
    TLC5947_SignalDevice::Shared signalDevice;
};

//#include "Kornweinheim.h"

// send a turnout state via websocket
void Kornweinheim::turnoutSendState(const std::string turnoutTrackId, const winston::Turnout::Direction dir)
{
    Json obj = Json::object{
        { "op", "turnoutState" },
        { "data", Json::object{
            { "id", turnoutTrackId },
            { "state", (int)dir }
            }
        }
    };
    webServer.broadcast(obj.dump());
}

// send a signal state via websocket
void Kornweinheim::signalSendState(const std::string trackId, const winston::Track::Connection connection, const winston::Signal::Aspects aspects)
{
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
}

void Kornweinheim::locoSend(winston::Locomotive& loco)
{
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
}

void Kornweinheim::locoSend(winston::Address address)
{
    if (auto loco = this->locoFromAddress(address))
    {
        locoSend(*loco);
    }
}
void Kornweinheim::initNetwork()
{
    // z21
    z21Socket = UDPSocket::make(z21IP, z21Port);

    // webServer
    this->webServer.init(
        std::bind(&Kornweinheim::on_http, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&Kornweinheim::on_message, this, std::placeholders::_1, std::placeholders::_2),
        8080);
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
    callbacks.programmingTrackStatusCallback = [=] (const bool programmingOn) {
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
        locoSend(loco);
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
        turnoutSendState(turnout->name(), direction);
        return winston::State::Finished;
    };

    callbacks.signalUpdateCallback = [=](winston::Track::Shared track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
    {
        // send to web socket server
        signalSendState(track->name(), connection, aspects);

        // update physical light
        this->signalDevice->update(track->signalGuarding(connection));

        return winston::State::Finished;
    };

    return callbacks;
}

// Define a callback to handle incoming messages
WebServer::HTTPResponse Kornweinheim::on_http(WebServer::Client client, std::string resource) {
    const std::string path_index("/");
    const std::string path_railway("/railway");
    const std::string path_log("/log");

    const std::string header_html("\r\ncontent-type: text/html; charset=UTF-8\r\n");
    const std::string header_json("\r\ncontent-type: application/json; charset=UTF-8\r\n");
    WebServer::HTTPResponse response;
    if (resource.compare(path_index) == 0)
    {
        response.headers = { {"content-type", "text/html; charset=UTF-8"} };
        response.body = "<html>winston</html>";
    }
    else if (resource.compare(path_railway) == 0)
    {
        response.headers = { {"content-type", "application/json; charset=UTF-8"} };
        response.body = "{}";
    }
    else if (resource.compare(path_log) == 0)
    {
        //extern winston::Logger winston::logger;
        response.headers = { {"content-type", "text/html; charset=UTF-8"} };
        response.body = "<html><head>winston</head><body><table><tr><th>timestamp</th><th>level</th><th>log</th></tr>";
        for (const auto& entry : winston::logger.entries())
        {
            response.body.append("<tr><td>")
                .append(winston::build(entry.timestamp)).append("</td><td>")
                .append(entry.level._to_string()).append("</td><td>")
                .append(entry.text).append("</td><td></tr>");
        }

        response.body.append("</table></body></html>");
    }
    response.status = 200;

    return response;
}

void Kornweinheim::writeAttachedSignal(Json::array& signals, winston::Track::Shared track, const winston::Track::Connection connection)
{
    auto signal = track->signalGuarding(connection);
    if (signal)
        signals.push_back(Json::object{
            { "parentTrack", track->name()} ,
            { "guarding", winston::Track::ConnectionToString(connection)},
            { "pre", signal->preSignal()},
            { "main", signal->mainSignal() }
            });
}

// Define a callback to handle incoming messages
void Kornweinheim::on_message(WebServer::Client client, std::string message) {
    std::string jsonParseError;
    Json m = Json::parse(message, jsonParseError);
    std::string op = m["op"].dump();
    Json data = m["data"];

    if (std::string("\"doTurnoutToggle\"").compare(op) == 0)
    {
        auto id = data["id"].string_value();
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
    else if (std::string("\"getTurnoutState\"").compare(op) == 0)
    {
        auto id = data["id"].string_value();
        auto turnout = std::dynamic_pointer_cast<winston::Turnout>(railway->track(id));
        this->turnoutSendState(turnout->name(), turnout->direction());
    }
    else if (std::string("\"getSignalState\"").compare(op) == 0)
    {
        auto id = data["parentTrack"].string_value();
        std::string guarding = data["guarding"].string_value();

        auto connection = winston::Track::ConnectionFromString(guarding);
        auto signal = this->railway->track(id)->signalGuarding(connection);

        this->signalSendState(id, connection, signal->aspect());
    }
    else if (std::string("\"getRailway\"").compare(op) == 0)
    {
        auto tracks = Json::array();
        auto signals = Json::array();
        auto blocks = Json::array();

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

                Json::object track = Json::object();
                track["a"] = a->name();
                track["name"] = bumper->name();
                tracks.push_back(track);

                writeAttachedSignal(signals, bumper, winston::Track::Connection::A);

                break;
            }
            case winston::Track::Type::Rail:
            {
                winston::Rail::Shared rail = std::dynamic_pointer_cast<winston::Rail>(track);
                winston::Track::Shared a, b;
                rail->connections(a, b);

                Json::object track = Json::object();
                track["a"] = a->name();
                track["b"] = b->name();
                track["name"] = rail->name();
                tracks.push_back(track);

                writeAttachedSignal(signals, rail, winston::Track::Connection::A);
                writeAttachedSignal(signals, rail, winston::Track::Connection::B);

                break;
            }
            case winston::Track::Type::Turnout:
            {
                winston::Turnout::Shared turnout = std::dynamic_pointer_cast<winston::Turnout>(track);
                winston::Track::Shared a, b, c;
                turnout->connections(a, b, c);

                Json::object track = Json::object();
                track["a"] = a->name();
                track["b"] = b->name();
                track["c"] = c->name();
                track["name"] = turnout->name();
                tracks.push_back(track);

                writeAttachedSignal(signals, turnout, winston::Track::Connection::A);
                writeAttachedSignal(signals, turnout, winston::Track::Connection::B);
                writeAttachedSignal(signals, turnout, winston::Track::Connection::C);
                break;
            }
            }
        }

        for (auto& block : this->railway->blocks())
        {
            Json::object b = Json::object();
            b["address"] = block.first;
            auto bl = block.second;

            Json::array blockTracks = Json::array();
            for (auto& track : bl->tracks())
                blockTracks.push_back(track->name());
            b["tracks"] = blockTracks;
            blocks.push_back(b);
        }

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
    }
    else if (std::string("\"storeRailwayLayout\"").compare(op) == 0)
    {
        unsigned int address = 0;
        auto layout = data["layout"].string_value();
        auto length = layout.size();
        auto offset = (size_t)data["offset"].int_value();
        auto fullSize = (size_t)data["fullSize"].int_value();

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
            Json successObject = Json::object{
                {"op", "storeRailwayLayoutSuccessful"},
                {"data", true}
            };
            this->webServer.send(client, successObject.dump());
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

        const size_t sizePerMessage = size_t(0.7f * 32000);// webServer.maxMessageSize());
        size_t remaining = length;
        size_t offset = 0;

        while (remaining > 0)
        {
            size_t sent = remaining > sizePerMessage ? sizePerMessage : remaining;
            auto layout = std::string(sent, '0');

            for (size_t i = 0; i < sent; ++i)
                layout[i] = winston::hal::storageRead(address + offset + i);

            Json successObject = Json::object{
                {"op", "layout"},
                {"data", Json::object{
                    {"offset", (int)offset},
                    {"fullSize", (int)length},
                    {"layout", layout}
                }}
            };
            /*this->webServer.send(client, successObject.dump());
            JSON successObject = JSON::Make(JSON::Class::Object);
            successObject["op"] = "layout";
            auto data = JSON::Make(JSON::Class::Object);
            data["offset"] = (unsigned int)offset;
            data["fullSize"] = (unsigned int)length;
            data["layout"] = layout.c_str();
            successObject["data"] = data;*/
            this->webServer.send(client, successObject.dump());

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

// setup our model railway system
void Kornweinheim::systemSetup() {
    this->initNetwork();

    // the user defined railway and its address translator
    this->railway = RAILWAY_CLASS::make(railwayCallbacks());
    this->addressTranslator = RAILWAY_CLASS::AddressTranslator::make(railway);

    // the internal signal box
    this->signalBox = winston::SignalBox::make(nullMutex);

    // the system specific digital central station
    auto at = std::dynamic_pointer_cast<winston::DigitalCentralStation::TurnoutAddressTranslator>(addressTranslator);
    auto udp = std::dynamic_pointer_cast<winston::hal::UDPSocket>(this->z21Socket);
    this->digitalCentralStation = Z21::make(udp, at, *this, this->signalBox, z21Callbacks());

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
    this->signalBox->order(this->signalDevice->flushCommand(40));
}

// accept new requests and loop over what the signal box has to do
bool Kornweinheim::systemLoop() {

#ifdef WINSTON_WITH_WEBSOCKETS
    this->webServer.step();
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

