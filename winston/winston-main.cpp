// winston-simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "Kornweinheim.hpp"
#include "winston-main.h"

Kornweinheim kwh;

void winston_setup()
{
#ifdef WINSTON_PLATFORM_TEENSY
    int led = 13;
    pinMode(led, OUTPUT);
    digitalWrite(led, HIGH);
#endif
	winston::hal::storageSetFilename(Kornweinheim::name());
    winston::hal::init();
	winston::hal::text("Hello from Winston!"_s);
#ifdef WINSTON_HAS_CHRONO
    std::srand((unsigned int)(inMilliseconds(winston::hal::now().time_since_epoch())));
#else
    std::srand((unsigned int)(inMilliseconds(winston::hal::now())));
#endif

	//using Modelleisenbahn = MRS<RailwayWithSiding>;
	//using Modelleisenbahn = MRS<TimeSaverRailway
	//using Modelleisenbahn = MRS<Y2020Railway>;

#ifdef WINSTON_PLATFORM_TEENSY
	// setup
    digitalWrite(led, LOW);
#endif
    kwh.setup();
#ifdef WINSTON_PLATFORM_TEENSY
    digitalWrite(led, HIGH);
#endif
}

#ifdef WINSTON_STATISTICS
#ifdef WINSTON_HAS_CHRONO
auto nextSWJPrint = winston::hal::now().time_since_epoch();
#else
auto nextSWJPrint = winston::hal::now();
#endif
size_t loopsPerSecond = 0;
const size_t secondsPerPrint = WINSTON_STATISTICS_SECONDS_PER_PRINT;
#endif
void winston_loop()
{
    if (!kwh.loop())
        winston::hal::delay(FRAME_SLEEP);
#ifdef WINSTON_STATISTICS
#ifdef WINSTON_HAS_CHRONO
    if (winston::hal::now().time_since_epoch() > nextSWJPrint)
    {        
        nextSWJPrint = winston::hal::now().time_since_epoch() + toSeconds(secondsPerPrint);
#else
    if (winston::hal::now() > nextSWJPrint)
    {
        nextSWJPrint = winston::hal::now() + toSeconds(secondsPerPrint);
#endif
        winston::logger.info(kwh.statistics(5));
        winston::logger.info(kwh.statisticsSignalBox(5));
        winston::logger.info(winston::build("LooPS: ", loopsPerSecond / secondsPerPrint));
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
