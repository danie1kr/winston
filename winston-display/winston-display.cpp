// winston-main.cpp : This file contains the 'main' function. Program execution begins and ends there.


#include "winston-display.h"
#include "Cinema.h"

#ifdef WINSTON_PLATFORM_WIN_x64
#include "winston-display-hal-x64.h"
#endif
#ifdef WINSTON_PLATFORM_ESP32
#include "winston-display-hal-esp32.h"
#endif

#include "../libwinston/external/ArduinoJson-v6.21.3.h"

Screen currentScreen = Screen::Cinema;
DisplayUX::Shared display = DisplayUX::make(480, 320);
Cinema cinema(display);

WebSocketClient webSocketClient;
winston::RailwayMicroLayout rml;

void winston_setup()
{
    winston::hal::init();
    winston::hal::text("Hello from Winston!"_s);
    std::srand((unsigned int)(inMilliseconds(winston::hal::now().time_since_epoch())));

    // setup
    winston::hal::text("Setup complete!"_s);

    //display = DisplayUX::make(480, 320);
    display->init();

    setupUX(display,
        [](unsigned char value) -> winston::Result
        {
            return display->brightness(value);
        },
        []() -> unsigned char { return display->brightness(); },
        [](Screen screen) -> winston::Result
        {
            currentScreen = screen;
            if (currentScreen == Screen::Cinema)
                return winston::Result::OK;
            else
                return showUX(screen);
        },
        [](WinstonTarget target) -> winston::Result
        {
            webSocketClient.init([](ConnectionWSPP& client, const std::string message) {
                DynamicJsonDocument msg(32 * 1024);
                deserializeJson(msg, message);
                JsonObject obj = msg.as<JsonObject>();
                std::string op("\"");
                op.append(obj["op"].as<std::string>());
                op.append("\"");
                JsonObject data = obj["data"];

                if (std::string("\"microLayout\"").compare(op) == 0)
                {
                    DynamicJsonDocument layoutDoc(64*1024);
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
                        for(size_t i = 0; i < points.size(); i += 2)
                        {
                            winston::RailwayMicroLayout::Point p{ points[i+0].as<int32_t>(), points[i+1].as<int32_t>() };
                            t.push_back(p);
                        }
                        rml.tracks.push_back(t);
                    }
                    rml.scale(480, 320);
                    uxUpdateRailwayLayout(rml);
                }
                
                }, target == WinstonTarget::BlackCanary ? "ws://192.168.188.57:8080" : "ws://192.168.188.133:8080");

            DynamicJsonDocument msg(256);
            msg["op"] = "getRailwayMicroLayout";
            std::string json("");
            serializeJson(msg, json);
            webSocketClient.send(json);
            return winston::Result::OK;
        },
        []() -> std::string
        {
            return std::string("not connected");
        }
    );

    cinema.collectMovies();
}

void lvgl_loop()
{
    auto timeTillNext = display->tick();
    winston::hal::delay(timeTillNext);
}

void cinema_loop()
{
    cinema.play();
    /*
    if(touch)
        displayMode = DisplayMode::LVGL;
    */
}

void winston_loop()
{
#ifndef WINSTON_PLATFORM_WIN_x64
    if (currentScreen == Screen::Cinema)
        cinema_loop();
    else
#endif
        lvgl_loop();

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
