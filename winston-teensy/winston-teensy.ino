/*
 Name:		winston_teensy.ino
 Created:	12/9/2021 10:04:53 PM
 Author:	daniel
*/
#include "winston-main.h"

/*
teensy4.1 boards.txt:

remove -fno-rtti:
 old:
  #teensy41.build.flags.cpp=-std=gnu++14 -fno-exceptions -fpermissive -fno-rtti -fno-threadsafe-statics -felide-constructors -Wno-error=narrowing
 new:
  teensy41.build.flags.cpp=-std=gnu++14 -fno-exceptions -fpermissive -fno-threadsafe-statics -felide-constructors -Wno-error=narrowing

edit fnet_user_config.h and disable unneeded stuff

use a board.txt with:
    vm.build.sketch.cpp-use-build-cache=true
    vm.preproc.lib-search.lib-in-lib=all
    vm.preproc.lib-search.lib-in-sketch=all
    vm.ctags.cache.gcc-e=true

debugger: hardware
default optimization
menu -> teensy option 3 "optimize" -> debug
*/

// the setup function runs once when you press reset or power the board
void setup() {
    winston_setup();
}

// the loop function runs over and over again until power down or reset
void loop() {
    winston_loop();
}
