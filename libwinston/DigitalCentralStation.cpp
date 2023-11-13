#include "DigitalCentralStation.h"

namespace winston
{
	DigitalCentralStation::DigitalCentralStation(TurnoutAddressTranslator::Shared& turnoutAddressTranslator, LocoAddressTranslator& locoAddressTranslator, SignalTower::Shared& signalTower, const Callbacks callbacks)
		: Shared_Ptr<DigitalCentralStation>(), turnoutAddressTranslator(turnoutAddressTranslator), locoAddressTranslator(locoAddressTranslator), signalTower(signalTower), callbacks(callbacks), emergencyStop(false), connectedToDCS(false)
	{

	}

	DigitalCentralStation::DebugInjector::DebugInjector(DigitalCentralStation::Shared& station)
		: Shared_Ptr<DebugInjector>(), station(station)
	{

	}

	void DigitalCentralStation::DebugInjector::injectTurnoutUpdate(Turnout& turnout, const Turnout::Direction direction)
	{
		this->station->turnoutUpdate(turnout, direction);
	}

	void DigitalCentralStation::DebugInjector::injectDoubleSlipTurnoutUpdate(DoubleSlipTurnout& turnout, const DoubleSlipTurnout::Direction direction)
	{
		this->station->doubleSlipUpdate(turnout, direction);
	}

	void DigitalCentralStation::DebugInjector::injectLocoUpdate(Locomotive& loco, bool busy, bool forward, unsigned char speed, uint32_t functions)
	{
		this->station->callbacks.locomotiveUpdateCallback(loco, busy, forward, speed, functions);
	}

	void DigitalCentralStation::DebugInjector::injectConnected()
	{
		this->station->onConnected();
	}

	void DigitalCentralStation::keepAlive()
	{

	}

	void DigitalCentralStation::turnoutUpdate(Turnout& turnout, const Turnout::Direction direction)
	{
		this->signalTower->order(Command::make([&turnout, direction](const TimePoint& created) -> const State
			{
				return turnout.finalizeChangeTo(direction);
			}, __PRETTY_FUNCTION__));
	}

	void DigitalCentralStation::doubleSlipUpdate(DoubleSlipTurnout& turnout, const DoubleSlipTurnout::Direction direction)
	{
		this->signalTower->order(Command::make([&turnout, direction](const TimePoint& created) -> const State
			{
				return turnout.finalizeChangeTo(direction);
			}, __PRETTY_FUNCTION__));
	}

	const bool DigitalCentralStation::connected() const
	{
		return this->connectedToDCS;
	}

	const Result DigitalCentralStation::onConnected()
	{
		const auto result = this->connectedInternal();
		this->connectedToDCS = result == Result::OK;
		this->callbacks.connectedCallback();
		return result;
	}
};