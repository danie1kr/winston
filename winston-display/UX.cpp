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
LV_IMG_DECLARE(arrow_back_24);
LV_IMG_DECLARE(close_48);
LV_IMG_DECLARE(movie_48);
LV_IMG_DECLARE(quiz_48);
LV_IMG_DECLARE(settings_48);
LV_IMG_DECLARE(refresh_48);
LV_IMG_DECLARE(train_48);
#ifdef __cplusplus
}
#endif

lv_obj_t* lvglScreenMenu = nullptr;
lv_obj_t* lvglScreenSettings = nullptr;
lv_obj_t* lvglScreenRailway = nullptr;
lv_obj_t* lvglScreenStoryline = nullptr;

lv_obj_t* labelWifiIP = nullptr;
lv_obj_t* ledWifi = nullptr;
lv_obj_t* labelStorylineText = nullptr;

lv_obj_t* lvglScreenRailwayButtonReconnect = nullptr;
lv_obj_t* lvglScreenRailwayButtonBack = nullptr;

ValueCallbackUX<Screen> lvglGotoScreen = nullptr;

std::vector<lv_obj_t*> tracks;
std::map<lv_obj_t*, const winston::RailwayMicroLayout::TurnoutConnection> turnouts;
std::map<lv_obj_t*, const std::string> turnoutTogglers;

lv_style_t lvglStyleTrack, lvglStyleTurnout, lvglStyleTurnoutLabel, lvglStyleCenteredText, lvglStyleHeadline;
lv_theme_t* lvglTheme;

//lv_style_t lvglStyleDebug;

constexpr int uiButtonPadding = 8;
constexpr int uiButtonSize = 32;

#define APPLY_TO_CALLBACK(T) [](lv_event_t* e) { auto userData = (T*)lv_event_get_user_data(e); userData->callback(userData->value); }

void uxScreenRailwayShowButtonReconnect()
{
    lv_obj_remove_flag(lvglScreenRailwayButtonReconnect, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(lvglScreenRailwayButtonBack, LV_OBJ_FLAG_HIDDEN);
}

struct TurnoutToggleData
{
    ValueCallbackUX<std::string> callback;
    std::string turnoutId;
};
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
        auto data = (TurnoutToggleData *)lv_obj_get_user_data(turnoutToggler.first);
        if (data)
            delete data;
        lv_obj_delete(turnoutToggler.first);
    }
    turnoutTogglers.clear();
}

void uxUpdateRailwayLayout(winston::RailwayMicroLayout& rml, ValueCallbackUX<std::string> turnoutToggle)
{
    lv_obj_add_flag(lvglScreenRailwayButtonReconnect, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lvglScreenRailwayButtonBack, LV_OBJ_FLAG_HIDDEN);

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
        lv_obj_set_pos(button, bounds.min.x + (bounds.max.x - bounds.min.x)/2-16, bounds.min.y + (bounds.max.y - bounds.min.y) / 2-16);
        lv_obj_set_size(button, 32, 32);
        lv_obj_set_user_data(button, turnoutToggleData);
        lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(button, labelText.c_str());
        lv_obj_add_event_cb(button,
            [](lv_event_t* e) {
                TurnoutToggleData* userData = (TurnoutToggleData*)lv_event_get_user_data(e);
                userData->callback(userData->turnoutId);
            },
            LV_EVENT_SHORT_CLICKED, turnoutToggleData);
        lv_obj_add_event_cb(button,
            [](lv_event_t* e) {
                if(lvglGotoScreen)
                    lvglGotoScreen(Screen::Menu);
            },
            LV_EVENT_LONG_PRESSED, turnoutToggleData);
        lv_obj_add_style(button, &lvglStyleCenteredText, 0);
        lv_obj_add_style(button, &lvglStyleTurnoutLabel, 0);
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
            if(state == 0 && tc.connection.compare("b") == 0)
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 1 && tc.connection.compare("c") == 0)
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 2 && ((tc.connection.compare("a") == 0) || (tc.connection.compare("b") == 0)))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 3 && ((tc.connection.compare("a") == 0) || (tc.connection.compare("c") == 0)))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 4 && ((tc.connection.compare("c") == 0) || (tc.connection.compare("d") == 0)))
                lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
            else if (state == 5 && ((tc.connection.compare("b") == 0) || (tc.connection.compare("d") == 0)))
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

    if (lvglScreenRailwayButtonReconnect)
    {
        if (on)
            lv_obj_clear_state(lvglScreenRailwayButtonReconnect, LV_STATE_DISABLED);
        else
            lv_obj_add_state(lvglScreenRailwayButtonReconnect, LV_STATE_DISABLED);
    }
}

void uxUpdateStorylineText(const std::string text)
{
    lv_label_set_text(labelStorylineText, text.c_str());
    lv_label_set_long_mode(labelStorylineText, LV_LABEL_LONG_WRAP);
}

void setupUX(winston::hal::DisplayUX::Shared display,
    ValueCallbackUX<unsigned char> brightnessCallback,
    ValueGetterUX<unsigned char> brightness,
    ValueCallbackUX<Screen> gotoScreen,
    ValueCallbackUX<WinstonTarget> winstonTarget,
    ValueGetterUX<std::string> wifiIP,
    ValueCallbackUX<std::string> storylineReply,
    CallbackUX reconnect)
{
    using ButtonDataScreen = ValueCallbackUXTriggerData<Screen>;

    lvglGotoScreen = gotoScreen;

    const unsigned int yStart = 8;
    unsigned int y = yStart;
    const unsigned int x = 8;
    const unsigned int ySize = 24;
    const unsigned int yInc = ySize + 8;

    lv_style_init(&lvglStyleCenteredText);
    lv_style_set_text_align(&lvglStyleCenteredText, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&lvglStyleHeadline); 
    lv_style_set_text_font(&lvglStyleHeadline, &lv_font_montserrat_20);
    lv_style_set_text_align(&lvglStyleHeadline, LV_TEXT_ALIGN_CENTER);
    /*
    lv_style_init(&lvglStyleDebug);
    lv_style_set_border_width(&lvglStyleDebug, 3);
    lv_style_set_border_color(&lvglStyleDebug, lv_palette_main(LV_PALETTE_DEEP_ORANGE));
    */
    lv_style_init(&lvglStyleTurnoutLabel);
    lv_style_set_text_font(&lvglStyleTurnoutLabel, &lv_font_montserrat_10);
    lv_point_t p;
    lv_text_get_size(&p, "1234567890", &lv_font_montserrat_10, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_EXPAND);
    lv_style_set_border_color(&lvglStyleTurnoutLabel, lv_palette_main(LV_PALETTE_DEEP_ORANGE));
    lv_style_set_pad_top(&lvglStyleTurnoutLabel, 32/2 - ((p.y+1) / 2));

    lv_theme_t* th = lv_theme_default_init(lv_display_get_default(),
        lv_palette_main(LV_PALETTE_TEAL), lv_palette_main(LV_PALETTE_ORANGE),
        false,
        &lv_font_montserrat_14);

    lv_display_set_theme(lv_display_get_default(), th); /*Assign the theme to the display*/

    static lv_style_t styleIconRecolor;
    lv_style_init(&styleIconRecolor);
    lv_style_set_image_recolor(&styleIconRecolor, lv_color_white());
    lv_style_set_img_recolor_opa(&styleIconRecolor, LV_OPA_COVER);

    // menu
    {
        auto screen = lvglScreenMenu = lv_obj_create(NULL);
        lv_obj_t* labelTitle = lv_label_create(screen);
        lv_label_set_text(labelTitle, "Winston Display");
        lv_obj_set_width(labelTitle, display->width);
        lv_obj_set_pos(labelTitle, 0, y);
        lv_obj_add_style(labelTitle, &lvglStyleHeadline, 0);
        y += yInc;

        // railway
        {
            auto buttonData = new ButtonDataScreen();
            buttonData->value = Screen::Railway;
            buttonData->callback = gotoScreen;
            lv_obj_t* button = lv_button_create(screen);
            lv_obj_set_content_width(button, 4*uiButtonSize);
            lv_obj_set_content_height(button, 2*uiButtonSize);
            lv_obj_align(button, LV_ALIGN_LEFT_MID, 60, -32);
            lv_obj_add_event_cb(button, APPLY_TO_CALLBACK(ButtonDataScreen) , LV_EVENT_CLICKED, buttonData);

            lv_obj_t* buttonIcon = lv_image_create(button);
            lv_image_set_src(buttonIcon, &train_48);
            lv_obj_add_style(buttonIcon, &styleIconRecolor, 0);
            lv_obj_align(buttonIcon, LV_ALIGN_CENTER, 0, 0);
        }
        // story
        {
            auto buttonData = new ButtonDataScreen();
            buttonData->value = Screen::Storyline;
            buttonData->callback = gotoScreen;
            lv_obj_t* button = lv_button_create(screen);
            lv_obj_set_content_width(button, 4 * uiButtonSize);
            lv_obj_set_content_height(button, 2 * uiButtonSize);
            lv_obj_align(button, LV_ALIGN_RIGHT_MID, -60, -32);
            lv_obj_add_event_cb(button, APPLY_TO_CALLBACK(ButtonDataScreen), LV_EVENT_CLICKED, buttonData);

            lv_obj_t* buttonIcon = lv_image_create(button);
            lv_image_set_src(buttonIcon, &quiz_48);
            lv_obj_add_style(buttonIcon, &styleIconRecolor, 0);
            lv_obj_align(buttonIcon, LV_ALIGN_CENTER, 0, 0);
        }
        // cinema
        {
            auto buttonData = new ButtonDataScreen();
            buttonData->value = Screen::Cinema;
            buttonData->callback = gotoScreen;
            lv_obj_t* button = lv_button_create(screen);
            lv_obj_set_content_width(button, 4 * uiButtonSize);
            lv_obj_set_content_height(button, 2 * uiButtonSize);
            lv_obj_align(button, LV_ALIGN_LEFT_MID, 60, 80);
            lv_obj_add_event_cb(button, APPLY_TO_CALLBACK(ButtonDataScreen), LV_EVENT_CLICKED, buttonData);

            lv_obj_t* buttonIcon = lv_image_create(button);
            lv_image_set_src(buttonIcon, &movie_48);
            lv_obj_add_style(buttonIcon, &styleIconRecolor, 0);
            lv_obj_align(buttonIcon, LV_ALIGN_CENTER, 0, 0);
        }
        // settings
        {
            auto buttonData = new ButtonDataScreen();
            buttonData->value = Screen::Settings;
            buttonData->callback = gotoScreen;
            lv_obj_t* button = lv_button_create(screen);
            lv_obj_set_content_width(button, 4 * uiButtonSize);
            lv_obj_set_content_height(button, 2 * uiButtonSize);
            lv_obj_align(button, LV_ALIGN_RIGHT_MID, -60, 80);
            lv_obj_add_event_cb(button, APPLY_TO_CALLBACK(ButtonDataScreen), LV_EVENT_CLICKED, buttonData);

            lv_obj_t* buttonIcon = lv_image_create(button);
            lv_image_set_src(buttonIcon, &settings_48);
            lv_obj_add_style(buttonIcon, &styleIconRecolor, 0);
            lv_obj_align(buttonIcon, LV_ALIGN_CENTER, 0, 0);
        }
    }

    // settings
    {
        y = yStart;
        auto screen = lvglScreenSettings = lv_obj_create(NULL);
        lv_obj_t* labelTitle = lv_label_create(screen);
        lv_label_set_text(labelTitle, "Settings");
        lv_obj_set_width(labelTitle, display->width - 80);
        lv_obj_set_pos(labelTitle, 0, y);
        lv_obj_add_style(labelTitle, &lvglStyleHeadline, 0);
        y += yInc;

        lv_obj_t* labelSliderBacklight = lv_label_create(screen);
        lv_label_set_text(labelSliderBacklight, "Backlight");
        lv_obj_set_width(labelSliderBacklight, display->width - 80);
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

        lv_obj_t* sliderBacklight = lv_slider_create(screen);
        lv_obj_set_width(sliderBacklight, display->width - 92 - 24);
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
        lv_obj_t* labelWifi = lv_label_create(screen);
        lv_label_set_text(labelWifi, "WiFi");
        lv_obj_set_width(labelWifi, display->width - 92);
        lv_obj_set_pos(labelWifi, 0, y);
        lv_obj_add_style(labelWifi, &lvglStyleCenteredText, 0);
        y += yInc;
        labelWifiIP = lv_label_create(screen);
        lv_label_set_text(labelWifiIP, wifiIP().c_str());
        lv_obj_set_width(labelWifiIP, display->width - 120);
        lv_obj_set_pos(labelWifiIP, 40, y);
        lv_obj_add_style(labelWifiIP, &lvglStyleCenteredText, 0);
        ledWifi = lv_led_create(screen);
        lv_obj_set_pos(ledWifi, 20, y);
        lv_led_set_color(ledWifi, lv_palette_main(LV_PALETTE_TEAL));
        lv_led_off(ledWifi);
        y += yInc;

        // websocket target
        lv_obj_t* labelDropdownWinstonTarget = lv_label_create(screen);
        lv_label_set_text(labelDropdownWinstonTarget, "Winston");
        lv_obj_set_width(labelDropdownWinstonTarget, display->width - 92);
        lv_obj_set_pos(labelDropdownWinstonTarget, 0, y);
        lv_obj_add_style(labelDropdownWinstonTarget, &lvglStyleCenteredText, 0);
        y += yInc;

        struct WinstonTargetData
        {
            ValueCallbackUX<WinstonTarget> callback;
        };
        WinstonTargetData* winstonTargetData = new WinstonTargetData();
        winstonTargetData->callback = winstonTarget;
        lv_obj_t* dropdownWinstonTarget = lv_dropdown_create(screen);
        lv_dropdown_set_options(dropdownWinstonTarget, "Black Canary LAN\nBlack Canary Wifi\nLocalhost\nTeensy\n");
        lv_obj_set_width(dropdownWinstonTarget, display->width - 92 - 24);
        lv_obj_set_pos(dropdownWinstonTarget, x, y);
        lv_obj_add_event_cb(dropdownWinstonTarget,
            [](lv_event_t* e) {
                lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
                auto value = lv_dropdown_get_selected(dropdown);
                WinstonTargetData* userData = (WinstonTargetData*)lv_event_get_user_data(e);
                userData->callback((WinstonTarget)value);
            },
            LV_EVENT_VALUE_CHANGED, winstonTargetData);

        // back to menu
        {
            auto buttonData = new ButtonDataScreen();
            buttonData->value = Screen::Menu;
            buttonData->callback = gotoScreen;
            lv_obj_t* button = lv_button_create(screen);
            lv_obj_set_content_width(button, uiButtonSize);
            lv_obj_set_content_height(button, uiButtonSize);
            lv_obj_align(button, LV_ALIGN_BOTTOM_RIGHT, -uiButtonPadding, -uiButtonPadding);
            lv_obj_add_event_cb(button, APPLY_TO_CALLBACK(ButtonDataScreen), LV_EVENT_CLICKED, buttonData);
            lv_obj_t* buttonIcon = lv_image_create(button);
            lv_image_set_src(buttonIcon, &arrow_back_24);
            lv_obj_add_style(buttonIcon, &styleIconRecolor, 0);
            lv_obj_align(buttonIcon, LV_ALIGN_CENTER, 0, 0);
        }
    }

    // railway
    {
        auto screen = lvglScreenRailway = lv_obj_create(NULL);
        {
            auto buttonData = new ButtonDataScreen();
            buttonData->value = Screen::Menu;
            buttonData->callback = gotoScreen;
            auto button = lvglScreenRailwayButtonBack = lv_button_create(screen);
            lv_obj_set_content_width(button, uiButtonSize);
            lv_obj_set_content_height(button, uiButtonSize);
            lv_obj_align(button, LV_ALIGN_BOTTOM_RIGHT, -uiButtonPadding, -uiButtonPadding);
            lv_obj_add_event_cb(button, APPLY_TO_CALLBACK(ButtonDataScreen), LV_EVENT_CLICKED, buttonData);
            lv_obj_t* buttonIcon = lv_image_create(button);
            lv_image_set_src(buttonIcon, &arrow_back_24);
            lv_obj_add_style(buttonIcon, &styleIconRecolor, 0);
            lv_obj_align(buttonIcon, LV_ALIGN_CENTER, 0, 0);
        }
        {
            CallbackUXTriggerData* buttonData = new CallbackUXTriggerData();
            buttonData->callback = reconnect;
            lvglScreenRailwayButtonReconnect = lv_button_create(screen);
            lv_obj_align(lvglScreenRailwayButtonReconnect, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_event_cb(lvglScreenRailwayButtonReconnect, [](lv_event_t* e) {
                CallbackUXTriggerData* userData = (CallbackUXTriggerData*)lv_event_get_user_data(e);
                userData->callback();
                }, LV_EVENT_CLICKED, buttonData);

            lv_obj_t* btnReconnectLabel = lv_label_create(lvglScreenRailwayButtonReconnect);
            lv_obj_add_state(lvglScreenRailwayButtonReconnect, LV_STATE_DISABLED);
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
    }

    // storyline
    {
        using StorylineReplyButtonData = ValueCallbackUXTriggerData<std::string>;
        auto screen = lvglScreenStoryline = lv_obj_create(NULL);
        {
            auto buttonData = new StorylineReplyButtonData();
            buttonData->value = "refresh";
            buttonData->callback = storylineReply;
            lv_obj_t* btnRefresh = lv_button_create(screen);
            lv_obj_set_content_width(btnRefresh, 4*uiButtonSize);
            lv_obj_set_content_height(btnRefresh, 2*uiButtonSize);
            lv_obj_align(btnRefresh, LV_ALIGN_LEFT_MID, 60, 48);
            lv_obj_add_event_cb(btnRefresh, APPLY_TO_CALLBACK(StorylineReplyButtonData), LV_EVENT_CLICKED, buttonData);

            lv_obj_t* btnRefreshIcon = lv_image_create(btnRefresh);
            lv_image_set_src(btnRefreshIcon, &refresh_48);
            lv_obj_add_style(btnRefreshIcon, &styleIconRecolor, 0);
            lv_obj_align(btnRefreshIcon, LV_ALIGN_CENTER, 0, 0);
        }
        {
            auto buttonData = new StorylineReplyButtonData();
            buttonData->value = "cancel";
            buttonData->callback = storylineReply;
            lv_obj_t* btnCancel = lv_button_create(screen);
            lv_obj_set_content_width(btnCancel, 4*uiButtonSize);
            lv_obj_set_content_height(btnCancel, 2*uiButtonSize);
            lv_obj_align(btnCancel, LV_ALIGN_RIGHT_MID, -60, 48);
            lv_obj_add_event_cb(btnCancel, APPLY_TO_CALLBACK(StorylineReplyButtonData), LV_EVENT_CLICKED, buttonData);

            lv_obj_t* btnCancelIcon = lv_image_create(btnCancel);
            lv_image_set_src(btnCancelIcon, &close_48);
            lv_obj_add_style(btnCancelIcon, &styleIconRecolor, 0);
            lv_obj_align(btnCancelIcon, LV_ALIGN_CENTER, 0, 0);
        }
        {
            labelStorylineText = lv_label_create(screen);
            lv_obj_align(labelStorylineText, LV_ALIGN_CENTER, 0, -72);
            lv_obj_set_width(labelStorylineText, display->width - 16);
            lv_label_set_text_static(labelStorylineText, "???");
            lv_obj_add_style(labelStorylineText, &lvglStyleCenteredText, 0);
            lv_label_set_long_mode(labelStorylineText, LV_LABEL_LONG_WRAP);
        }
        {
            auto buttonData = new ButtonDataScreen();
            buttonData->value = Screen::Menu;
            buttonData->callback = gotoScreen;
            lv_obj_t* btnRailway = lv_button_create(screen);
            lv_obj_set_content_width(btnRailway, uiButtonSize);
            lv_obj_set_content_height(btnRailway, uiButtonSize);
            lv_obj_align(btnRailway, LV_ALIGN_BOTTOM_RIGHT, -uiButtonPadding, -uiButtonPadding);
            lv_obj_add_event_cb(btnRailway, APPLY_TO_CALLBACK(ButtonDataScreen), LV_EVENT_CLICKED, buttonData);
            lv_obj_t* btnRailwayIcon = lv_image_create(btnRailway);
            lv_image_set_src(btnRailwayIcon, &arrow_back_24);
            lv_obj_add_style(btnRailwayIcon, &styleIconRecolor, 0);
            lv_obj_align(btnRailwayIcon, LV_ALIGN_CENTER, 0, 0);
        }
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
        lv_obj_invalidate(lv_screen_active());
        break;
    case Screen::Settings:
        lv_screen_load(lvglScreenSettings);
        lv_obj_invalidate(lv_screen_active());
        break;
    case Screen::Menu:
        lv_screen_load(lvglScreenMenu);
        lv_obj_invalidate(lv_screen_active());
        break;
    case Screen::Storyline:
        lv_screen_load(lvglScreenStoryline);
        lv_obj_invalidate(lv_screen_active());
        break;
    default:
        return winston::Result::NotFound;
    }

    return winston::Result::OK;
}