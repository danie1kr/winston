#include "EventLooper.h"

namespace winston
{

	EventLooper::EventLooper()
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		: stopwatchJournal("EventLooper")
#endif
	{
	}

	void EventLooper::order(Command::Shared command)
	{
		this->commands.push(std::move(command));
	}

	bool EventLooper::work()
	{
		if (this->commands.size() > 0)
		{
			auto command = std::move(this->commands.front());
			this->commands.pop();

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
			auto now = hal::now();
#endif
			auto state = command->execute();

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
			if (state != State::Delay)
				this->stopwatchJournal.add(hal::now() - now, command->name());
#endif
			if (state == State::Running || state == State::Delay)
				this->commands.push(std::move(command));

			return state == State::Running || state == State::Finished;
		}
		return false;
	}

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	const std::string EventLooper::statistics(const size_t withTop) const
	{
		return this->stopwatchJournal.toString(withTop);
	}
#endif
}