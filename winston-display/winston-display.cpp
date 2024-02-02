// winston-main.cpp : This file contains the 'main' function. Program execution begins and ends there.

/*
    lovyangfx + multiple files: (406,37 secs)
    bb_lcd & bb_touch: (364,96 secs)
*/

#include "winston-display.h"
#include "Cinema.h"
#include "../winston-display/winston-secrets.h"

#include "../libwinston/external/ArduinoJson-v7.0.1.h"

#ifdef WINSTON_PLATFORM_WIN_x64
#include "../winston/winston-hal-x64.h"
#endif
#ifdef WINSTON_PLATFORM_ESP32
#include "../winston-teensy/winston-hal-teensy.h"
#include "winston-display-hal-esp32.h"
#include "winston-display-hal-esp32-websocketclient.hpp"
#endif

Screen currentScreen = Screen::Cinema;
DisplayUX::Shared display = DisplayUX::make(480, 320);

#ifdef WINSTON_PLATFORM_ESP32
Cinema cinema(SD, display);
#else
Cinema cinema(display);
#endif

WebSocketClient webSocketClient;
winston::RailwayMicroLayout rml;

Settings settings;
Storage::Shared storageSettings;

winston::EventLooper eventLooper;

void saveSettings(Storage::Shared &storageSettings)
{
    storageSettings->write(0, settings.brightness);
    storageSettings->write(1, (unsigned char)settings.target);
    storageSettings->write(2, (unsigned char)settings.screen);
    storageSettings->sync();
}

void loadSettings(Storage::Shared &storageSettings)
{
    storageSettings->read(0, settings.brightness);
    storageSettings->read(1, (unsigned char&)settings.target);
    storageSettings->read(2, (unsigned char&)settings.screen);
}

void applySettings()
{
    display->brightness(settings.brightness);
    currentScreen = settings.screen;
    // target
}

#ifndef WINSTON_PLATFORM_WIN_x64/*
int jpegDraw(JPEGDRAW* pDraw)
{
    display->lcd.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
    return 1;
}

File globalFile;
void* jpegOpen(const char* filename, int32_t* size) {
    globalFile = SD.open(filename);
    *size = globalFile.size();
    return &globalFile;
}

void jpegClose(void* handle) {
    if (globalFile) globalFile.close();
}

int32_t jpegRead(JPEGFILE* handle, uint8_t* buffer, int32_t length) {
    if (!globalFile) return 0;
    return globalFile.read(buffer, length);
}

int32_t jpegSeek(JPEGFILE* handle, int32_t position) {
    if (!globalFile) return 0;
    return globalFile.seek(position);
}*/
#endif

void winston_setup()
{
    winston::hal::init();
    winston::hal::text("Hello from Winston!"_s);
    std::srand((unsigned int)(24));// inMilliseconds(winston::hal::now().time_since_epoch())));

    display->init();
    display->displayLoadingScreen();
    /*
#ifndef WINSTON_PLATFORM_WIN_x64
    const char loadingScreenJPEG[] = "/loading.jpg";
    jpeg.open(loadingScreenJPEG, jpegOpen, jpegClose, jpegRead, jpegSeek, jpegDraw);
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
    jpeg.decode(0, 0, 0);
    jpeg.close();
#endif*/

    storageSettings = Storage::make("winston-display.settings", 3);
    storageSettings->init();
    loadSettings(storageSettings);
    applySettings();

    setupUX(display,
        [](unsigned char value) -> winston::Result
        {
            settings.brightness = value;
            saveSettings(storageSettings);
            return display->brightness(value);
        },
        []() -> unsigned char { return display->brightness(); },
        [](Screen screen) -> winston::Result
        {
            currentScreen = screen;
            settings.screen = screen;
            saveSettings(storageSettings);
            if (currentScreen == Screen::Cinema)
                return winston::Result::OK;
            else
                return showUX(screen);
        },
        [&](WinstonTarget target) -> winston::Result
        {
            std::string ip = "localhost";
            switch (target)
            {
            case WinstonTarget::BlackCanaryLAN:
                ip = WINSTON_IP_BLACKCANARY_LAN;
                break;
            case WinstonTarget::BlackCanaryWifi:
                ip = WINSTON_IP_BLACKCANARY_WIFI;
                break;
            case WinstonTarget::Teensy:
                ip = WINSTON_IP_TEENSY;
                break;
            default:
                break;
            }
            winston::URI uri(ip);

            webSocketClient.init([](WebSocketClient::Client& client, const std::string message) {
                JsonDocument msg;
                deserializeJson(msg, message);
                JsonObject obj = msg.as<JsonObject>();
                std::string op("\"");
                op.append(obj["op"].as<std::string>());
                op.append("\"");
                JsonObject data = obj["data"];

                if (std::string("\"microLayout\"").compare(op) == 0)
                {
                    JsonDocument layoutDoc;
                    std::string layoutJSON = data["layout"].as<std::string>();
                    deserializeJson(layoutDoc, layoutJSON);
                    JsonArray tracks = layoutDoc["tracks"].as<JsonArray>();
                    JsonArray turnouts = layoutDoc["turnouts"].as<JsonArray>();
                    JsonObject bounds = layoutDoc["bounds"];
                    rml.bounds.min.x = bounds["min"]["x"].as<int32_t>();
                    rml.bounds.min.y = bounds["min"]["y"].as<int32_t>();
                    rml.bounds.max.x = bounds["max"]["x"].as<int32_t>();
                    rml.bounds.max.y = bounds["max"]["y"].as<int32_t>();

                    for (JsonVariant track : tracks)
                    {
                        winston::RailwayMicroLayout::Track t;
                        JsonArray points = track.as<JsonArray>();
                        for (size_t i = 0; i < points.size(); i += 2)
                        {
                            winston::RailwayMicroLayout::Point p{ points[i + 0].as<int32_t>(), points[i + 1].as<int32_t>() };
                            t.push_back(p);
                        }
                        rml.tracks.push_back(t);
                    }

                    for (JsonVariant turnout : turnouts)
                    {
                        winston::RailwayMicroLayout::Turnout t;
                        t.id = turnout["id"].as<std::string>();
                        auto connections = turnout["connections"].as<JsonArray>();
                        for (JsonVariant connection : connections)
                        {
                            winston::RailwayMicroLayout::Connection c;
                            c.connection = connection["connection"].as<std::string>();
                            auto points = connection["p"].as<JsonArray>();
                            c.p[0].x = points[0].as<int32_t>();
                            c.p[0].y = points[1].as<int32_t>();
                            c.p[1].x = points[2].as<int32_t>();
                            c.p[1].y = points[3].as<int32_t>();
                            t.connections.push_back(c);
                        }
                        rml.turnouts.push_back(t);
                    }
                    int offset = 8;
                    const winston::RailwayMicroLayout::Bounds screen(offset, offset, 480 - 4 * offset, 320 - 2 * offset);
                    rml.scale(screen);
                    uxUpdateRailwayLayout(rml, [](const std::string turnout) -> const winston::Result {
                        eventLooper.order(winston::Command::make([turnout](const winston::TimePoint& created) -> const winston::State {
                            std::string json("");
                            {
                                JsonDocument msg;
                                msg["op"] = "doTurnoutToggle";
                                auto data = msg["data"].to<JsonObject>();
                                data["id"] = turnout;
                                serializeJson(msg, json);
                            }
                            webSocketClient.send(json);
                            return winston::State::Finished;
                            }, __PRETTY_FUNCTION__));
                        return winston::Result::OK;
                        });

                    unsigned int idx = 1;
                    for (const auto& turnout : rml.turnouts)
                    {
                        eventLooper.order(winston::Command::make([turnout, idx](const winston::TimePoint& created) -> const winston::State
                        {
                            if (created + toMilliseconds(idx * 50) > winston::hal::now())
                                    return winston::State::Delay;

                            std::string json("");
                            {
                                JsonDocument msg;
                                msg["op"] = "getTurnoutState";
                                auto data = msg["data"].to<JsonObject>();
                                data["id"] = turnout.id;
                                serializeJson(msg, json);
                            }
                            webSocketClient.send(json);
                            return winston::State::Finished;
                        }, __PRETTY_FUNCTION__));
                        idx++;
                    }
                }
                else if (std::string("\"turnoutState\"").compare(op) == 0)
                {
                    std::string track = data["id"].as<std::string>();
                    int state = data["state"].as<int>();
                    bool locked = data["direction"].as<bool>();
                    uxUpdateTurnout(track, state, locked);
                }
                
                }, uri);

            settings.target = target;
            saveSettings(storageSettings);

            JsonDocument msg;
            msg["op"] = "getRailwayMicroLayout";
            std::string json("");
            serializeJson(msg, json);
            webSocketClient.send(json);
            return winston::Result::OK;
        },
        []() -> std::string
        {
#ifdef WINSTON_PLATFORM_ESP32
            if (WiFi.status() != WL_CONNECTED)
                return std::string("not connected");
            else
                return std::string(WiFi.localIP().toString().c_str());
#else
            return std::string("not connected");
#endif
        }
    );

    cinema.init();
    showUX(currentScreen);
    // setup
    winston::hal::text("Setup complete!"_s);
}

void lvgl_loop()
{
    auto timeTillNext = display->tick();
    winston::hal::delay(timeTillNext);
}

unsigned int consecutiveCinemaTouches = 0;
void cinema_loop()
{
    cinema.play();
#ifndef WINSTON_PLATFORM_WIN_x64
    unsigned int x, y;
    if (display->getTouch(x, y))
    {
        ++consecutiveCinemaTouches;
        if (consecutiveCinemaTouches > 24)
        {
            winston::logger.info("touch on", x, ", ", y);
            currentScreen = Screen::Settings;
        }
    }
    else
        consecutiveCinemaTouches = 0;
#endif
}

void winston_loop()
{
#ifndef WINSTON_PLATFORM_WIN_x64
    if (currentScreen == Screen::Cinema)
        cinema_loop();
    else
#endif
        lvgl_loop();

    eventLooper.work();
    webSocketClient.step();
}

#ifdef WINSTON_PLATFORM_WIN_x64
int main()
{
    // setup
    winston_setup();

    // and loop
    while (true)
        winston_loop();
}
#endif
