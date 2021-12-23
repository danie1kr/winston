#include "DigitalCentralStation.h"

namespace winston
{
	DigitalCentralStation::DigitalCentralStation(TurnoutAddressTranslator::Shared& turnoutAddressTranslator, LocoAddressTranslator& locoAddressTranslator, SignalBox::Shared& signalBox, const Callbacks callbacks)
		: Shared_Ptr<DigitalCentralStation>(), turnoutAddressTranslator(turnoutAddressTranslator), locoAddressTranslator(locoAddressTranslator), signalBox(signalBox), callbacks(callbacks)
	{

	}

	DigitalCentralStation::DebugInjector::DebugInjector(DigitalCentralStation::Shared& station)
		: Shared_Ptr<DebugInjector>(), station(station)
	{

	}

	void DigitalCentralStation::DebugInjector::injectTurnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction)
	{
		this->station->turnoutUpdate(turnout, direction);
	}

	void DigitalCentralStation::DebugInjector::injectLocoUpdate(Locomotive& loco, bool busy, bool forward, unsigned char speed, uint32_t functions)
	{
		this->station->callbacks.locomotiveUpdateCallback(loco, busy, forward, speed, functions);
	}

	void DigitalCentralStation::turnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction)
	{
		this->signalBox->order(Command::make([turnout, direction](const unsigned long long& created) -> const State { return turnout->finalizeChangeTo(direction);  }));
	}
};