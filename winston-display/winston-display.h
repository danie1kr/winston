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
	Cinema = 0,
	Settings = 1,
	Railway = 2,
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

void setupUX(winston::hal::DisplayUX::Shared display,
	ValueCallbackUX<unsigned char> brightnessCallback,
	ValueGetterUX<unsigned char> brightness,
	ValueCallbackUX<Screen> gotoScreen,
	ValueCallbackUX<WinstonTarget> winstonTarget,
	ValueGetterUX<std::string> wifiIP);

void uxUpdateRailwayLayout(winston::RailwayMicroLayout& rml, ValueCallbackUX<std::string> turnoutToggle);
void uxUpdateTurnout(const std::string& id, const int& state, const bool& locked);
void uxUpdateWifiIP(const std::string ip);
void uxUpdateWifiLED(const bool on);

const winston::Result showUX(const Screen screen);