#include "SignalBox.h"
#include "HAL.h"
#include "Tasks.h"
#include "Rail.h"

namespace winston
{/*
	TaskSignal::TaskSignal(Signal& signal, Signal::Aspect aspect)
		: Task(), signal(signal), aspect(aspect)
	{

	}
	*/
	TaskTurnoutStartToggle::TaskTurnoutStartToggle(Event::Unique event, Turnout::Shared& turnout)
		: Task(std::move(event)), turnout(turnout), Uniqe_Ptr<TaskTurnoutStartToggle>()
	{

	}

	const Task::State TaskTurnoutStartToggle::execute()
	{
		turnout->startToggle();
		return State::Finished;
	}

	TaskTurnoutFinalizeToggle::TaskTurnoutFinalizeToggle(Event::Unique event, Turnout::Shared& turnout, const Turnout::Direction direction)
		: Task(std::move(event)), turnout(turnout), direction(direction), Uniqe_Ptr<TaskTurnoutFinalizeToggle>()
	{

	}

	const Task::State TaskTurnoutFinalizeToggle::execute()
	{
		turnout->finalizeChangeTo(this->direction);
		return State::Finished;
	}


}