#pragma once

#include <memory>
#include <queue>
#include "Command.h"
#include "Util.h"
#include "Signal.h"
#include "WinstonTypes.h"
#include "Railway.h"

namespace winston
{
	class SignalBox : public Shared_Ptr<SignalBox>, public std::enable_shared_from_this<SignalBox>
	{
	public:

		enum class Callback
		{
			TurnoutToggled,
			Count
		};

		SignalBox();

		Railway::Callbacks::TurnoutUpdateCallback injectTurnoutSignalHandling(Railway::Callbacks::TurnoutUpdateCallback callback);

		void initSignalsForTurnouts(std::set<Turnout::Shared> turnouts, std::set<DoubleSlipTurnout::Shared> doubleSlipTurnouts);
		void setSignalsFor(Turnout::Shared turnout, const Track::Connection connectionStartFrom);
		void setSignalsFor(Turnout::Shared turnout, const Turnout::Direction direction);
		void setSignalsFor(DoubleSlipTurnout::Shared turnout, const Track::Connection connectionStartFrom, Track::Shared track);
		void setSignalsForChangingTurnout(Turnout::Shared turnout, const Turnout::Direction targetDirection);
		void setSignalsForChangingDoubleSlipTurnout(DoubleSlipTurnout::Shared turnout, const DoubleSlipTurnout::Direction targetDirection);

		static void setSignalOn(Track::Shared track, const Track::Connection signalGuardedConnection, const Signal::Aspect aspect, const Signal::Aspect preAspect = Signal::Aspect::Off);
		
		void order(Command::Shared command);
		bool work();
		using Shared_Ptr<SignalBox>::Shared;
		using Shared_Ptr<SignalBox>::make;
	private:

		static Signal::Shared nextSignal(Track::Shared& track, const bool guarding, Track::Connection& leaving, const bool main, const bool includingFirst);
		std::queue<Command::Shared> commands;

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	public:
		const std::string statistics(const size_t withTop = 0) const;
	private:
		StopwatchJournal stopwatchJournal;
#endif
	};
}