#pragma once

#include <queue>
#include "WinstonTypes.h"
#include "Command.h"

namespace winston
{
	class EventLooper : public Looper, public Shared_Ptr<EventLooper>
	{
	public:
		EventLooper();
		~EventLooper() = default;

		void order(Command::Shared command);
		const Result loop();
		using Shared_Ptr<EventLooper>::Shared;
		using Shared_Ptr<EventLooper>::make;

	private:		
		std::queue<Command::Shared> commands;

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	public:
		const std::string statistics(const size_t withTop = 0) const;
	private:
		StopwatchJournal stopwatchJournal;
#endif
	};
}