#pragma once
#include "../libwinston/WinstonConfig.h"

#include <functional>

#include "../libwinston/HAL.h"
#include "../libwinston/EventLooper.h"
#include "../libwinston/RailwayMicroLayout.h"
#ifdef __cplusplus
extern "C" {
#endif
	void winston_setup();
	void winston_loop();

#ifdef __cplusplus
}
#endif

enum class WinstonTarget
{
	BlackCanaryLAN = 0,
	BlackCanaryWifi = 1,
	Localhost = 2,
	Teensy = 3
};

enum class Screen
{
	Menu = 0,
	Cinema = 1,
	Settings = 2,
	Railway = 3,
	Storyline = 4,
};

struct Settings
{
	unsigned char brightness;
	WinstonTarget target;
	Screen screen;
};

template<typename T>
using ValueCallbackUX = std::function<const winston::Result(const T value)>;
using CallbackUX = std::function<const winston::Result()>;

template<typename T>
using ValueGetterUX = std::function<const T()>;

template<typename T>
struct ValueCallbackUXTriggerData
{
	T value;
	ValueCallbackUX<T> callback;
};
struct CallbackUXTriggerData
{
	CallbackUX callback;
};

template<typename T>
struct ValueGetterUXTriggerData
{
	ValueGetterUX<T> callback;
};

void setupUX(winston::hal::DisplayUX::Shared display,
	ValueCallbackUX<unsigned char> brightnessCallback,
	ValueGetterUX<unsigned char> brightness,
	ValueCallbackUX<Screen> gotoScreen,
	ValueCallbackUX<WinstonTarget> winstonTarget,
	ValueGetterUX<std::string> wifiIP,
	ValueCallbackUX<std::string> storylineReply,
	CallbackUX reconnect);

void uxUpdateRailwayLayout(winston::RailwayMicroLayout& rml, ValueCallbackUX<std::string> turnoutToggle);
void uxUpdateTurnout(const std::string& id, const int& state, const bool& locked);
void uxUpdateWifiIP(const std::string ip);
void uxUpdateWifiLED(const bool on);
void uxScreenRailwayShowButtonReconnect();
void uxScreenRailwayClear();
void uxUpdateStorylineText(const std::string text);

const winston::Result showUX(const Screen screen);