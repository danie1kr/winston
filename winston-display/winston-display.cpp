// winston-main.cpp : This file contains the 'main' function. Program execution begins and ends there.

/*
    lovyangfx + multiple files: (406,37 secs)
    bb_lcd & bb_touch: (364,96 secs)
*/

#include "winston-display.h"
#include "Cinema.h"
#include "../winston-display/winston-secrets.h"

#ifndef WINSTON_SECRET
pragma error("WINSTON_SECRET not found, see winston-secrets.h.template and create winston-secrets.h from it")
#endif

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
winston::TimePoint lastWebsocketConnectionCheck;

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
#ifndef WINSTON_PLATFORM_WIN_x64
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        uxUpdateWifiLED(false);
        uxUpdateWifiIP("not connected");
        winston::logger.info("WiFi disconnect");
        }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        uxUpdateWifiLED(true);
        uxUpdateWifiIP(WiFi.localIP().toString().c_str());
        winston::logger.info("WiFi connected", WiFi.localIP().toString().c_str());
        }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    WiFi.onEvent([](WiFiEvent_t event) {
        Serial.printf("[WiFi-event] event: %d\n", event);

        switch (event) {
        case ARDUINO_EVENT_WIFI_READY:
            Serial.println("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            Serial.println("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            Serial.println("STA IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP6:
            Serial.println("Ethernet IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
        default: break;
        }
    });

#endif
    winston::hal::init();
    winston::hal::text("Hello from Winston!");


#ifdef WINSTON_PLATFORM_WIN_x64
    std::srand(inMilliseconds(winston::hal::now().time_since_epoch()));
#else
    std::srand(::micros());
#endif

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

    webSocketClient.init([](WebSocketClient::Client& client, const std::string& message) {
        JsonDocument msg;
        deserializeJson(msg, message);
        JsonObject obj = msg.as<JsonObject>();
        std::string op("\"");
        op.append(obj["op"].as<std::string>());
        op.append("\"");
        JsonObject data = obj["data"];

        if (std::string("\"microLayout\"").compare(op) == 0)
        {
            rml.tracks.clear();
            rml.turnouts.clear();

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
            const winston::RailwayMicroLayout::Bounds screen(offset, offset, 480 - 2*offset, 320 - 2 * offset);
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
        else if (std::string("\"storyLineText\"").compare(op) == 0)
        {
            std::string text = data["text"].as<std::string>();
            uxUpdateStorylineText(text);
        }
    });

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
            settings.target = target;
            saveSettings(storageSettings);
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
        },
        [](const std::string reply) -> const winston::Result {
            std::string json("");
            {
                JsonDocument msg;
                msg["op"] = "storylineReply";
                auto data = msg["data"].to<JsonObject>();
                data["reply"] = reply;
                serializeJson(msg, json);
            }
            webSocketClient.send(json);
            return winston::Result::OK;
        },
        [](){
            std::string ip = "localhost";
            switch (settings.target)
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

            webSocketClient.connect(uri);

            {
                JsonDocument msg;
                msg["op"] = "getRailwayMicroLayout";
                std::string json("");
                serializeJson(msg, json);
                webSocketClient.send(json);
            }
            {
                JsonDocument msg;
                msg["op"] = "getStorylineText";
                std::string json("");
                serializeJson(msg, json);
                webSocketClient.send(json);
            }
            eventLooper.order(winston::Command::make([](const winston::TimePoint& created) -> const winston::State
                {
#ifdef WINSTON_PLATFORM_ESP32
                    if (winston::hal::now() - lastWebsocketConnectionCheck > 2000)
#else
                    if (winston::hal::now() - lastWebsocketConnectionCheck > 2000ms)
#endif
                    {
                        lastWebsocketConnectionCheck = winston::hal::now();
                        if (!webSocketClient.connected())
                        {
                            uxScreenRailwayShowButtonReconnect();
                            uxScreenRailwayClear();
                        }
                    }
                    return winston::State::Delay;
                }, __PRETTY_FUNCTION__));

            return winston::Result::OK;
        }
    );


#ifdef WINSTON_PLATFORM_WIN_x64
    eventLooper.order(winston::Command::make([](const winston::TimePoint& created) -> const winston::State
        {
            if (winston::hal::now() - created > 2000ms)
            {
                uxUpdateWifiLED(true);
                uxUpdateWifiIP("connected");
                return winston::State::Finished;
            }
            return winston::State::Delay;
        }, __PRETTY_FUNCTION__));
#endif

    cinema.init();
    showUX(currentScreen);
    // setup
    winston::hal::text("Setup complete!");
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
            currentScreen = Screen::Menu;
            showUX(currentScreen);
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

    eventLooper.loop();
    webSocketClient.loop();
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
