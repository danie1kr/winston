/*
 Name:		winston_display.ino
 Created:	12/21/2023 1:06:10 PM
 Author:	daniel
*/
#include <vector>
#include <string>

#define SPI_DRIVER_SELECT 0

#define LGFX_USE_V1

//#define SHOW_TIMINGS

#include <SPI.h>
#include <SdFat.h>
#include <LovyanGFX.hpp>
#include <driver/i2c.h>
#include <JPEGDEC.h>

#include <lvgl.h>

#include <ArduinoJson.h>
/*
class MySpiClass : public SdSpiBaseClass {
 public:
 MySpiClass() : SdSpiBaseClass() {
    m_spiSettings = SPISettings(40000000, MSBFIRST, SPI_MODE0);
 }
  // Activate SPI hardware with correct speed and mode.
  void activate() { SPI.beginTransaction(m_spiSettings); }
  // Initialize the SPI bus.
  void begin(SdSpiConfig config) {
    (void)config;
    //SPI.begin();
  }
  // Deactivate SPI hardware.
  void deactivate() { SPI.endTransaction(); }
  // Receive a byte.
  uint8_t receive() { return SPI.transfer(0XFF); }
  // Receive multiple bytes.
  // Replace this function if your board has multiple byte receive.
  uint8_t receive(uint8_t* buf, size_t count) {
    for (size_t i = 0; i < count; i++) {
      buf[i] = SPI.transfer(0XFF);
    }
    return 0;
  }
  // Send a byte.
  void send(uint8_t data) { SPI.transfer(data); }
  // Send multiple bytes.
  // Replace this function if your board has multiple byte send.
  void send(const uint8_t* buf, size_t count) {
    for (size_t i = 0; i < count; i++) {
      SPI.transfer(buf[i]);
    }
  }
  // Save SPISettings for new max SCK frequency
  void setSckSpeed(uint32_t maxSck) {
    m_spiSettings = SPISettings(maxSck, MSBFIRST, SPI_MODE0);
  }

 private:
  SPISettings m_spiSettings;
} mySpi;
*/
/*
#include <Wire.h>

#define TOUCH_I2C_ADD 0x38

#define TOUCH_REG_XL 0x04
#define TOUCH_REG_XH 0x03
#define TOUCH_REG_YL 0x06
#define TOUCH_REG_YH 0x05

int readTouchReg(int reg);

int getTouchPointX();

int getTouchPointY();

int ft6236_pos(int pos[2]);
const int i2c_touch_addr = TOUCH_I2C_ADD;
#define get_pos ft6236_pos
*/
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

SdFat SD;
File fileJPEG;

#include <Wire.h>

#define TOUCH_I2C_ADD 0x38

#define TOUCH_REG_XL 0x04
#define TOUCH_REG_XH 0x03
#define TOUCH_REG_YL 0x06
#define TOUCH_REG_YH 0x05

int readTouchReg(int reg);
int getTouchPointX();
int getTouchPointY();
int ft6236_pos(int pos[2]);

const int i2c_touch_addr = TOUCH_I2C_ADD;
#define get_pos ft6236_pos

int readTouchReg(int reg)
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

int getTouchPointX()
{
    int XL = 0;
    int XH = 0;

    XH = readTouchReg(TOUCH_REG_XH);
    //Serial.println(XH >> 6,HEX);
    if (XH >> 6 == 1)
        return -1;
    XL = readTouchReg(TOUCH_REG_XL);

    return ((XH & 0x0F) << 8) | XL;
}

int getTouchPointY()
{
    int YL = 0;
    int YH = 0;

    YH = readTouchReg(TOUCH_REG_YH);
    YL = readTouchReg(TOUCH_REG_YL);

    return ((YH & 0x0F) << 8) | YL;
}

int ft6236_pos(int pos[2])
{
    int XL = 0;
    int XH = 0;
    int YL = 0;
    int YH = 0;

    XH = readTouchReg(TOUCH_REG_XH);
    if (XH >> 6 == 1)
    {
        pos[0] = -1;
        pos[1] = -1;
        return 0;
    }
    XL = readTouchReg(TOUCH_REG_XL);
    YH = readTouchReg(TOUCH_REG_YH);
    YL = readTouchReg(TOUCH_REG_YL);

    pos[0] = ((XH & 0x0F) << 8) | XL;
    pos[1] = ((YH & 0x0F) << 8) | YL;
    return 1;
}

const unsigned int screenWidth = 480;
const unsigned int screenHeight = 320;
const unsigned int lvBufferSize = screenWidth * 10;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t* buf;//[screenWidth * screenHeight];

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
    bool init_impl(bool use_reset, bool use_clear) override
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
                cfg.x_max = screenHeight;
                cfg.y_max = screenWidth;
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

public:

    LGFX(void)
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
};

static LGFX lcd;
static JPEGDEC jpeg;

int SD_init();

size_t largestJPEGFileSize = 0;
uint8_t* jpegBuffer;

int drawJPEG(JPEGDRAW* pDraw)
{
    lcd.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
    return 1;
}

const unsigned int movieFrameStart = 1;
struct Movie
{
    const std::string path;
    const unsigned int frames;

    Movie(const std::string path, const unsigned int frames) : path(path), frames(frames) { };
};
std::vector<Movie> movies;
// the setup function runs once when you press reset or power the board
void setup() {
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_BLK, OUTPUT);

    digitalWrite(LCD_CS, LOW);
    digitalWrite(LCD_BLK, HIGH);

    Serial.begin(115200);
    delay(500);
    Serial.println("hello from board");
    Serial.flush();

    lcd.init();
    lcd.fillScreen(TFT_BLUE);
    lcd.setTextColor(TFT_YELLOW);
    lcd.setTextSize(2);
    lcd.setCursor(0, 0);
    lcd.print("Makerfabs ESP32-S3");
    lcd.setCursor(0, 16);
    lcd.print("Parallel TFT with Touch");
    lcd.setCursor(0, 32);
    lcd.setRotation(3);

    //I2C init
    Wire.begin(I2C_SDA, I2C_SCL);
    byte error, address;

    Wire.beginTransmission(i2c_touch_addr);
    error = Wire.endTransmission();


    lcd.setCursor(0, 48);
    if (error == 0)
    {
        Serial.print("I2C device found at address 0x");
        Serial.print(i2c_touch_addr, HEX);
        Serial.println("  !");
        lcd.print("TOUCH INIT OVER");
    }
    else
    {
        Serial.print("Unknown error at address 0x");
        Serial.println(i2c_touch_addr, HEX);
        lcd.print("ERROR:   TOUCH");
    }

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    if (SD_init())
    {
        lcd.print("ERROR:   SD");
    }
    else
        lcd.print("SD INIT OVER");

    lcd.setCursor(0, 48);
    jpegBuffer = (uint8_t*)malloc(sizeof(uint8_t) * largestJPEGFileSize);
    if (!jpegBuffer)
    {
        lcd.print("JPEG alloc error");
        while (true);
    }

    lcd.setCursor(0, 64);
    buf = (lv_color_t*)malloc(sizeof(lv_color_t) * lvBufferSize);
    if (!buf)
    {
        lcd.print("LVGL Buffer alloc error");
        while (true);
    }

    setupLVGL();

    log_d("Total heap: %d", ESP.getHeapSize());
    log_d("Free heap: %d", ESP.getFreeHeap());
    log_d("Total PSRAM: %d", ESP.getPsramSize());
    log_d("Free PSRAM: %d", ESP.getFreePsram());
}

lv_obj_t* lvglScreenSettings, * lvglScreenRailway;
void setupLVGL()
{
    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, lvBufferSize);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = [](lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t
        * color_p) {
            uint32_t w = (area->x2 - area->x1 + 1);
            uint32_t h = (area->y2 - area->y1 + 1);
            lcd.pushImageDMA(area->x1, area->y1, w, h, &color_p->full);
            lv_disp_flush_ready(disp);
        };
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = [](lv_indev_drv_t* drv, lv_indev_data_t* data) {
        /*TS_Point touchPoint = touch.getPoint();
        if(touchPoint.x > 0 || touchPoint.y > 0){
          data->state = LV_INDEV_STATE_PR;
          data->point.x = 240 - touchPoint.x;
          data->point.y = 320 - touchPoint.y;
        }
        else {
          data->state = LV_INDEV_STATE_REL;
        }*/
        uint16_t touchX, touchY;

        data->state = LV_INDEV_STATE_REL;

        if (lcd.getTouch(&touchX, &touchY))
        {
            data->state = LV_INDEV_STATE_PR;

            /*Set the coordinates*/
            data->point.x = touchX;
            data->point.y = touchY;
        }
        };
    lv_indev_drv_register(&indev_drv);
    /*
      lv_obj_t *label = lv_label_create(lv_scr_act());
      lv_label_set_text(label, "Hello World!");
      lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    */
    unsigned int y = 0;
    const unsigned int ySize = 24;
    const unsigned int yInc = ySize + 8;

    // settings
    lvglScreenSettings = lv_obj_create(NULL);
    lv_obj_t* labelTitle = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelTitle, "Settings");
    //lv_label_set_align(labelTitle, LV_LABEL_ALIGN_CENTER);
    lv_obj_set_size(labelTitle, screenWidth - 80, ySize);
    lv_obj_set_pos(labelTitle, 0, y);
    y += yInc;

    lv_obj_t* labelSliderBacklight = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelSliderBacklight, "Backlight");
    //lv_label_set_align(labelSliderBacklight, LV_LABEL_ALIGN_CENTER);
    lv_obj_set_size(labelSliderBacklight, screenWidth - 80, ySize);
    lv_obj_set_pos(labelSliderBacklight, 0, y);
    y += yInc;

    lv_obj_t* sliderBacklight = lv_slider_create(lvglScreenSettings);
    lv_obj_set_size(sliderBacklight, screenWidth - 80, ySize);
    lv_obj_set_pos(sliderBacklight, 0, y);
    lv_obj_add_event_cb(sliderBacklight,
        [](lv_event_t* e) {
            lv_obj_t* slider = lv_event_get_target(e);
            auto value = lv_slider_get_value(slider);
            lv_obj_t* label = (lv_obj_t*)lv_event_get_user_data(e);
            std::string text = std::string("Backlight (") + std::to_string(value) + std::string(")");
            lcd.setBrightness(value);
            lv_label_set_text(label, text.c_str());
        },
        LV_EVENT_VALUE_CHANGED, labelSliderBacklight);
    /*
    lv_obj_t btnCinema = lv_btn_create(lvglScreenSettings, NULL);
    lv_obj_set_size(btnCinema, screenWidth - 80, ySize);
    lv_obj_set_pos(btnCinema, 0, y);
    lv_obj_set_event_cb(btnCinema, event_handler_btn);
    lv_obj_set_width(btn1, 70);
    lv_obj_set_height(btn1, 32);
    lv_obj_set_pos(btn1, 32, 100);
    // slider backlight
    // label: ip
    // led: wlan connected
    // dropdown: teensy or blackcanary
    // button cinema
    // button graph

  // graph
    // button settings
    // button cinema*/
}

int SD_init()
{
    if (!SD.begin(SD_CONFIG))
    {
        Serial.println("Card Mount Failed");
        SD.initErrorHalt(&Serial);
        return 1;
    }
    collectMovies(SD, "/movies");

    Serial.println("SD init over.");
    return 0;
}

std::string frameFileName(unsigned int frame)
{
    std::string fileName("");
    if (frame < 1000)
        fileName += "0";
    if (frame < 100)
        fileName += "0";
    if (frame < 10)
        fileName += "0";
    fileName += std::to_string(frame);
    fileName += ".jpg";

    return fileName;
}

void collectMovies(SdFat& fs, const char* dirname)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            char fileName[64];
            file.getName(fileName, 64);
            std::string path = std::string(dirname) + std::string("/") + std::string(fileName);
            unsigned int frames = movieFrameStart;
            /*  File movieDir = SD.open(path.c_str());
              std::string frame = frameFileName(movieFrameStart);
              Serial.print("Dir:"); Serial.print(path.c_str());
              while(movieDir.exists(frame.c_str()))
              {
                ++frames;
                frame = frameFileName(frames);
                Serial.print(" Frames: ") + Serial.println(frames);
              }
              movies.emplace_back(path, frames);
              */
            File frameFile;
            while (frameFile = file.openNextFile())
            {
                if (largestJPEGFileSize < frameFile.size())
                    largestJPEGFileSize = frameFile.size();
                ++frames;
            }
            Serial.print("Dir: "); Serial.print(path.c_str()); Serial.print(" Frames: "); Serial.print(frames); Serial.print(" Maximum size: "); Serial.println(largestJPEGFileSize);
            movies.emplace_back(path, frames);
        }
        file = root.openNextFile();
    }
}

bool movieRunning = false;
Movie* currentMovie;
File movieFile;
unsigned int currentFrame = movieFrameStart;
unsigned long lastFrame = 0;
const unsigned long FPSdelay = 1000 / 20;

enum class DisplayMode
{
    LVGL = 1,
    Cinema = 2
};
DisplayMode displayMode = DisplayMode::Cinema;

void lvgl()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}

void cinema()
{
    if (!movieRunning)
    {
        currentMovie = &movies[random(movies.size())];
        movieRunning = true;
        currentFrame = movieFrameStart;
    }

    if (movieRunning)
    {
        std::string jpegFileName = currentMovie->path + "/";
        //currentFrame = 666;
        if (currentFrame < 1000)
            jpegFileName += "0";
        if (currentFrame < 100)
            jpegFileName += "0";
        if (currentFrame < 10)
            jpegFileName += "0";
        jpegFileName += std::to_string(currentFrame);
        jpegFileName += ".jpg";
        File f = SD.open(jpegFileName.c_str());
        if (f)
        {
            size_t size = f.size();
            if (size < largestJPEGFileSize)
            {
#ifdef SHOW_TIMINGS
                const unsigned long tStart = millis();
#endif
                f.read(jpegBuffer, size);
#ifdef SHOW_TIMINGS
                const unsigned long tRead = millis();
#endif

                jpeg.openRAM((uint8_t*)jpegBuffer, size, drawJPEG);
                jpeg.setPixelType(RGB565_BIG_ENDIAN);
                jpeg.decode(0, 0, 0);
                jpeg.close();
#ifdef SHOW_TIMINGS
                const unsigned long tDraw = millis();
#endif
                f.close();

#ifdef SHOW_TIMINGS
                const unsigned long lastFrameDuration = millis() - lastFrame;
                std::string t = std::string("Read: ") + std::to_string(tRead - tStart) + std::string("ms");
                lcd.setCursor(0, 0);
                lcd.print(t.c_str());
                t = std::string("Draw: ") + std::to_string(tDraw - tRead) + std::string("ms");
                lcd.setCursor(0, 16);
                lcd.print(t.c_str());
#endif
            }

            ++currentFrame;

            if (currentFrame > currentMovie->frames)
                movieRunning = false;
        }
        else
            movieRunning = false;
    }

    lcd.setCursor(0, 0);

    //if(getTouchPointX() > -1 && getTouchPointY() > -1)
    int pos[2];
    if (ft6236_pos(pos))
    {
        movieRunning = false;
        displayMode = DisplayMode::LVGL;
    }
}

void loop() {

    switch (displayMode)
    {
    case DisplayMode::Cinema:
        cinema();
        break;
    case DisplayMode::LVGL:
        lvgl();
        break;
    default:
        break;
    }
}
