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

    void setupSignals();
    void setupDetectors();

    winston::DigitalCentralStation::Callbacks z21Callbacks();
    winston::Locomotive::Callbacks locoCallbacks();
    winston::Railway::Callbacks railwayCallbacks();

#ifdef WINSTON_WITH_WEBSOCKET
    winston::WebUI<WebServer, RAILWAY_CLASS, Storage> webUI;
    
    /* websocket *
    WebServer webServer;

    // add a signal to the /signal output
    void writeSignal(WebServer::HTTPConnection& connection, const winston::Track::Shared track, const winston::Track::Connection trackCon);
    */
    // Define a callback to handle incoming messages
    winston::Result on_http(WebServer::HTTPConnection& connection, const winston::HTTPMethod method, const std::string& resource);
    void writeSignalHTMLList(WebServer::HTTPConnection& connection, const winston::Track::Shared track, const winston::Track::Connection trackCon);

    /*
    void writeAttachedSignal(JsonArray& signals, winston::Track::Shared track, const winston::Track::Connection connection);

    // Define a callback to handle incoming messages
    void on_message(WebServer::Client &client, const std::string &message);*/
#endif
    // setup our model railway system
    void systemSetup();

    const winston::Result orderRouteSet(winston::Route::Shared route, const bool set);
    std::vector<winston::Route::Shared> routesInProgress;

    void orderTurnoutToggle(winston::Turnout::Shared turnout, winston::Turnout::Direction direction);
    void orderDoubleSlipTurnoutToggle(winston::DoubleSlipTurnout::Shared turnout, winston::DoubleSlipTurnout::Direction direction);
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

    std::vector<winston::Signal::Shared> signals;

    /*
    * cie1931 linear LED-PWM mapping
    static const size_t TLC5947_LED_Steps = 1 << TLC5947_SignalDevice::bits;
    
    template<size_t _N>
    static constexpr std::array<unsigned short, _N> cie1931() 
    {
        // see https://jared.geek.nz/2013/feb/linear-led-pwm and https://stackoverflow.com/a/56383477
        std::array<unsigned short, _N> a{};
        for (size_t i = 0; i < _N; ++i)
        {
            double L = (double)i/(double)_N * 100.0;
            a[i] = (L <= 8 ? (L / 903.3) : pow((L + 16.0) / 119.0, 3.0)) * TLC5947_SignalDevice::bits;
        }
        return a;
    };
    static constexpr std::array<unsigned short, TLC5947_LED_Steps> TLC5947_LED_map = cie1931<TLC5947_LED_Steps>();
    
    template <unsigned... i>
    static constexpr auto init_axis(std::integer_sequence<unsigned, i...>) {
        return std::array{ (180 + 0.1 * i)... };
    };
    template <unsigned short... i>
    static constexpr auto cie1931(std::integer_sequence<unsigned short, i...>)
    {
        constexpr double L = (double)i / (double)TLC5947_LED_Steps * 100.0;
        constexpr auto v = (unsigned short)((L <= 8 ? (L / 903.3) : pow((L + 16.0) / 119.0, 3.0)) * TLC5947_LED_Steps);
        return std::array{ v... };
        //return std::array{ (180 + 0.1 * i)... };
    };

    //static constexpr auto axis = init_axis(std::make_integer_sequence<unsigned, num_points>{});
    static constexpr auto TLC5947_LED_map = cie1931(std::make_integer_sequence<unsigned short, TLC5947_LED_Steps>{});
    */
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