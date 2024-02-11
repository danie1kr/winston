
// don't include binary.h
#define Binary_h
#include "winston-display.h"


/* debug:
C:\Users\XX\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\esp-2021r2-patch5-8.4.0\bin\xtensa-esp32-elf-addr2line.exe -pfiaC -e C:\Users\XX\AppData\Local\Temp\VMBuilds\winston-display-esp\esp32_esp32s3\winston-display-esp.ino.elf -a 0x4200a0cb
*/
SET_LOOP_TASK_STACK_SIZE(32 * 1024);

// the setup function runs once when you press reset or power the board
void setup() {
    winston_setup();
}

// the loop function runs over and over again until power down or reset
void loop() {
    winston_loop();
}
