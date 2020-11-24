/*#include "Events.h"

namespace winston
{
	EventTurnoutStartToggle::EventTurnoutStartToggle(Callback::Shared taskFinishedCallback, Turnout::Shared& turnout)
		: Event(taskFinishedCallback), Uniqe_Ptr<EventTurnoutStartToggle>(), section(turnout)
	{
	}
	Turnout::Shared& EventTurnoutStartToggle::turnout() { return section; };

	EventTurnoutFinalizeToggle::EventTurnoutFinalizeToggle(Callback::Shared taskFinishedCallback, Turnout::Shared& turnout, const Turnout::Direction direction)
		: Event(taskFinishedCallback), Uniqe_Ptr<EventTurnoutFinalizeToggle>(), section(turnout), dir(direction)
	{
	}
	const Turnout::Direction EventTurnoutFinalizeToggle::direction() const  { return dir; };
	Turnout::Shared& EventTurnoutFinalizeToggle::turnout() { return section; };

	/*const Event::Type EventTurnoutToggle::type() const
	{
		return Event::Type::TurnoutToggle;
	}

	const unsigned int EventTurnoutToggle::turnout() const
	{
		return this->id;
	}*
}*/