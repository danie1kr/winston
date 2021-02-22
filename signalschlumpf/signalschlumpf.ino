
#include <memory>
#include <Adafruit_TLC5947.h>
#include <NativeEthernet.h>

const unsigned int LED_SPI_DATA = 0;
const unsigned int LED_SPI_CLOCK = 0;
const unsigned int LED_SPI_LATCH = 0;
const unsigned int LED_SPI_BLANK = 0;

std::shared_ptr<Adafruit_TLC5947> led;

void initLED(const unsigned int devices)
{
	led->begin();
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {

	if(EthernetClient client = server.accept())
	{
		unsigned int bytesReady = client.available();
		if(bytesReady > 0)
		{
			unsigned byte command = client.read();
			switch(command)
			{
				case 'i':
				{
					if(bytesReady >= 3)
					{
						initLED();

					}
					break;
				}
				case 'p':
				{

					led->setPWM(port, pwm);
				}
				case 'w':
				{
					led->write();
					break;
				}
			}
		}
	}
}