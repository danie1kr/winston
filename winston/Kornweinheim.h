#pragma once

#include <functional>

#include "../libwinston/Winston.h"

#include "winston.h"
#include "railways.h"

#include "external/json11/json11.hpp"

#ifdef WINSTON_PLATFORM_WIN_x64
#include "winston-hal-x64.h"
#define WINSTON_WITH_WEBSOCKETS
#endif

#ifdef WINSTON_PLATFORM_TEENSY
#include "winston-hal-teensy.h"
#endif

#include "external/central-z21/Z21.h"
#include "TLC5947_SignalDevice.h"

constexpr auto FRAME_SLEEP = 50;

constexpr auto RAILWAY_DEBUG_INJECTOR_DELAY = 1000;
//#define RAILWAY_CLASS RailwayWithSiding
//#define RAILWAY_CLASS TimeSaverRailway
//#define RAILWAY_CLASS Y2020Railway
#define RAILWAY_CLASS SignalRailway
//#define RAILWAY_CLASS Y2021Railway

using namespace json11;

class Kornweinheim : public winston::ModelRailwaySystem<RAILWAY_CLASS::Shared, RAILWAY_CLASS::AddressTranslator::Shared, Z21::Shared>
{
private:

    // send a turnout state via websocket
    void turnoutSendState(const unsigned int turnoutTrackId, const winston::Turnout::Direction dir);

    // send a signal state via websocket
    void signalSendState(const unsigned int trackId, const winston::Track::Connection connection, const winston::Signal::Aspects aspects);

    void locoSend(winston::Locomotive::Shared& loco);

    void locoSend(winston::Address address);

    void initNetwork();

    winston::DigitalCentralStation::Callbacks z21Callbacks();

    winston::Locomotive::Callbacks locoCallbacks();

    winston::Railway::Callbacks railwayCallbacks();

#ifdef WINSTON_WITH_WEBSOCKETS
    /* websocket */
    WebServerWSPP webServer;

    // Define a callback to handle incoming messages
    WebServerWSPP::HTTPResponse on_http(WebServerWSPP::Client client, std::string resource);

    void writeAttachedSignal(Json::array& signals, winston::Track::Shared track, const winston::Track::Connection connection);

    // Define a callback to handle incoming messages
    void on_message(WebServerWSPP::Client client, std::string message);
#endif

    // setup our model railway system
    void systemSetup();

    void systemSetupComplete();

    // accept new requests and loop over what the signal box has to do
    bool systemLoop();

    void populateLocomotiveShed();


    /* z21 */
    UDPSocketLWIP::Shared z21Socket;
    const std::string z21IP = { "192.168.0.100" };
    const unsigned short z21Port = 5000;

    /* Signal Device */
    SignalSPIDevice::Shared signalSPIDevice;
    TLC5947_SignalDevice::Shared signalDevice;
};
