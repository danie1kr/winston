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

#define PRINT_SIZE(t) winston::hal::text(std::string(std::string(#t) + std::string(": ") + std::to_string(sizeof(t))));
    PRINT_SIZE(winston::Track);
    PRINT_SIZE(winston::Track::Shared);
    PRINT_SIZE(winston::Bumper);
    PRINT_SIZE(winston::Bumper::Shared);
    PRINT_SIZE(winston::Rail);
    PRINT_SIZE(winston::Rail::Shared);
    PRINT_SIZE(winston::Turnout);
    PRINT_SIZE(winston::Turnout::Shared); 
    PRINT_SIZE(winston::Signal::Shared);
    PRINT_SIZE(winston::Signal::Shared);

    // and loop
    while (true)
        winston_loop();
}
#endif
