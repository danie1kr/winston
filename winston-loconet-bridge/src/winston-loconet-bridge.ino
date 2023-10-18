
#define LOCONET_NO_EEPROM

#include "LocoNet/LocoNet.h"
#include "../../libwinston/WinstonSharedTypes.h"

// LocoNet RailCom Monitor
// Demonstrates the use of the:
//
// OPC_MULTI_SENSE transponder call-back
//
// Tested on a Uhlenbrock 68500 MARCo-Empfänger in Digitrax Mode
// LNCV 15 == 2 - Send ÜF Digitrax with Locomotive address and Block status (vacant/occupied) 

#define DEBUG
const unsigned int PIN_LOCONET = 0;

lnMsg* LnPacket;

#define DEBUG_SERIAL
#define UPSTREAM_SERIAL Serial

void setup() {
    // First initialize the LocoNet interface
    LocoNet.init(PIN_LOCONET);

#ifdef DEBUG
    Serial1 .begin(115200);
    delay(500);
#endif
}

void loop() {
    // Check for any received LocoNet packets
    LnPacket = LocoNet.receive();
    if (LnPacket) {
#ifdef DEBUG
        // First print out the packet in HEX
        SerialUSB.print("RX: ");
        uint8_t msgLen = getLnMsgSize(LnPacket);
        for (uint8_t x = 0; x < msgLen; x++)
        {
            uint8_t val = LnPacket->data[x];
            // Print a leading 0 if less than 16 to make 2 HEX digits
            if (val < 16)
                SerialUSB.print('0');

            SerialUSB.print(val, HEX);
            SerialUSB.print(' ');
        }
#endif
        // If this packet was not a Switch or Sensor Message then print a new line 
        if (!LocoNet.processSwitchSensorMessage(LnPacket)) {
#ifdef DEBUG
            SerialUSB.println();
#endif
        }
    }
}


// This call-back function is called from LocoNet.processSwitchSensorMessage
// for OPC_MULTI_SENSE 0xD0
void notifyMultiSenseTransponder(uint16_t Address, uint8_t Zone, uint16_t LocoAddress, uint8_t Present) {
#ifdef DEBUG
    SerialUSB.print("Railcom Sensor ");
    SerialUSB.print(Address);
    SerialUSB.print(" reports ");
    SerialUSB.print(Present ? "present" : "absent");
    SerialUSB.print(" of decoder address ");
    SerialUSB.print(LocoAddress, DEC);
    SerialUSB.print(" in zone ");
    SerialUSB.println(Zone, HEX);
#endif
    winston::RailComDetectorSerialMessage rcdsm(Address, Zone, LocoAddress, Present);
    Serial1.write((unsigned char*) & rcdsm, sizeof(winston::RailComDetectorSerialMessage));
}