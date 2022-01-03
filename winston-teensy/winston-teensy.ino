/*
 Name:		winston_teensy.ino
 Created:	12/9/2021 10:04:53 PM
 Author:	daniel
*/
#include "winston-main.h"

// the setup function runs once when you press reset or power the board
void setup() {
    winston_setup();
}

// the loop function runs over and over again until power down or reset
void loop() {
    winston_loop();
}
