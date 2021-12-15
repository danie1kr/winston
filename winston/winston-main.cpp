// winston-simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "Kornweinheim.h"
#include "winston-main.h"

Kornweinheim kwh;
void winston_setup()
{
	winston::hal::storageSetFilename(Kornweinheim::name());
    winston::hal::init();
	winston::hal::text("Hello from Winston!");
    std::srand((unsigned int)(winston::hal::now() & 0x00000000FFFFFFFF));

	//using Modelleisenbahn = MRS<RailwayWithSiding>;
	//using Modelleisenbahn = MRS<TimeSaverRailway
	//using Modelleisenbahn = MRS<Y2020Railway>;


	// setup
    kwh.setup();
}

void winston_loop()
{
    if (!kwh.loop())
        winston::hal::delay(FRAME_SLEEP);
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
