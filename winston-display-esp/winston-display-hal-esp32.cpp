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
                    delay(200);
                while (!Serial) { //}&& millis() < 2000) {
                    // Wait for Serial to initialize
                }
                text("Winston Display Init Hello");
                Serial.flush();

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
                /*
                const char loadingScreenJPEG[] = "/loading.jpg";
                jpeg.open(loadingScreenJPEG, jpegOpen, jpegClose, jpegRead, jpegSeek, jpegDraw);
                jpeg.setPixelType(RGB565_BIG_ENDIAN);
                jpeg.decode(0, 0, 0);
                jpeg.close();*/

                /*I2C init
                Wire.begin(I2C_SDA, I2C_SCL);
                byte error, address;

                Wire.beginTransmission(TOUCH_I2C_ADD);
                error = Wire.endTransmission();

                if (error == 0)
                {
                    Serial.print("I2C device found at address 0x");
                    Serial.print(TOUCH_I2C_ADD, HEX);
                    Serial.println("  !");
                    //lcd.print("TOUCH INIT OVER");
                }
                else
                {
                    Serial.print("Unknown error at address 0x");
                    Serial.println(TOUCH_I2C_ADD, HEX);
                    //lcd.print("ERROR:   TOUCH");
                }*/

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
/*
DisplayUXESP32::LGFX::LGFX(const unsigned int screenWidth, const unsigned int screenHeight)
    : screenWidth(screenWidth), screenHeight(screenHeight)
{
    {
        auto cfg = _bus_instance.config();

        cfg.freq_write = 40000000;
        cfg.pin_wr = 35;
        cfg.pin_rd = 48;//34;
        cfg.pin_rs = 36;

        cfg.pin_d0 = 47;//33;
        cfg.pin_d1 = 21;
        cfg.pin_d2 = 14;
        cfg.pin_d3 = 13;
        cfg.pin_d4 = 12;
        cfg.pin_d5 = 11;
        cfg.pin_d6 = 10;
        cfg.pin_d7 = 9;
        cfg.pin_d8 = 3;
        cfg.pin_d9 = 8;
        cfg.pin_d10 = 16;
        cfg.pin_d11 = 15;
        cfg.pin_d12 = 7;
        cfg.pin_d13 = 6;
        cfg.pin_d14 = 5;
        cfg.pin_d15 = 4;
        _bus_instance.config(cfg);
        _panel_instance.bus(&_bus_instance);
    }

    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs = -1; //37;
        cfg.pin_rst = -1;
        cfg.pin_busy = -1;
        cfg.offset_rotation = 0;
        cfg.readable = true;
        cfg.invert = false;
        cfg.rgb_order = false;
        cfg.dlen_16bit = true;
        cfg.memory_height = screenWidth;
        cfg.panel_width = screenHeight;
        cfg.panel_height = screenWidth;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.bus_shared = true; //false;

        _panel_instance.config(cfg);
    }

    {
        auto cfg = _light_instance.config();

        cfg.pin_bl = 45;
        cfg.invert = false;
        cfg.freq = 44100;
        cfg.pwm_channel = 7;

        _light_instance.config(cfg);
        _panel_instance.light(&_light_instance);
    }
    setPanel(&_panel_instance);
}

/// Detects and configures the touch panel during initialization;
bool DisplayUXESP32::LGFX::init_impl(bool use_reset, bool use_clear)
{
    if (_touch_instance_ptr == nullptr)
    {
        lgfx::ITouch::config_t cfg;
        lgfx::i2c::init(I2C_PORT_NUM, I2C_PIN_SDA, I2C_PIN_SCL);
        if (lgfx::i2c::beginTransaction(I2C_PORT_NUM, 0x38, 400000, false).has_value()
            && lgfx::i2c::endTransaction(I2C_PORT_NUM).has_value())
        {
            _touch_instance_ptr = new lgfx::Touch_FT5x06();
            cfg = _touch_instance_ptr->config();
            cfg.i2c_addr = 0x38;
            cfg.x_max = this->screenHeight;
            cfg.y_max = this->screenWidth;
        }
        else
            if (lgfx::i2c::beginTransaction(I2C_PORT_NUM, 0x48, 400000, false).has_value()
                && lgfx::i2c::endTransaction(I2C_PORT_NUM).has_value())
            {
                _touch_instance_ptr = new lgfx::Touch_NS2009();
                cfg = _touch_instance_ptr->config();
                cfg.i2c_addr = 0x48;
                cfg.x_min = 368;
                cfg.y_min = 212;
                cfg.x_max = 3800;
                cfg.y_max = 3800;
            }
        if (_touch_instance_ptr != nullptr)
        {
            cfg.i2c_port = I2C_PORT_NUM;
            cfg.pin_sda = I2C_PIN_SDA;
            cfg.pin_scl = I2C_PIN_SCL;
            cfg.pin_int = I2C_PIN_INT;
            cfg.freq = 400000;
            cfg.bus_shared = false;
            _touch_instance_ptr->config(cfg);
            _panel_instance.touch(_touch_instance_ptr);
        }
    }
    return lgfx::LGFX_Device::init_impl(use_reset, use_clear);
}
*/
DisplayUXESP32::DisplayUXESP32(const unsigned int width, const unsigned int height)
    : winston::hal::DisplayUX(width, height), winston::Shared_Ptr<DisplayUXESP32>(),
    //lcd(width, height),
    lcd(), touch(),
    lvBufferSize(width * height / 10 * (LV_COLOR_DEPTH / 8)), lvDisplay(), lvInput(), lvBuffer(nullptr), lastTick(0)
{

}

const winston::Result DisplayUXESP32::init()
{
    //this->lcd.init();
    this->lcd.begin(DISPLAY_MAKERFABS_S3);
    this->lcd.setRotation(3);

    constexpr int I2C_PIN_SDA = 38;
    constexpr int I2C_PIN_SCL = 39;
    int error = touch.init(I2C_PIN_SDA, I2C_PIN_SCL);

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
/*
void DisplayUXESP32::calibrateTouch(void)
{
    auto cfg = _touch->config();
    /*
    uint16_t parameters[8] =
    { cfg.x_min, cfg.y_min
    , cfg.x_min, cfg.y_max
    , cfg.x_max, cfg.y_min
    , cfg.x_max, cfg.y_max };

    cfg.x_min = 368;
    cfg.y_min = 212;
    cfg.x_max = 3800;
    cfg.y_max = 3800;
    *
    uint16_t parameters[8] =
    { 368, 212
    , 368, 3800
    , 3800, 212
    , 3800, 3800 };

    uint32_t vect[6] = { 0,0,0,0,0,0 };
    float mat[3][3] = { {0,0,0},{0,0,0},{0,0,0} };
    float a;

    // swapped
    auto w = this->height;
    auto h = this->width;

    for (int i = 0; i < 4; ++i) {
        int32_t tx = (i & 2) ? (w - 1) : 0;
        int32_t ty = (i & 1) ? (h - 1) : 0;
        int32_t px = parameters[i * 2];
        int32_t py = parameters[i * 2 + 1];
        a = px * px;
        mat[0][0] += a;
        a = px * py;
        mat[0][1] += a;
        mat[1][0] += a;
        a = px;
        mat[0][2] += a;
        mat[2][0] += a;
        a = py * py;
        mat[1][1] += a;
        a = py;
        mat[1][2] += a;
        mat[2][1] += a;
        mat[2][2] += 1;

        vect[0] += px * tx;
        vect[1] += py * tx;
        vect[2] += tx;
        vect[3] += px * ty;
        vect[4] += py * ty;
        vect[5] += ty;
    }

    {
        for (int k = 0; k < 3; ++k)
        {
            float t = mat[k][k];
            for (int i = 0; i < 3; ++i) mat[k][i] /= t;

            mat[k][k] = 1 / t;
            for (int j = 0; j < 3; ++j)
            {
                if (j == k) continue;

                float u = mat[j][k];

                for (int i = 0; i < 3; ++i)
                {
                    if (i != k) mat[j][i] -= mat[k][i] * u;
                    else mat[j][i] = -u / t;
                }
            }
        }
    }

    float v0 = vect[0];
    float v1 = vect[1];
    float v2 = vect[2];
    touchCalibrationAffine[0] = mat[0][0] * v0 + mat[0][1] * v1 + mat[0][2] * v2;
    touchCalibrationAffine[1] = mat[1][0] * v0 + mat[1][1] * v1 + mat[1][2] * v2;
    touchCalibrationAffine[2] = mat[2][0] * v0 + mat[2][1] * v1 + mat[2][2] * v2;
    float v3 = vect[3];
    float v4 = vect[4];
    float v5 = vect[5];
    touchCalibrationAffine[3] = mat[0][0] * v3 + mat[0][1] * v4 + mat[0][2] * v5;
    touchCalibrationAffine[4] = mat[1][0] * v3 + mat[1][1] * v4 + mat[1][2] * v5;
    touchCalibrationAffine[5] = mat[2][0] * v3 + mat[2][1] * v4 + mat[2][2] * v5;
}

void DisplayUXESP32::convertRawTouch(unsigned int& x, unsigned int& y)
{
    auto r = 3;
    if (_touch) {
        auto offset = _touch->config().offset_rotation;
        r = ((r + offset) & 3) | ((r & 4) ^ (offset & 4));
    }
    bool vflip = (1 << r) & 0b10010110; // r 1,2,4,7

    for (size_t idx = 0; idx < count; ++idx)
    {
        int32_t tx = (touchCalibrationAffine[0] * (float)x + touchCalibrationAffine[1] * (float)y) + touchCalibrationAffine[2];
        int32_t ty = (touchCalibrationAffine[3] * (float)x + touchCalibrationAffine[4] * (float)y) + touchCalibrationAffine[5];
        if (r)
        {
            if (r & 1) { std::swap(tx, ty); }
            if (r & 2) { tx = (_width - 1) - tx; }
            if (vflip) { ty = (_height - 1) - ty; }
        }
        x = tx;
        y = ty;
    }
}
*/
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
/*
int DisplayUXESP32::ft6236_readTouchReg(int reg)
{
    int data = 0;
    Wire.beginTransmission(TOUCH_I2C_ADD);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(TOUCH_I2C_ADD, 1);
    if (Wire.available())
    {
        data = Wire.read();
    }
    return data;
}

int DisplayUXESP32::ft6236_getTouchPointX()
{
    int XL = 0;
    int XH = 0;

    XH = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_XH);
    //Serial.println(XH >> 6,HEX);
    if (XH >> 6 == 1)
        return -1;
    XL = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_XL);

    return ((XH & 0x0F) << 8) | XL;
}

int DisplayUXESP32::ft6236_getTouchPointY()
{
    int YL = 0;
    int YH = 0;

    YH = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_YH);
    YL = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_YL);

    return ((YH & 0x0F) << 8) | YL;
}

int DisplayUXESP32::ft6236_getPos(int pos[2])
{
    int XL = 0;
    int XH = 0;
    int YL = 0;
    int YH = 0;

    XH = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_XH);
    if (XH >> 6 == 1)
    {
        pos[0] = -1;
        pos[1] = -1;
        return 0;
    }
    XL = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_XL);
    YH = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_YH);
    YL = DisplayUXESP32::ft6236_readTouchReg(TOUCH_REG_YL);

    pos[0] = ((XH & 0x0F) << 8) | XL;
    pos[1] = ((YH & 0x0F) << 8) | YL;
    return 1;
}
*/