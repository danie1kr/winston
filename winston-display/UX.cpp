#include <string>
#include <map>

#include "winston-display.h"
#ifdef WINSTON_PLATFORM_WIN_x64
#include "external/lvgl/lvgl.h"
#else
#include <lvgl.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
LV_IMG_DECLARE(icon_arrow_back);
LV_IMG_DECLARE(icon_movie);
LV_IMG_DECLARE(icon_settings);
LV_IMG_DECLARE(icon_train);
#ifdef __cplusplus
}
#endif

lv_obj_t* lvglScreenSettings = nullptr;
lv_obj_t* lvglScreenRailway = nullptr;

lv_obj_t* labelWifiIP = nullptr;
lv_obj_t* ledWifi = nullptr;

lv_obj_t* lvglScreenRailwayButtonRecconect = nullptr;

std::vector<lv_obj_t*> tracks;
std::map<lv_obj_t*, const winston::RailwayMicroLayout::TurnoutConnection> turnouts;
std::map<lv_obj_t*, const std::string> turnoutTogglers;

lv_style_t lvglStyleTrack, lvglStyleTurnout, lvglStyleCenteredText;

constexpr int uiButtonPadding = 8;
constexpr int uiButtonSize = 32;

void uxScreenRailwayShowButtonReconnect()
{
    lv_obj_remove_flag(lvglScreenRailwayButtonRecconect, LV_OBJ_FLAG_HIDDEN);
}

void uxScreenRailwayClear()
{
    for (auto& track : tracks)
        lv_obj_delete(track);
    tracks.clear();

    for (auto& turnout : turnouts)
        lv_obj_delete(turnout.first);
    turnouts.clear();

    for (auto& turnoutToggler : turnoutTogglers)
    {
        auto data = lv_obj_get_user_data(turnoutToggler.first);
        if (data)
            delete data;
        lv_obj_delete(turnoutToggler.first);
    }
    turnoutTogglers.clear();
}

void uxUpdateRailwayLayout(winston::RailwayMicroLayout& rml, ValueCallbackUX<std::string> turnoutToggle)
{
    lv_obj_add_flag(lvglScreenRailwayButtonRecconect, LV_OBJ_FLAG_HIDDEN);
    struct TurnoutToggleData
    {
        ValueCallbackUX<std::string> callback;
        std::string turnoutId;
    };

    uxScreenRailwayClear();

    for (const auto& track : rml.tracks)
    {
        auto line = lv_line_create(lvglScreenRailway);
        lv_line_set_points(line, (lv_point_precise_t*)&track.front(), (unsigned int)track.size());
        lv_obj_add_style(line, &lvglStyleTrack, 0);
        tracks.push_back(line);
    }

    for (const auto& turnout : rml.turnouts)
    {
        winston::RailwayMicroLayout::Bounds bounds;

        for (const auto& connection : turnout.connections)
        {
            auto line = lv_line_create(lvglScreenRailway);
            lv_line_set_points(line, (lv_point_precise_t*)&connection.p.front(), connection.p.size());
            lv_obj_add_style(line, &lvglStyleTurnout, 0);
            lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);

            const winston::RailwayMicroLayout::TurnoutConnection turnoutConnection = { turnout.id, connection.connection };
            turnouts.insert(std::make_pair(line, turnoutConnection));

            bounds.update(connection.p[0]);
            bounds.update(connection.p[1]);
        }
        
        TurnoutToggleData* turnoutToggleData = new TurnoutToggleData();
        turnoutToggleData->callback = turnoutToggle;
        turnoutToggleData->turnoutId = turnout.id;

        std::string labelText = turnout.id;
        auto posDST = labelText.find("DoubleSlipTurnout");
        auto posT = labelText.find("Turnout");

        if (posDST != std::string::npos)
            labelText.erase(posDST, strlen("DoubleSlipTurnout"));
        else if (posT != std::string::npos)
            labelText.erase(posT, strlen("Turnout"));

        auto button = lv_label_create(lvglScreenRailway);
        lv_obj_set_pos(button, bounds.min.x + (bounds.max.x - bounds.min.x)/2 - 16, bounds.min.y + (bounds.max.y - bounds.min.y) / 2 - 16);
        lv_obj_set_size(button, 32, 32);
        lv_obj_set_user_data(button, turnoutToggleData);
        lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(button, labelText.c_str());
        lv_obj_add_event_cb(button,
            [](lv_event_t* e) {
                TurnoutToggleData* userData = (TurnoutToggleData*)lv_event_get_user_data(e);
                userData->callback(userData->turnoutId);
            },
            LV_EVENT_CLICKED, turnoutToggleData);
        lv_obj_add_style(button, &lvglStyleCenteredText, 0);
        turnoutTogglers.insert(std::make_pair(button, turnout.id));
    }
}

void uxUpdateTurnout(const std::string &id, const int &state, const bool &locked)
{
    for (const auto& turnout : turnouts)
    {
        auto &tc = turnout.second;
        auto line = turnout.first;
        if (tc.turnout.compare(id) == 0)
        {
            /*
            two way:
            A_B = 0, 
            A_C = 1,
            four way:
            A_B = 2,
			A_C = 3,
			C_D = 4,
			B_D = 5,
            changing = 8
            */
            if(state == 0 && tc.connection.compare("b"))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 1 && tc.connection.compare("c"))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 2 && (tc.connection.compare("a") || tc.connection.compare("b")))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 3 && (tc.connection.compare("a") || tc.connection.compare("c")))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 4 && (tc.connection.compare("c") || tc.connection.compare("d")))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 5 && (tc.connection.compare("b") || tc.connection.compare("d")))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void uxUpdateWifiIP(const std::string ip)
{
    lv_label_set_text(labelWifiIP, ip.c_str());
}

void uxUpdateWifiLED(const bool on)
{
    if (ledWifi)
    {
        if (on)
            lv_led_on(ledWifi);
        else
            lv_led_off(ledWifi);
    }
}

void setupUX(winston::hal::DisplayUX::Shared display, 
    ValueCallbackUX<unsigned char> brightnessCallback, 
    ValueGetterUX<unsigned char> brightness, 
    ValueCallbackUX<Screen> gotoScreen,
    ValueCallbackUX<WinstonTarget> winstonTarget,
    ValueGetterUX<std::string> wifiIP,
    CallbackUX reconnect)
{
    unsigned int y = 0;
    const unsigned int x = 8;
    const unsigned int ySize = 24;
    const unsigned int yInc = ySize + 8;

    lv_style_init(&lvglStyleCenteredText);
    lv_style_set_text_align(&lvglStyleCenteredText, LV_TEXT_ALIGN_CENTER);

    static lv_style_t styleIconRecolor;
    lv_style_init(&styleIconRecolor);
    lv_style_set_image_recolor(&styleIconRecolor, lv_color_white());
    lv_style_set_img_recolor_opa(&styleIconRecolor, LV_OPA_COVER);

    // settings
    lvglScreenSettings = lv_obj_create(NULL);
    lv_obj_t* labelTitle = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelTitle, "Settings");
    lv_obj_set_size(labelTitle, display->width - 80, ySize);
    lv_obj_set_pos(labelTitle, 0, y);
    lv_obj_add_style(labelTitle, &lvglStyleCenteredText, 0);
    y += yInc;

    lv_obj_t* labelSliderBacklight = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelSliderBacklight, "Backlight");
    lv_obj_set_size(labelSliderBacklight, display->width - 80, ySize);
    lv_obj_set_pos(labelSliderBacklight, 0, y);
    lv_obj_add_style(labelSliderBacklight, &lvglStyleCenteredText, 0);
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
    lv_obj_set_size(sliderBacklight, display->width - 92 - 24, ySize);
    lv_obj_set_pos(sliderBacklight, x, y);
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
    lv_obj_set_size(labelWifi, display->width - 92, ySize);
    lv_obj_set_pos(labelWifi, 0, y);
    lv_obj_add_style(labelWifi, &lvglStyleCenteredText, 0);
    y += yInc;
    labelWifiIP = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelWifiIP, wifiIP().c_str());
    lv_obj_set_size(labelWifiIP, display->width - 120, ySize);
    lv_obj_set_pos(labelWifiIP, 40, y);
    lv_obj_add_style(labelWifiIP, &lvglStyleCenteredText, 0);
    ledWifi = lv_led_create(lvglScreenSettings);
    lv_obj_set_pos(ledWifi, 20, y);
    lv_led_on(ledWifi);
    y += yInc;

    // websocket target
    lv_obj_t* labelDropdownWinstonTarget = lv_label_create(lvglScreenSettings);
    lv_label_set_text(labelDropdownWinstonTarget, "Winston");
    lv_obj_set_size(labelDropdownWinstonTarget, display->width - 92, ySize);
    lv_obj_set_pos(labelDropdownWinstonTarget, 0, y);
    lv_obj_add_style(labelDropdownWinstonTarget, &lvglStyleCenteredText, 0);
    y += yInc;

    struct WinstonTargetData
    {
        ValueCallbackUX<WinstonTarget> callback;
    };
    WinstonTargetData* winstonTargetData = new WinstonTargetData();
    winstonTargetData->callback = winstonTarget;
    lv_obj_t* dropdownWinstonTarget = lv_dropdown_create(lvglScreenSettings);
    lv_dropdown_set_options(dropdownWinstonTarget, "Black Canary LAN\nBlack Canary Wifi\nLocalhost\nTeensy\n");
    lv_obj_set_size(dropdownWinstonTarget, display->width - 92 - 24, ySize);
    lv_obj_set_pos(dropdownWinstonTarget, x, y);
    lv_obj_add_event_cb(dropdownWinstonTarget,
        [](lv_event_t* e) {
            lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
            auto value = lv_dropdown_get_selected(dropdown);
            WinstonTargetData* userData = (WinstonTargetData*)lv_event_get_user_data(e);
            userData->callback((WinstonTarget)value);
        },
        LV_EVENT_VALUE_CHANGED, winstonTargetData);

    using ButtonData = ValueCallbackUXTriggerData<Screen>;
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->value = Screen::Cinema;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnCinema = lv_button_create(lvglScreenSettings);
        lv_obj_set_content_width(btnCinema, uiButtonSize);
        lv_obj_set_content_height(btnCinema, uiButtonSize);
        lv_obj_align(btnCinema, LV_ALIGN_BOTTOM_RIGHT, -uiButtonPadding, -uiButtonPadding);
        //lv_obj_padd
        lv_obj_add_event_cb(btnCinema, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->value);
            }, LV_EVENT_CLICKED, buttonData);

        lv_obj_t *btnCinemaIcon = lv_image_create(btnCinema);
        lv_image_set_src(btnCinemaIcon, &icon_movie);
        lv_obj_align(btnCinemaIcon, LV_ALIGN_CENTER, 0, 0);
    } 
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->value = Screen::Railway;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnRailway = lv_button_create(lvglScreenSettings);
        lv_obj_set_content_width(btnRailway, uiButtonSize);
        lv_obj_set_content_height(btnRailway, uiButtonSize);
        lv_obj_align(btnRailway, LV_ALIGN_BOTTOM_RIGHT, -uiButtonPadding, -(4* uiButtonPadding + uiButtonSize));
        lv_obj_add_event_cb(btnRailway, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->value);
            }, LV_EVENT_CLICKED, buttonData);
        lv_obj_t* btnRailwayIcon = lv_image_create(btnRailway);
        lv_image_set_src(btnRailwayIcon, &icon_train);
        lv_obj_add_style(btnRailwayIcon, &styleIconRecolor, 0);
        lv_obj_align(btnRailwayIcon, LV_ALIGN_CENTER, 0, 0);
    }

    lvglScreenRailway = lv_obj_create(NULL);
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->value = Screen::Settings;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnSettings = lv_button_create(lvglScreenRailway);
        lv_obj_set_content_width(btnSettings, uiButtonSize);
        lv_obj_set_content_height(btnSettings, uiButtonSize);
        lv_obj_align(btnSettings, LV_ALIGN_BOTTOM_RIGHT, -uiButtonPadding, -uiButtonPadding);
        lv_obj_add_event_cb(btnSettings, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->value);
            }, LV_EVENT_CLICKED, buttonData);
        
        lv_obj_t* btnSettingsIcon = lv_image_create(btnSettings);
        lv_image_set_src(btnSettingsIcon, &icon_settings);
        lv_obj_align(btnSettingsIcon, LV_ALIGN_CENTER, 0, 0);
    }
    {
        ButtonData* buttonData = new ButtonData();
        buttonData->value = Screen::Railway;
        buttonData->callback = gotoScreen;
        lv_obj_t* btnRailway = lv_button_create(lvglScreenRailway);
        lv_obj_set_content_width(btnRailway, uiButtonSize);
        lv_obj_set_content_height(btnRailway, uiButtonSize);
        lv_obj_align(btnRailway, LV_ALIGN_BOTTOM_RIGHT, -uiButtonPadding, -(4 * uiButtonPadding + uiButtonSize));
        lv_obj_add_event_cb(btnRailway, [](lv_event_t* e) {
            ButtonData* userData = (ButtonData*)lv_event_get_user_data(e);
            userData->callback(userData->value);
            }, LV_EVENT_CLICKED, buttonData);
        
        lv_obj_t* btnRailwayIcon = lv_image_create(btnRailway);
        lv_image_set_src(btnRailwayIcon, &icon_train);
        lv_obj_align(btnRailwayIcon, LV_ALIGN_CENTER, 0, 0);
    }
    {
        CallbackUXTriggerData* buttonData = new CallbackUXTriggerData();
        buttonData->callback = reconnect;
        lvglScreenRailwayButtonRecconect = lv_button_create(lvglScreenRailway);
        lv_obj_align(lvglScreenRailwayButtonRecconect, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(lvglScreenRailwayButtonRecconect, [](lv_event_t* e) {
            CallbackUXTriggerData* userData = (CallbackUXTriggerData*)lv_event_get_user_data(e);
            userData->callback();
            }, LV_EVENT_CLICKED, buttonData);
        
        lv_obj_t* btnReconnectLabel = lv_label_create(lvglScreenRailwayButtonRecconect);
        lv_label_set_text_static(btnReconnectLabel, "reconnect");
    }

    lv_style_init(&lvglStyleTrack);
    lv_style_set_line_width(&lvglStyleTrack, 6);
    lv_style_set_line_color(&lvglStyleTrack, lv_palette_main(LV_PALETTE_TEAL));
    lv_style_set_line_rounded(&lvglStyleTrack, true);
    lv_style_init(&lvglStyleTurnout);
    lv_style_set_line_width(&lvglStyleTurnout, 8);
    lv_style_set_line_color(&lvglStyleTurnout, lv_palette_main(LV_PALETTE_ORANGE));
    lv_style_set_line_rounded(&lvglStyleTurnout, true);

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
        lv_obj_invalidate(lv_screen_active());
        break;
    case Screen::Settings:
        lv_screen_load(lvglScreenSettings);
        lv_obj_invalidate(lv_screen_active());
    default:
        return winston::Result::NotFound;
    }

    return winston::Result::OK;
}