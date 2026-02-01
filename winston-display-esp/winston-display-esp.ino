
// don't include binary.h
#define Binary_h
#include "winston-display.h"

/*
* almost works:
* esp32 platform 3.3.4
* bb_spi_lcd: 2.4.1
* JPEGDEC 1.4.2:
* 
* ??? works:
* esp32 platform 3.3.4
* bb_spi_lcd: 2.4.1
* JPEGDEC 1.8.4
*/

/* debug:
C:\Users\XX\AppData\Local\Arduino15\packages\esp32\tools\xtensa-esp32-elf-gcc\esp-2021r2-patch5-8.4.0\bin\xtensa-esp32-elf-addr2line.exe -pfiaC -e C:\Users\XX\AppData\Local\Temp\VMBuilds\winston-display-esp\esp32_esp32s3\winston-display-esp.ino.elf -a 0x4200a0cb
C:\Users\XX\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2507\bin\xtensa-esp32-elf-addr2line.exe -pafiCe C:\Users\XX\AppData\Local\Temp\VMBuilds\winston-display-esp\esp32_esp32s3/winston-display-esp.ino.elf 0x4203c6b5:0x3fcebde0 0x4203c445:0x3fcebe00 0x4203c478:0x3fcebe20 0x42039cd9:0x3fcebe70 0x4201e821:0x3fcebe90 0x4201090e:0x3fcebed0 0x4200e13a:0x3fcebf00 0x42001cb7:0x3fcebfc0 0x4205f99e:0x3fcebfe0 0x4037ec99:0x3fcec000
*/
SET_LOOP_TASK_STACK_SIZE(8 * 1024);

// the setup function runs once when you press reset or power the board
void setup() {
    winston_setup();
}

// the loop function runs over and over again until power down or reset
void loop() {
    winston_loop();
}
