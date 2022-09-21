/*
 Name:		winston_esp_signal.ino
 Created:	9/21/2022 9:00:12 PM
 Author:	daniel
*/

#include <SPI.h>
#include <ESP8266WiFi.h>

#include <array>

#include "wifi.h"

WiFiServer server(24000);
std::array<unsigned char, 32> buffer;
unsigned char signals = 0;

const unsigned int PIN_74HC595_CS = D8;
SPISettings spi_settings = SPISettings(8000000, MSBFIRST, SPI_MODE0);

// the setup function runs once when you press reset or power the board
void setup() {

	pinMode(PIN_74HC595_CS, OUTPUT);
	updateLEDs();
	WiFi.begin(ssid, password);
}

bool updateLEDs()
{
	SPI.beginTransaction(spi_settings);
	digitalWrite(PIN_74HC595_CS, LOW);                 // select 74HC595
	delay(10);                                            // wait for initialization
	SPI.transfer(signals);                            // write value
	delay(10);                                            // wait for operation complete
	digitalWrite(PIN_74HC595_CS, HIGH);                // release 74HC595
	SPI.endTransaction();

	return true;
}

// the loop function runs over and over again until power down or reset
void loop() {
	WiFiClient client = server.available();
	if (client)
	{
		while (client.connected())
		{
			if (client.available())
			{
				size_t read = client.readBytes(buffer.data(), buffer.size());
				if (read > 0)
				{
					signals = buffer.at(read - 1);
					updateLEDs();
				}
			}
			delay(50);
		}
	}
	delay(50);
}
