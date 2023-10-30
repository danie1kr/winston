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

constexpr auto FRAME_SLEEP = 50;

//#define RAILWAY_CLASS RailwayWithSiding
//#define RAILWAY_CLASS TimeSaverRailway
//#define RAILWAY_CLASS Y2020Railway
//#define RAILWAY_CLASS SignalRailway
#define RAILWAY_CLASS Y2021Railway


class Kornweinheim : public winston::ModelRailwaySystem<RAILWAY_CLASS, RAILWAY_CLASS::AddressTranslator, Z21, WebServer>
{
private:
    void setupSignals();
    void setupDetectors();

    winston::DigitalCentralStation::Callbacks z21Callbacks();
    winston::Locomotive::Callbacks locoCallbacks();
    winston::Railway::Callbacks railwayCallbacks();
    
    const winston::Result on_http(WebServer::HTTPConnection& connection, const winston::HTTPMethod method, const std::string& resource);
    
    // setup our model railway system
    void systemSetup();

    void systemSetupComplete();

    // accept new requests and loop over what the signal box has to do
    void systemLoop();

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
    winston::SignalController::Shared signalController;
    std::vector<SignalInterfaceDevice::Shared> signalInterfaceDevices;
    std::vector<winston::SignalDevice::Shared> signalDevices;

    //std::vector<winston::Signal::Shared> signals;

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