#pragma once

#include "WinstonTypes.h"
#include "Event.h"

namespace winston
{
	class Task : public Uniqe_Ptr<Task>
	{
	public:
		enum class State
		{
			Running,
			Finished
		};

		Task(Event::Unique event);
		virtual const State execute() = 0;
		unsigned long age();
		void finished();
	private:
		unsigned long created;
		Event::Unique event;
	};
}