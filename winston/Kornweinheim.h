#pragma once

#include <functional>

#include "../libwinston/Winston.h"
#include "../libwinston/Log.h"
#include "../libwinston/Util.h"

#include "winston-main.h"
#include "railways.h"

#ifdef WINSTON_PLATFORM_WIN_x64
#include "winston-hal-x64.h"
#endif

#ifdef WINSTON_PLATFORM_TEENSY
#include "../winston-teensy/winston-hal-teensy.h"
#include "../winston-teensy/winston-hal-teensy-webserver.hpp"
#endif

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

#include "external/central-z21/Z21.h"
#include "TLC5947_SignalDevice.h"
#include "PN532_DetectorDevice.h"

constexpr auto FRAME_SLEEP = 50;

constexpr auto WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY = 1000;
//#define RAILWAY_CLASS RailwayWithSiding
//#define RAILWAY_CLASS TimeSaverRailway
//#define RAILWAY_CLASS Y2020Railway
//#define RAILWAY_CLASS SignalRailway
#define RAILWAY_CLASS Y2021Railway


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

    void setupSignals();
    void setupDetectors();

    winston::DigitalCentralStation::Callbacks z21Callbacks();

    winston::Locomotive::Callbacks locoCallbacks();

    winston::Railway::Callbacks railwayCallbacks();

#ifdef WINSTON_WITH_WEBSOCKET
    /* websocket */
    WebServer webServer;

    // add a signal to the /signal output
    void writeSignal(WebServer::HTTPConnection& connection, const winston::Track::Shared track, const winston::Track::Connection trackCon);

    // Define a callback to handle incoming messages
    void on_http(WebServer::HTTPConnection& connection, const winston::HTTPMethod method, const std::string& resource);

    void writeAttachedSignal(JsonArray& signals, winston::Track::Shared track, const winston::Track::Connection connection);

    // Define a callback to handle incoming messages
    void on_message(WebServer::Client &client, const std::string &message);
#endif
    // setup our model railway system
    void systemSetup();

    void systemSetupComplete();

    // accept new requests and loop over what the signal box has to do
    bool systemLoop();

    void populateLocomotiveShed();

    void inventStorylines();
    std::vector<winston::Storyline::Shared> storylines;
    winston::Storyline::Shared activeStory = nullptr;
    winston::ConfirmationProvider::Shared userConfirmation;
    winston::TaskConfirm::Display::Shared display;

    /* z21 */
    UDPSocket::Shared z21Socket;

    const std::string z21IP = { "192.168.188.100" };
    const unsigned short z21Port = 21105;

    /* Signal Device */
    SignalInterfaceDevice::Shared signalInterfaceDevice;
    TLC5947_SignalDevice::Shared signalDevice;

    /* Storage */
    Storage::Shared storageLayout;

    /* Occupancy Detector */
    winston::Result detectorUpdate(winston::Detector::Shared detector, winston::Locomotive &loco);
#ifdef WINSTON_PLATFORM_TEENSY
    
#elif defined(WINSTON_PLATFORM_WIN_x64)
    SerialDeviceWin::Shared serial;
#endif

#ifdef WINSTON_LOCO_TRACKING
    void webSocket_sendLocosPosition();
    winston::TimePoint lastWebsocketLocoTrackingUpdate;
#endif
};