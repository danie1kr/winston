#include <stdint.h>
#include "Log.h"
#include "winston-display-hal-esp32.h"

#include "../winston-display/winston-secrets.h"
#include "../winston-display/res/loading.h"

#define SPI_MOSI 2 //1
#define SPI_MISO 41
#define SPI_SCK 42
#define SD_CS 1 //2
#define LCD_CS 37
#define LCD_BLK 45

#define I2C_SCL 39
#define I2C_SDA 38

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)
#define SD_CONFIG SdSpiConfig(SD_CS, USER_SPI_BEGIN | DEDICATED_SPI, SPI_CLOCK)//, &mySpi)

extern SdFat SD;
extern winston::Logger winston::logger;

namespace winston
{
    namespace hal {
        void init()
        {
            {
                pinMode(LCD_CS, OUTPUT);
                pinMode(LCD_BLK, OUTPUT);

                digitalWrite(LCD_CS, LOW);
                digitalWrite(LCD_BLK, HIGH);

                Serial.begin(115200);
          //          delay(200);
          //      while (!Serial) { //}&& millis() < 2000) {
                    // Wait for Serial to initialize
            //    }
                text("Winston Display Init Hello");

                WiFi.begin(WINSTON_WIFI_SSID, WINSTON_WIFI_PASS);
                winston::runtimeEnableNetwork();

                SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
                if (!SD.begin(SD_CONFIG))
                {
                    Serial.println("Card Mount Failed");
                    SD.initErrorHalt(&Serial);
                    while (1);
                }
                winston::runtimeEnablePersistence();

                logRuntimeStatus();
            }
        }

        void text(const std::string& text)
        {
            Serial.println(text.c_str());
        }

        void error(const std::string& error)
        {
            logger.err(error);
        }

        void fatal(const std::string reason)
        {
            logger.log(reason, Logger::Entry::Level::Fatal);
        }

        void delay(const unsigned int ms)
        {
            ::delay(ms);
        }

        TimePoint now()
        {
            return ::millis();
        }
    }
}

DisplayUXESP32::DisplayUXESP32(const unsigned int width, const unsigned int height)
    : winston::hal::DisplayUX(width, height), winston::Shared_Ptr<DisplayUXESP32>(),
    lcd(), touch(),
    lvBufferSize(width * height / 10 * (LV_COLOR_DEPTH / 8)), lvDisplay(), lvInput(), lvBuffer(nullptr), lastTick(0)
{

}

const winston::Result DisplayUXESP32::init(const std::string title)
{
    this->lcd.begin(DISPLAY_MAKERFABS_S3);
    this->lcd.setRotation(3);

    constexpr int I2C_PIN_SDA = 38;
    constexpr int I2C_PIN_SCL = 39;
    int error = touch.init(I2C_PIN_SDA, I2C_PIN_SCL);
    if (error)
        return winston::Result::ExternalHardwareFailed;

    lvBuffer = malloc(lvBufferSize);
    if (!lvBuffer)
        return winston::Result::OutOfMemory;

    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb([](lv_log_level_t level, const char* buf) {
        LV_UNUSED(level);
        winston::logger.info(buf);
        });
#endif

    this->lvDisplay = lv_display_create(this->width, this->height);
    lv_display_set_user_data(this->lvDisplay, this);
    lv_display_set_flush_cb(this->lvDisplay, [](lv_display_t* display, const lv_area_t* area, unsigned char* data) {
        auto displayUX = (DisplayUXESP32*)lv_display_get_user_data(display);
        uint32_t w = lv_area_get_width(area);
        uint32_t h = lv_area_get_height(area);
        lv_draw_sw_rgb565_swap(data, w * h);
        displayUX->lcd.pushImage(area->x1, area->y1, w, h, (uint16_t*)data);
        lv_display_flush_ready(display);
        });
    lv_display_set_buffers(this->lvDisplay, this->lvBuffer, nullptr, this->lvBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

    this->lvInput = lv_indev_create();
    lv_indev_set_type(this->lvInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_user_data(this->lvInput, this);
    lv_indev_set_read_cb(this->lvInput, [](lv_indev_t* dev, lv_indev_data_t* data) {
        unsigned int touchX, touchY;

        data->state = LV_INDEV_STATE_REL;
        auto displayUX = (DisplayUXESP32*)lv_indev_get_user_data(dev);

        if (displayUX->getTouch(touchX, touchY))
        {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = touchX;
            data->point.y = touchY;
            Serial.printf("touch: %d %d", touchX, touchY);
        }
    });

    return winston::Result::OK;
}

const winston::Result DisplayUXESP32::setCursor(unsigned int x, unsigned int y)
{
    this->lcd.setCursor(x, y);
    return winston::Result::OK;
}
const bool DisplayUXESP32::getTouch(unsigned int& x, unsigned int& y)
{
    TOUCHINFO ti;
    if (touch.getSamples(&ti))
    {
        if (ti.count > 0)
        {
            // swap and flip
            x = this->width - 1 - ti.y[0];
            y = ti.x[0];
            return true;
        }
    }
    return false;
}

const winston::Result DisplayUXESP32::draw(unsigned int x, unsigned int y, unsigned int w, unsigned int h, void* data)
{
    this->lcd.pushImage(x, y, w, h, (uint16_t*)data, DRAW_TO_LCD);
    return winston::Result::OK;
}

void DisplayUXESP32::displayLoadingScreen()
{
    extern JPEGDEC jpeg;
    jpeg.openFLASH((uint8_t*)cinemaLoadingScreen, sizeof(cinemaLoadingScreen), [](JPEGDRAW * pDraw)
    {
        DisplayUXESP32* display = (DisplayUXESP32*)pDraw->pUser;
        display->lcd.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels, DRAW_TO_LCD);
        return 1;
    });

    jpeg.setPixelType(RGB565_BIG_ENDIAN);
    jpeg.setUserPointer(this);
    jpeg.decode(0, 0, 0);
    jpeg.close();
}

const winston::Result DisplayUXESP32::brightness(unsigned char value)
{
    //this->lcd.setBrightness(value);
    return winston::Result::OK;
}

const unsigned char DisplayUXESP32::brightness()
{
    return 255;// DisplayUXESP32::lcd.getBrightness();
}

const unsigned int DisplayUXESP32::tick()
{
    auto now = winston::hal::now();
    lv_tick_inc(now - this->lastTick);
    this->lastTick = now;
    return lv_task_handler();
}