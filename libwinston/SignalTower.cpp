#include <unordered_set>

#include "Winston.h"
#include "SignalTower.h"
#include "HAL.h"
#include "Railway.h"
#include "Track.h"

namespace winston
{
	SignalTower::SignalTower() : EventLooper()
	{
	}

	Railway::Callbacks::TurnoutUpdateCallback SignalTower::injectTurnoutSignalHandling(Railway::Callbacks::TurnoutUpdateCallback callback)
	{
		return [this, callback](Turnout& turnout, Turnout::Direction direction) -> State {
			State state = callback(turnout, direction);
			if (state == State::Finished)
				this->setSignalsFor(turnout);
			return state;
			};
	}

	void SignalTower::initSignalsForTurnouts(std::set<Turnout::Shared> turnouts, std::set<DoubleSlipTurnout::Shared> doubleSlipTurnouts)
	{
		for (auto& turnout : turnouts)
			this->setSignalsFor(*turnout);
		for (auto& doubleSlipTurnout : doubleSlipTurnouts)
			this->setSignalsFor(*doubleSlipTurnout);
	}

	void SignalTower::setSignalOn(Track& track, const Track::Connection signalGuardedConnection, const Signal::Aspect aspect, const Signal::Aspect preAspect)
	{
		const auto preSignalAspect = aspect & Signal::Aspect::Go ? Signal::Aspect::ExpectGo : Signal::Aspect::ExpectHalt;

		auto current = track.shared_from_this();
		auto connection = signalGuardedConnection;
		// current and from are now the position of mainSignal
		if (Signal::Shared mainSignal = track.signalGuarding(connection))
		{
			if (preAspect == Signal::Aspect::Off)
				mainSignal->aspect(preAspect);
			mainSignal->aspect(aspect);
			if (preAspect != Signal::Aspect::Off)
				mainSignal->aspect(preAspect);
			auto otherConnection = current->otherConnection(connection);
			Signal::Shared preSignal;
			Track::traverse<Track::TraversalSignalHandling::OppositeDirection, true>(current, otherConnection, preSignal);
			if (preSignal)
				preSignal->aspect(preSignalAspect);
		}
	}

	void SignalTower::setSignalsFor(Track& turnout, const Track::Connection connectionStartFrom)
	{
		Track::Shared signalCurrent = turnout.shared_from_this();
		auto signalConnection = connectionStartFrom;
		auto signalToSet = this->nextSignal(signalCurrent, true, signalConnection, true, true);

		Track::Shared current = signalCurrent;
		auto connection = signalConnection;
		Signal::Shared signal;
		auto result = Track::traverse<Track::TraversalSignalHandling::ForwardDirection, false>(current, connection, signal);

		auto aspect = Signal::Aspect::Halt;
		auto preAspect = Signal::Aspect::Off;
		switch (result)
		{
		case Track::TraversalResult::Bumper:
		case Track::TraversalResult::Looped:
			aspect = Signal::Aspect::Go;
			break;
		case Track::TraversalResult::Signal:
			aspect = Signal::Aspect::Go;
			if (signal->shows(Signal::Aspect::Halt))
				preAspect = Signal::Aspect::ExpectHalt;
			break;
		default:
		case Track::TraversalResult::OpenTurnout: aspect = Signal::Aspect::Halt; break;
		}
		this->setSignalOn(*signalCurrent, signalConnection, aspect, preAspect);
	}

	void SignalTower::setSignalsFor(Track& track)
	{
		track.eachConnection([this](Track& track, const Track::Connection connection) {
			this->order(Command::make([this, &track, connection](const TimePoint& created) -> const winston::State {
				this->setSignalsFor(track, connection);
				return State::Finished;
				}, __PRETTY_FUNCTION__));
			});
	}

	Signal::Shared SignalTower::nextSignal(Track::Shared& track, const bool guarding, Track::Connection& leaving, const bool main, const bool includingFirst)
	{
		Track::Connection connection = leaving;
		Track::Shared onto = track;
		Signal::Shared signal;
		Track::TraversalResult result;

		if (connection == Track::Connection::DeadEnd)
			return nullptr;

		if (!includingFirst)
		{
			if (!track->traverse(connection, onto, true))
				return nullptr;
			connection = onto->otherConnection(onto->whereConnects(track));
		}

		if (guarding)
		{
			result = Track::traverse<Track::TraversalSignalHandling::OppositeDirection, true>(onto, connection, signal);
		}
		else
		{
			result = Track::traverse<Track::TraversalSignalHandling::ForwardDirection, true>(onto, connection, signal);
		}

		switch (result)
		{
		case Track::TraversalResult::Bumper:
		case Track::TraversalResult::Looped:
		case Track::TraversalResult::OpenTurnout: return nullptr;
		case Track::TraversalResult::Signal:
			if (signal)
			{
				if ((signal->mainSignal() && main) || (signal->preSignal() && !main))
				{
					track = onto;
					leaving = connection;
					return signal;
				}
				else if (signal->mainSignal() && !main)
					return nullptr;
			}
			break;
		}

		return nullptr;
	}

	void SignalTower::setSignalsForLocoPassing(Track::Const track, const Track::Connection connection, const Signal::Pass pass) const
	{

	}
}
