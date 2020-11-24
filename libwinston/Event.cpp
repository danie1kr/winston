/*
#include "Event.h"
#include "SignalBox.h"

namespace winston
{
	Payload::Payload(EventCallback evaluate, TaskCallback execute)
		: eventCallback(evaluate), taskCallback(execute)
	{

	}

	const State Payload::evaluate(SignalBox::Shared& signalBox)
	{
		return this->eventCallback(signalBox);
	}

	const State Payload::execute(SignalBox::Shared& signalBox)
	{
		return this->taskCallback(signalBox);
	}


	Event::Event(Payload::Shared payload, Callback::Shared taskFinishedCallback)
		: _payload(payload), Uniqe_Ptr<Event>(), callback(taskFinishedCallback->use())
	{
	}

	Payload::Shared Event::payload()
	{
		return this->_payload;
	}

	void Event::finished()
	{
		this->callback->execute();
	}
}*/