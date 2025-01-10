#pragma once

#include <memory>
#include <queue>
#include "Command.h"
#include "Util.h"
#include "Signal.h"
#include "WinstonTypes.h"
#include "Railway.h"
#include "EventLooper.h"
#include "Locomotive.h"

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

		SignalTower(const LocomotiveShed &locomotiveShed);

		Railway::Callbacks::TurnoutUpdateCallback injectTurnoutSignalHandling(Railway::Callbacks::TurnoutUpdateCallback callback);

		void initSignalsForTurnouts(std::set<Turnout::Shared> turnouts, std::set<DoubleSlipTurnout::Shared> doubleSlipTurnouts);
		const Track::TraversalResult findSignalsFor(Track::Const& current, Track::Connection& connection, Signal::Shared& signal) const;
		void setSignalsFor(const Track::TraversalResult traversalResult, Track::Const current, Track::Connection connection, Signal::Shared signal) const;
		void setSignalsFor(Track& turnout, const Track::Connection connectionStartFrom, const bool requireLocoNextSignalUpdate = true);
		void setSignalsFor(Track& turnout);

		void setSignalsForLocoPassing(Track::Const track, const Track::Connection connection, const Signal::Pass pass) const;
		static void setSignalsForLoco(const Locomotive::Const loco);

		static void setSignalOn(const Track& track, const Track::Connection signalGuardedConnection, const Signal::Aspect aspect, const Signal::Aspect preAspect = Signal::Aspect::Off, const Signal::Authority = Signal::Authority::Turnout);

		using Shared_Ptr<SignalTower>::Shared;
		using Shared_Ptr<SignalTower>::make;

		static Signal::Shared nextSignal(Track::Const& track, const bool guarding, Track::Connection& leaving, const bool main, const bool includingFirst);

		static const bool findNextSignal(Track::Const& track, Track::Connection& leaving, Distance& traveled, const Signal::Pass pass, Signal::Shared& signal);
		static NextSignal::Const nextSignal(const Position position, const Signal::Pass pass);

		static const bool setupNextSignal(Track::Shared track, const Track::Connection leaving, const Signal::Pass pass);

	private:
		const LocomotiveShed& locomotiveShed;
		/*std::queue<Command::Shared> commands;

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	public:
		const std::string statistics(const size_t withTop = 0) const;
	private:
		StopwatchJournal stopwatchJournal;
#endif*/
	};
}