#pragma once
/*
#include "Event.h"
#include "Rail.h"
#include "WinstonTypes.h"

namespace winston
{
	class EventTurnoutStartToggle : public Event, Uniqe_Ptr<EventTurnoutStartToggle>
	{
	public:
		EventTurnoutStartToggle(Callback::Shared finishedCallback, Turnout::Shared& turnout);
		Turnout::Shared& turnout();

		using Uniqe_Ptr<EventTurnoutStartToggle>::Unique;
		using Uniqe_Ptr<EventTurnoutStartToggle>::make;

	private:
		Turnout::Shared section;
	};

	class EventTurnoutFinalizeToggle : public Event, Uniqe_Ptr<EventTurnoutFinalizeToggle>
	{
	public:
		EventTurnoutFinalizeToggle(Callback::Shared finishedCallback, Turnout::Shared& turnout, const Turnout::Direction direction);

		Turnout::Shared& turnout();
		const Turnout::Direction direction() const;

		using Uniqe_Ptr<EventTurnoutFinalizeToggle>::Unique;
		using Uniqe_Ptr<EventTurnoutFinalizeToggle>::make;
	private:
		Turnout::Shared section;
		const Turnout::Direction dir;
	};
}*/