#include <string>

#include "winston-display.h"
#ifdef WINSTON_PLATFORM_WIN_x64
#include "external/lvgl/lvgl.h"
#else
#include <lvgl.h>
#endif

lv_obj_t* lvglScreenSettings;
lv_obj_t* lvglScreenRailway;

lv_obj_t* labelWifiIP;
lv_obj_t* ledWifi;

std::vector<lv_obj_t*> tracks;

void uxUpdateRailwayLayout(winston::RailwayMicroLayout& rml)
{
    for (auto& track : tracks)
        lv_obj_delete(track);

    tracks.clear();
    for (const auto& track : rml.tracks)
    {
        auto line = lv_line_create(lvglScreenRailway);
        lv_line_set_points(line, (lv_point_precise_t*) &track.front(), track.size());
        tracks.push_back(line);
    }
}

void uxUpdateWifiIP(const std::string ip)
{
    lv_label_set_text(labelWifiIP, ip.c_str());
}

void uxUpdateWifiLED(const bool on)
{
    if (on)
        lv_led_on(ledWifi);
    else
        lv_led_off(ledWifi);
}

void setupUX(winston::hal::DisplayUX::Shared display, 
    ValueCallbackUX<unsigned char> brightnessCallback, 
    ValueGetterUX<unsigned char> brightness, 
    ValueCallbackUX<Screen> gotoScreen,
    ValueCallbackUX<WinstonTarget> winstonTarget,
    ValueGetterUX<std::string> wifiIP)
{
    unsigned int y = 0;
    const unsigned int x = 8;
    const unsigned int ySize = 24;
    const unsigned int yInc = ySize + 8;

    static lv_style_t styleLabel;
    lv_style_init(&styleLabel);
    lv_style_set_text_align(&styleLabel, LV_TEXT_ALIGN_CENTER);

    // settings
    lvglScreenSettings = lv_obj_create(NULL);
    lv_obj_t* labelTitle = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelTitle, "Settings");
    lv_obj_set_size(labelTitle, display->width - 80, ySize);
    lv_obj_set_pos(labelTitle, 0, y);
    lv_obj_add_style(labelTitle, &styleLabel, 0);
    y += yInc;

    lv_obj_t* labelSliderBacklight = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelSliderBacklight, "Backlight");
    lv_obj_set_size(labelSliderBacklight, display->width - 80, ySize);
    lv_obj_set_pos(labelSliderBacklight, 0, y);
    lv_obj_add_style(labelSliderBacklight, &styleLabel, 0);
    y += yInc;

    struct BrightnessData
    {
        lv_obj_t* label;
        ValueCallbackUX<unsigned char> callback;
    };
    BrightnessData* brightnessData = new BrightnessData();
    brightnessData->label = labelSliderBacklight;
    brightnessData->callback = brightnessCallback;

    lv_obj_t* sliderBacklight = lv_slider_create(lvglScreenSettings);
    lv_obj_set_size(sliderBacklight, display->width - 80 - 24, ySize);
    lv_obj_set_pos(sliderBacklight, 24, y);
    lv_obj_add_event_cb(sliderBacklight,
        [](lv_event_t* e) {
            lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
            auto value = lv_slider_get_value(slider);
            BrightnessData* userData = (BrightnessData*)lv_event_get_user_data(e);
            userData->callback(value);
            lv_label_set_text_fmt(userData->label, "Backlight (%d)", value);
        },
        LV_EVENT_VALUE_CHANGED, brightnessData);

    lv_slider_set_range(sliderBacklight, 0, 255);
    lv_slider_set_value(sliderBacklight, brightness(), LV_ANIM_OFF);
    lv_obj_send_event(sliderBacklight, LV_EVENT_VALUE_CHANGED, nullptr);
    y += yInc;

    // wifi on/off + ip
    lv_obj_t* labelWifi = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelWifi, "WiFi");
    lv_obj_set_size(labelWifi, display->width - 80, ySize);
    lv_obj_set_pos(labelWifi, 0, y);
    lv_obj_add_style(labelWifi, &styleLabel, 0);
    y += yInc;
    labelWifiIP = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelWifiIP, wifiIP().c_str());
    lv_obj_set_size(labelWifiIP, display->width - 120, ySize);
    lv_obj_set_pos(labelWifiIP, 40, y);
    lv_obj_add_style(labelWifiIP, &styleLabel, 0);
    ledWifi = lv_led_create(lvglScreenSettings);
    lv_obj_set_pos(ledWifi, 20, y);
    lv_led_on(ledWifi);
    y += yInc;

    // websocket connect
    lv_obj_t* labelDropdownWinstonTarget = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelDropdownWinstonTarget, "Winston");
    lv_obj_set_size(labelDropdownWinstonTarget, display->width - 80, ySize);
    lv_obj_set_pos(labelDropdownWinstonTarget, 0, y);
    lv_obj_add_style(labelDropdownWinstonTarget, &styleLabel, 0);
    y += yInc;

    struct WinstonTargetData
    {
        ValueCallbackUX<WinstonTarget> callback;
    };
    WinstonTargetData* winstonTargetData = new WinstonTargetData();
    winstonTargetData->callback = winstonTarget;
    lv_obj_t* dropdownWinstonTarget = lv_dropdown_create(lvglScreenSettings);
    lv_dropdown_set_options(dropdownWinstonTarget, "Black Canary\nTeensy\n");
    lv_obj_set_size(dropdownWinstonTarget, display->width - 80, ySize);
    lv_obj_set_pos(dropdownWinstonTarget, 0, y);
    lv_obj_add_event_cb(dropdownWinstonTarget,
        [](lv_event_t* e) {
            lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
            auto value = lv_dropdown_get_selected(dropdown);
            WinstonTargetData* userData = (WinstonTargetData*)lv_event_get_user_data(e);
            userData->callback((WinstonTarget)value);
        },
        LV_EVENT_VALUE_CHANGED, winstonTargetData);

    struct ButtonData
    {
        Screen screen;
        ValueCallbackUX<Screen> callback;
    };
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->screen = Screen::Cinema;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnCinema = lv_button_create(lvglScreenSettings);
        lv_obj_set_size(btnCinema, 64, 140);
        lv_obj_set_pos(btnCinema, display->width - 80, 10);
        lv_obj_add_event_cb(btnCinema, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->screen);
            }, LV_EVENT_CLICKED, buttonData);
    } 
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->screen = Screen::Railway;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnRailway = lv_button_create(lvglScreenSettings);
        lv_obj_set_size(btnRailway, 64, 140);
        lv_obj_set_pos(btnRailway, display->width - 80, 170);
        lv_obj_add_event_cb(btnRailway, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->screen);
            }, LV_EVENT_CLICKED, buttonData);
    }
    // slider backlight
    // label: ip
    // led: wlan connected
    // dropdown: teensy or blackcanary
    // button cinema
    // button graph

  // graph
    // button settings
    // button cinema

    lvglScreenRailway = lv_obj_create(NULL);
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->screen = Screen::Settings;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnSettings = lv_button_create(lvglScreenRailway);
        lv_obj_set_size(btnSettings, 64, 140);
        lv_obj_set_pos(btnSettings, display->width - 80, 10);
        lv_obj_add_event_cb(btnSettings, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->screen);
            }, LV_EVENT_CLICKED, buttonData);
    }
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->screen = Screen::Railway;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnRailway = lv_button_create(lvglScreenRailway);
        lv_obj_set_size(btnRailway, 64, 140);
        lv_obj_set_pos(btnRailway, display->width - 80, 170);
        lv_obj_add_event_cb(btnRailway, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->screen);
            }, LV_EVENT_CLICKED, buttonData);
    }

    lv_screen_load(lvglScreenSettings);
}

const winston::Result showUX(const Screen screen)
{
    switch (screen)
    {
    case Screen::Cinema:
        break;
    case Screen::Railway:
        lv_screen_load(lvglScreenRailway);
        break;
    case Screen::Settings:
        lv_screen_load(lvglScreenSettings);
    default:
        return winston::Result::NotFound;
    }

    return winston::Result::OK;
}