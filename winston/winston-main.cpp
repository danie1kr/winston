// winston-main.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "winston-main.h"

#include "Kornweinheim.hpp"
Kornweinheim modelRailWayConfiguration;

/*
#include "TimeSaver.hpp"
TimeSaver modelRailWayConfiguration;
*/
void winston_setup()
{
#ifdef WINSTON_PLATFORM_TEENSY
    int led = 22;
    pinMode(led, OUTPUT);
    digitalWrite(led, HIGH);
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
#endif
    winston::hal::init();
	winston::hal::text("Hello from Winston!"_s);
    std::srand((unsigned int)(inMilliseconds(winston::hal::now().time_since_epoch())));

	//using Modelleisenbahn = MRS<RailwayWithSiding>;
	//using Modelleisenbahn = MRS<TimeSaverRailway
	//using Modelleisenbahn = MRS<Y2020Railway>;

#ifdef WINSTON_PLATFORM_TEENSY
    digitalWrite(led, LOW);
#endif
	// setup
    modelRailWayConfiguration.setup();
    winston::hal::text("Setup complete!"_s);
#ifdef WINSTON_PLATFORM_TEENSY
    digitalWriteFast(led, HIGH);
#endif
}

#ifdef WINSTON_STATISTICS
auto nextSWJPrint = winston::hal::now().time_since_epoch();
size_t loopsPerSecond = 0;
const size_t secondsPerPrint = WINSTON_STATISTICS_SECONDS_PER_PRINT;
#endif
void winston_loop()
{
	for (size_t crumb = 0; crumb < 6; ++crumb)
        TEENSY_CRASHLOG_BREADCRUMB(crumb, 0x0);

#ifdef WINSTON_WITH_QNETHERNET
    TEENSY_CRASHLOG_BREADCRUMB(1, 0x1);
    Ethernet.loop();
#endif
    TEENSY_CRASHLOG_BREADCRUMB(1, 0x2);
    if (modelRailWayConfiguration.loop() == winston::Result::Idle)
        winston::hal::delay(FRAME_SLEEP);
    TEENSY_CRASHLOG_BREADCRUMB(1, 0x3);
#ifdef WINSTON_STATISTICS
    if (winston::hal::now().time_since_epoch() > nextSWJPrint)
    {        
        nextSWJPrint = winston::hal::now().time_since_epoch() + toSeconds(secondsPerPrint);

        LOG_INFO(kwh.statistics(5));
#ifdef WINSTON_STATISTICS_DETAILLED
        LOG_INFO(kwh.statisticsSignalTower(5));
#endif
        LOG_INFO(winston::build("LooPS: ", loopsPerSecond / secondsPerPrint));
        loopsPerSecond = 0;
    }
    ++loopsPerSecond;
#endif
}

#ifdef WINSTON_PLATFORM_WIN_x64
int main()
{
    // setup
    winston_setup();

    // and loop
    while (true)
        winston_loop();
}
#endif
