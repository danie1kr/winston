#pragma once
#include "../libwinston/WinstonConfig.h"

#include "../libwinston/HAL.h"

#include <SPI.h>
#include <SdFat.h>
#include <WiFi.h>
//#include <LovyanGFX.hpp>
#include <JPEGDEC.h>
#include <lvgl.h>

//#include <Wire.h>
//#include <driver/i2c.h>

#include <bb_captouch.h>
#include <bb_spi_lcd.h>

// https://www.makerfabs.com/esp32-s3-parallel-tft-with-touch-ili9488.html
#define TOUCH_I2C_ADD 0x38
#define TOUCH_REG_XL 0x04
#define TOUCH_REG_XH 0x03
#define TOUCH_REG_YL 0x06
#define TOUCH_REG_YH 0x05
//extern SdFat SD;

class DisplayUXESP32 : public winston::hal::DisplayUX, public winston::Shared_Ptr<DisplayUXESP32>
{
public:
	DisplayUXESP32(const unsigned int width, const unsigned int height);
	virtual ~DisplayUXESP32() = default;
	virtual const winston::Result init(const std::string title = "");
	virtual const winston::Result setCursor(unsigned int x, unsigned int y);
	virtual const bool getTouch(unsigned int& x, unsigned int& y);
	virtual const winston::Result draw(unsigned int x, unsigned int y, unsigned int w, unsigned int h, void* data);
	virtual void displayLoadingScreen();
	virtual const winston::Result brightness(unsigned char value); 
	virtual const unsigned char brightness();
	virtual const unsigned int tick();

	using winston::Shared_Ptr<DisplayUXESP32>::Shared;
	using winston::Shared_Ptr<DisplayUXESP32>::make;

private:

	BB_SPI_LCD lcd;
	BBCapTouch touch;
	const unsigned int lvBufferSize;
	lv_display_t *lvDisplay;
	lv_indev_t* lvInput;
	void* lvBuffer;
	winston::TimePoint lastTick;
};
using DisplayUX = DisplayUXESP32;
