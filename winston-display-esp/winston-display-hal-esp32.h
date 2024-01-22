#pragma once
#include "../libwinston/WinstonConfig.h"

#include "../libwinston/HAL.h"

#include <SPI.h>
#include <SdFat.h>
#include <WiFi.h>
#include <LovyanGFX.hpp>
#include <JPEGDEC.h>
#include <lvgl.h>

#include <Wire.h>
#include <driver/i2c.h>

// https://www.makerfabs.com/esp32-s3-parallel-tft-with-touch-ili9488.html
#define TOUCH_I2C_ADD 0x38
#define TOUCH_REG_XL 0x04
#define TOUCH_REG_XH 0x03
#define TOUCH_REG_YL 0x06
#define TOUCH_REG_YH 0x05
extern SdFat SD;

class DisplayUXESP32 : public winston::hal::DisplayUX, public winston::Shared_Ptr<DisplayUXESP32>
{
	class LGFX : public lgfx::LGFX_Device
	{
		static constexpr int I2C_PORT_NUM = I2C_NUM_0;
		static constexpr int I2C_PIN_SDA = 38;
		static constexpr int I2C_PIN_SCL = 39;
		static constexpr int I2C_PIN_INT = 40;

		lgfx::Bus_Parallel16 _bus_instance;
		lgfx::Panel_ILI9488 _panel_instance;
		lgfx::Light_PWM     _light_instance;
		lgfx::ITouch* _touch_instance_ptr = nullptr;

		/// Detects and configures the touch panel during initialization;
		bool init_impl(bool use_reset, bool use_clear) override;

	public:
		LGFX(const unsigned int screenWidth, const unsigned int screenHeight);
		const unsigned int screenWidth, screenHeight;
	};
public:
	DisplayUXESP32(const unsigned int width, const unsigned int height);
	virtual ~DisplayUXESP32() = default;
	virtual const winston::Result init();
	virtual const winston::Result setCursor(unsigned int x, unsigned int y);
	virtual const bool getTouch(unsigned int& x, unsigned int& y);
	virtual const winston::Result draw(unsigned int x, unsigned int y, unsigned int w, unsigned int h, void* data);
	virtual const winston::Result brightness(unsigned char value); 
	virtual const unsigned char brightness();
	virtual const unsigned int tick();

	static int ft6236_readTouchReg(int reg);
	static int ft6236_getTouchPointX();
	static int ft6236_getTouchPointY();
	static int ft6236_getPos(int pos[2]);

	using winston::Shared_Ptr<DisplayUXESP32>::Shared;
	using winston::Shared_Ptr<DisplayUXESP32>::make;

    static LGFX lcd;
	const unsigned int lvBufferSize;
	lv_display_t *lvDisplay;
	lv_indev_t* lvInput;
	void* lvBuffer;
};
using DisplayUX = DisplayUXESP32;
