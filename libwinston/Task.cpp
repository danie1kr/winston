
/*#include "Task.h"
#include "HAL.h"

namespace winston
{
	Task::Task(Payload::Shared payload, Event::Unique event)
		: _payload(payload), Uniqe_Ptr<Task>(), event(std::move(event)), created(hal::now())
	{
	}
	unsigned long Task::age()
	{
		return hal::now() - this->created;
	}

	const State Task::execute()
	{
		return this->_payload->execute();
	}

	Payload::Shared Task::payload()
	{
		return this->_payload;
	}

	void Task::finished()
	{
	//	Event::finished(std::move(this->event));
		this->event->finished();
	}
}*/