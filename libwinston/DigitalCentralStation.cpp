#include "DigitalCentralStation.h"

namespace winston
{
	DigitalCentralStation::DigitalCentralStation(AddressTranslator::Shared& addressTranslator, SignalBox::Shared& signalBox, const Callbacks callbacks)
		: Shared_Ptr<DigitalCentralStation>(), addressTranslator(addressTranslator), signalBox(signalBox), callbacks(callbacks)
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

	void DigitalCentralStation::DebugInjector::injectLocoUpdate(Locomotive::Shared loco, bool busy, bool forward, unsigned char speed, uint32_t functions)
	{
		this->station->callbacks.locomotiveUpdateCallback(loco, busy, forward, speed, functions);
	}

	DigitalCentralStation::AddressTranslator::AddressTranslator() : Shared_Ptr<AddressTranslator>()
	{
	}

	void DigitalCentralStation::turnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction)
	{
		this->signalBox->order(Command::make([turnout, direction](const unsigned long long& created) -> const State { return turnout->finalizeChangeTo(direction);  }));
	}
};