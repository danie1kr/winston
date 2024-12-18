#pragma once

#include <memory>
#include <queue>
#include "Command.h"
#include "Util.h"
#include "Signal.h"
#include "WinstonTypes.h"
#include "Railway.h"
#include "EventLooper.h"

namespace winston
{
	class SignalTower : public EventLooper, public Shared_Ptr<SignalTower>
	{
	public:

		enum class Callback
		{
			TurnoutToggled,
			Count
		};

		SignalTower();

		Railway::Callbacks::TurnoutUpdateCallback injectTurnoutSignalHandling(Railway::Callbacks::TurnoutUpdateCallback callback);

		void initSignalsForTurnouts(std::set<Turnout::Shared> turnouts, std::set<DoubleSlipTurnout::Shared> doubleSlipTurnouts);
		void setSignalsFor(Track& turnout, const Track::Connection connectionStartFrom);
		void setSignalsFor(Track& turnout);

		void setSignalsForLocoPassing(Track::Const track, const Track::Connection connection, const Signal::Pass pass) const;

		static void setSignalOn(Track& track, const Track::Connection signalGuardedConnection, const Signal::Aspect aspect, const Signal::Aspect preAspect = Signal::Aspect::Off);

		using Shared_Ptr<SignalTower>::Shared;
		using Shared_Ptr<SignalTower>::make;
	private:

		static Signal::Shared nextSignal(Track::Shared& track, const bool guarding, Track::Connection& leaving, const bool main, const bool includingFirst);
		/*std::queue<Command::Shared> commands;

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	public:
		const std::string statistics(const size_t withTop = 0) const;
	private:
		StopwatchJournal stopwatchJournal;
#endif*/
	};
}