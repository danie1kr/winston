#include "winston-display-hal-esp32.h"

DisplayUXESP32::LGFX DisplayUXESP32::lcd(480, 320);
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

DisplayUXESP32::DisplayUXESP32(const unsigned int width, const unsigned int height)
    : winston::hal::DisplayUX(width, height),
    lvBufferSize(width * 10), lvDisplayDraw(), lvBuffer(nullptr)
{

}

const winston::Result DisplayUXESP32::init()
{
    lvBuffer = (lv_color_t*)malloc(sizeof(lv_color_t) * lvBufferSize);
    if (!lvBuffer)
        return winston::Result::OutOfMemory;

    lv_init();
    lv_disp_draw_buf_init(&lvDisplayDraw, lvBuffer, NULL, lvBufferSize);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    disp_drv.flush_cb = [](lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t
        * color_p) {
            uint32_t w = (area->x2 - area->x1 + 1);
            uint32_t h = (area->y2 - area->y1 + 1);
            lcd.pushImageDMA(area->x1, area->y1, w, h, &color_p->full);
            lv_disp_flush_ready(disp);
        };
    disp_drv.draw_buf = &lvDisplayDraw;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = [](lv_indev_drv_t* drv, lv_indev_data_t* data) {
        uint16_t touchX, touchY;

        data->state = LV_INDEV_STATE_REL;

        if (lcd.getTouch(&touchX, &touchY))
        {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = touchX;
            data->point.y = touchY;
        }
        };
    lv_indev_drv_register(&indev_drv);

    return winston::Result::OK;
}

const winston::Result DisplayUXESP32::setCursor(unsigned int x, unsigned int y)
{
    this->lcd.setCursor(x, y);
    return winston::Result::OK;
}

const bool DisplayUXESP32::getTouch(unsigned int& x, unsigned int& y)
{
    return this->lcd.getTouch(&x, &y) != 0;
}

const winston::Result DisplayUXESP32::draw(unsigned int x, unsigned int y, unsigned int w, unsigned int h, void* data)
{
    DisplayUXESP32::lcd.pushImage(x, y, w, h, data);
    return winston::Result::OK;
}

const winston::Result DisplayUXESP32::brightness(unsigned char value)
{
    return winston::Result::OK;
}

const unsigned char DisplayUXESP32::brightness()
{
    return 24;
}

const unsigned int DisplayUXESP32::tick()
{
    return lv_timer_handler();
}

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
    XL = ft6236_readTouchReg(TOUCH_REG_XL);

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