
#include "Task.h"
#include "HAL.h"

namespace winston
{
	Task::Task(Event::Unique event)
		: Uniqe_Ptr<Task>(), event(std::move(event)), created(hal::now())
	{
	}
	unsigned long Task::age()
	{
		return hal::now() - this->created;
	}
	void Task::finished()
	{
	//	Event::finished(std::move(this->event));
		this->event->finished();
	}
}