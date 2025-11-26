/*
 Name:		winston_teensy.ino
 Created:	12/9/2021 10:04:53 PM
 Author:	daniel
*/
// don't include binary.h
#define Binary_h
#include "winston-main.h"

/*
teensy4.1 boards.txt:
edit fnet_user_config.h and disable unneeded stuff

use a board.txt with:
    # remove -fno-rtti
    teensy41.build.flags.cpp=-std=gnu++14 -fno-exceptions -fpermissive -fno-threadsafe-statics -felide-constructors -Wno-error=narrowing
    recipe.hooks.deploy.postupload.1.pattern=cmd.exe /c ping localhost -n 10 

    vm.build.sketch.cpp-use-build-cache=true
    vm.preproc.lib-search.lib-in-lib=all
    vm.preproc.lib-search.lib-in-sketch=all
    vm.ctags.cache.gcc-e=true

debugger: hardware
default optimization
menu -> teensy option 3 "optimize" -> debug

### not booting?
debugger: off
default optimization
menu -> teensy option 3 "optimize" -> fast
    teensy_size:   FLASH: code:420896, data:79904, headers:8608   free for files:7617056
    teensy_size:    RAM1: variables:88612, code:415924, padding:10060   free for local variables:9692
    teensy_size:    RAM2: variables:24768  free for malloc/new:499520

debugger: hardware
default optimization
menu -> teensy option 3 "optimize" -> faster
    teensy_size:   FLASH: code:434528, data:78880, headers:8288   free for files:7604768
    teensy_size:    RAM1*: variables:87588, code:429628, padding:29124   free for local variables:-22052
    teensy_size:    RAM2: variables:24768  free for malloc/new:499520
    
debugger: hardware
default optimization
menu -> teensy option 3 "optimize" -> faster with LTO
teensy_size:   FLASH: code:391184, data:77856, headers:8624   free for files:7648800
teensy_size:    RAM1: variables:86692, code:386068, padding:7148   free for local variables:44380
teensy_size:    RAM2: variables:24768  free for malloc/new:499520

### maybe instable?
debugger: hardware
default optimization
menu -> teensy option 3 "optimize" -> smallest

if Crashlog:
    C:\Users\daniel\AppData\Local\Arduino15\packages\arduino\tools\arm-none-eabi-gcc\7-2017q4\bin\arm-none-eabi-addr2line.exe -afiCe .\winston-teensy.ino.elf 0x2f044*/

// the setup function runs once when you press reset or power the board
void setup() {

    do not compile

    /*
    * 
    * watchdog:
    * #include <WDT_T4.h>
WDT_T4<WDT1> wdt;
void setup() {
  WDT_timings_t config;
  config.timeout = 4000; // 4 seconds
  wdt.begin(config);
}
void loop() {
  wdt.feed();
  // your z21 network code here
}
https://github.com/tonton81/WDT_T4/blob/master/examples/watchdog2_demo/watchdog2_demo.ino

log extern "C" char __StackTop, __StackLimit, __heap_start, *__brkval;
void memoryReport() {
  Serial.printf("Heap Free: %u, Stack Free: %u\n",
    (uint32_t)&__StackTop - (uint32_t)(__brkval?__brkval:&__heap_start),
    (uint32_t)&__StackTop - (uint32_t)&__StackLimit);
}

    */

    winston_setup();
}

// the loop function runs over and over again until power down or reset
void loop() {
    winston_loop();
}
