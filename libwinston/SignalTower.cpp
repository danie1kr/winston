#include <unordered_set>

#include "Winston.h"
#include "SignalTower.h"
#include "HAL.h"
#include "Railway.h"
#include "Track.h"

namespace winston
{
	SignalTower::SignalTower(const LocomotiveShed& locomotiveShed) : EventLooper(), locomotiveShed(locomotiveShed)
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

	void SignalTower::setSignalOn(const Track& track, const Track::Connection signalGuardedConnection, const Signal::Aspect aspect, const Signal::Aspect preAspect, const Signal::Authority authority)
	{
		const auto preSignalAspect = aspect & Signal::Aspect::Go ? Signal::Aspect::ExpectGo : Signal::Aspect::ExpectHalt;

		auto current = track.const_from_this();
		auto connection = signalGuardedConnection;
		// current and from are now the position of mainSignal
		if (Signal::Shared mainSignal = track.signalGuarding(connection))
		{
			if (preAspect == Signal::Aspect::Off)
				mainSignal->aspect(preAspect, authority);
			mainSignal->aspect(aspect);
			if (preAspect != Signal::Aspect::Off)
				mainSignal->aspect(preAspect, authority);
			auto otherConnection = current->otherConnection(connection);
			Signal::Shared preSignal;
			Track::traverse<Track::TraversalSignalHandling::OppositeDirection, true>(current, otherConnection, preSignal);
			if (preSignal)
				preSignal->aspect(preSignalAspect);
		}
	}

	const Track::TraversalResult SignalTower::findSignalsFor(Track::Const& current, Track::Connection &connection, Signal::Shared& signal) const
	{
		Track::Const signalCurrent = current;
		auto signalConnection = connection;
		auto signalToSet = this->nextSignal(signalCurrent, true, signalConnection, true, true);

		Track::Const currentTrack = signalCurrent;
		auto currentConnection = signalConnection;
		auto result = Track::traverse<Track::TraversalSignalHandling::ForwardDirection, false>(currentTrack, currentConnection, signal);

		if (!signal)
			signal = signalToSet;

		current = signalCurrent;
		connection = signalConnection;

		return result;
	}

	void SignalTower::setSignalsFor(const Track::TraversalResult traversalResult, Track::Const current, Track::Connection connection, Signal::Shared signal) const
	{
		auto aspect = Signal::Aspect::Halt;
		auto preAspect = Signal::Aspect::Off;
		switch (traversalResult)
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
		this->setSignalOn(*current, connection, aspect, preAspect);
	}

	void SignalTower::setSignalsFor(Track& turnout, const Track::Connection connectionStartFrom, const bool requireLocoNextSignalUpdate)
	{
		Track::Const signalCurrent = turnout.const_from_this();
		auto signalConnection = connectionStartFrom;
		Signal::Shared signal;
		auto result = findSignalsFor(signalCurrent, signalConnection, signal);
		this->setSignalsFor(result, signalCurrent, signalConnection, signal);

#ifdef WINSTON_DETECTOR_SIGNALING
		if (signal)
		{
			// is there a loco behind this signal?
			// loco behind signal == loco.nextSignals.contains(signal)
			for (auto loco : this->locomotiveShed.shed())
			{
				if (requireLocoNextSignalUpdate)
					loco->updateNextSignals();
				if (loco->isNextSignal(signal))
					this->setSignalOn(*signalCurrent, signalConnection, Signal::Aspect::Halt, Signal::Aspect::Off, Signal::Authority::Occupancy);
				else
					signal->clearAuthorities();
			}
		}
#endif
	}

	void SignalTower::setSignalsFor(Track& track)
	{
#ifdef WINSTON_DETECTOR_SIGNALING
		// update loco next signals once
		this->order(Command::make([this](const TimePoint& created) -> const winston::State {
			for (auto loco : this->locomotiveShed.shed())
				loco->updateNextSignals();
			return State::Finished;
			}, __PRETTY_FUNCTION__));
#endif
		track.eachConnection([this](Track& track, const Track::Connection connection) {
			this->order(Command::make([this, &track, connection](const TimePoint& created) -> const winston::State {
				this->setSignalsFor(track, connection, false);
				return State::Finished;
				}, __PRETTY_FUNCTION__));
			});	
	}

	Signal::Shared SignalTower::nextSignal(Track::Const& track, const bool guarding, Track::Connection& leaving, const bool main, const bool includingFirst)
	{
		Track::Connection connection = leaving;
		Track::Const onto = track;
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
		case Track::TraversalResult::CancelledByCallback:
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
		/*
		daniel, [22.12.2024 17:41]
traverseUpTo(Position, trainLength), ouput Blocks to occupy/signals to set to halt

daniel, [22.12.2024 17:46]
signalPassed:
- facing: green current block (nach hinten bis zum nächsten facing, als wenn wir rückwärts führen), halt next block (passed signal)
- backside: green current block (passed signal ), halt entering block (vorwärts bis zum nächsten backside)

daniel, [22.12.2024 18:01]
turnout toggling on occupied block? AB-AC: 
	Signal guarding B stays red, but now by turnout. 
	Remove occupation authority, add halt to C because of this. 
	Re-eval block or just move authority over?

daniel, [22.12.2024 18:02]
Future: Signals for Position, Signals for Train (Position, length, direction)

Pre-Calculate signals for turnout
*/
//
		if (pass == Signal::Pass::Facing)
		{
			// green current block we are leaving (nach hinten bis zum nächsten facing, als wenn wir rückwärts führen)
			auto signalOfLeftBlockTrack = track;
			auto signalOfLeftBlockConnection = track->otherConnection(connection);
			Signal::Shared signal;
			Track::Const signalTrack;
			Track::Connection signalConnection;
			Distance distance;
			// we walk backwards, we see the backside of the signal of the block we left 
			if(signalOfLeftBlockTrack->nextSignal(signalOfLeftBlockConnection, Signal::Pass::Backside, signal, signalTrack, signalConnection, distance))
				this->setSignalOn(*signalTrack, signalConnection, Signal::Aspect::Go, Signal::Aspect::Off, Signal::Authority::Occupancy);

			// halt next block (passed signal)
			this->setSignalOn(*track, connection, Signal::Aspect::Halt, Signal::Aspect::Off);
		}
		else
		{
			// green current block(passed signal)
			this->setSignalOn(*track, connection, Signal::Aspect::Go, Signal::Aspect::Off, Signal::Authority::Occupancy);

			// halt entering block(vorwärts bis zum nächsten backside)
			auto signalOfEnteredBlockTrack = track;
			auto signalOfEnteredBlockConnection = track->otherConnection(connection);
			Signal::Shared signal;
			Track::Const signalTrack;
			Track::Connection signalConnection;
			Distance distance;
			if (signalOfEnteredBlockTrack->nextSignal(signalOfEnteredBlockConnection, pass, signal, signalTrack, signalConnection, distance))
				this->setSignalOn(*signalTrack, signalConnection, Signal::Aspect::Halt, Signal::Aspect::Off, Signal::Authority::Occupancy);
		}
		/*
		if (pass == Signal::Pass::Facing)
		{
			// green current block we are leaving (nach hinten bis zum nächsten facing, als wenn wir rückwärts führen)
			auto signalOfLeftBlockTrack = track;
			auto signalOfLeftBlockConnection = connection;
			if (auto signalOfLeftBlock = this->nextSignal(signalOfLeftBlockTrack, false, signalOfLeftBlockConnection, true, false))
				this->setSignalOn(*signalOfLeftBlockTrack, signalOfLeftBlockConnection, Signal::Aspect::Go, Signal::Aspect::Off, Signal::Authority::Occupancy);

			// halt next block (passed signal)
			this->setSignalOn(*track, connection, Signal::Aspect::Halt, Signal::Aspect::Off);
		}
		else
		{
			// green current block(passed signal)
			this->setSignalOn(*track, connection, Signal::Aspect::Go, Signal::Aspect::Off, Signal::Authority::Occupancy);

			// halt entering block(vorwärts bis zum nächsten backside)
			auto signalOfEnteredTrack = track;
			auto signalOfEnteredBlockConnection = track->otherConnection(connection);
			if (auto signalOfEnteredBlock = this->nextSignal(signalOfEnteredTrack, true, signalOfEnteredBlockConnection, true, false))
				this->setSignalOn(*signalOfEnteredTrack, signalOfEnteredBlockConnection, Signal::Aspect::Halt, Signal::Aspect::Off);
		}
		*/
	}

	void SignalTower::setSignalsForLoco(const Locomotive::Const loco)
	{
		auto next = loco->nextSignal(Signal::Pass::Backside, true);
		if (next && next->signal && next->track)
			SignalTower::setSignalOn(*next->track, next->connection, Signal::Aspect::Halt, Signal::Aspect::Off);
		auto prev = loco->nextSignal(Signal::Pass::Backside, false);
		if (prev && prev->signal && prev->track)
			SignalTower::setSignalOn(*prev->track, prev->connection, Signal::Aspect::Halt, Signal::Aspect::Off);
	}

	const bool SignalTower::findNextSignal(Track::Const &track, Track::Connection &leaving, Distance& traveled, const Signal::Pass pass, Signal::Shared& signal)
	{
		return track->nextSignal(leaving, pass, signal, track, leaving, traveled);
		/*
		if (track->nextSignal(leaving, pass, signal, track, leaving, traveled))
		{
			traveled += nextSignal->distance;
			signal = nextSignal->signal;

			return true;
		}*/

		return false;
		/*
		auto current = track;
		auto connection = leaving;
		auto onto = current;
		std::unordered_set<Track::Const> visited;
		bool successful = true;
		while (true)
		{
			visited.insert(current);
			// step onto next track
			successful = current->traverse(connection, onto, true);
			if (!successful)
				return false;

			// we looped
			if (visited.find(onto) != visited.end())
				return false;

			connection = onto->whereConnects(current);
			// if pass == backside, check entering connection
			if (pass == Signal::Pass::Backside)
			{
				signal = onto->signalGuarding(connection);
				if (signal)
				{
					traveled += signal->distance();
					return true;
				}
			}

			// if pass == facing, check leaving connection
			connection = onto->otherConnection(connection);
			if (pass == Signal::Pass::Facing)
			{
				signal = onto->signalGuarding(connection);
				if (signal)
				{
					traveled += onto->length() - signal->distance();
					return true;
				}
			}

			current = onto;
		}
		return false;
		*/
	}
	
	NextSignal::Const SignalTower::nextSignal(const Position position, const Signal::Pass pass)
	{
		NextSignals signals;
		Track::Const current = position.track();
		if (pass == Signal::Pass::Facing)
		{
			// the signal facing us is guarding the other connection
			auto connection = current->otherConnection(position.connection());
			auto signal = current->signalGuarding(connection);
			if (signal)
			{
				// signal on the current track
				const auto signalDistanceFromReference = current->length() - signal->distance();
				if (signalDistanceFromReference > (unsigned)position.distance())
					return NextSignal::make(signal, current, connection, signalDistanceFromReference - position.distance(), pass);
			}

			// signal on the following track
			Distance distance = current->length() - position.distance();
			if (SignalTower::findNextSignal(current, connection, distance, pass, signal))
				return NextSignal::make(signal, current, connection, distance, pass);
		}
		else
		{
			// the signal backside is guarding our reference connection
			auto connection = position.connection();
			auto signal = current->signalGuarding(connection);
			if (signal)
			{
				// signal on the current track
				const auto signalDistanceFromReference = signal->distance();
				if (signalDistanceFromReference > (unsigned)position.distance())
					return NextSignal::make(signal, current, connection, signalDistanceFromReference - position.distance(), pass);
			}

			// signal on the following track, leave on otherConnection than our reference
			Distance distance = current->length() - position.distance();
			connection = current->otherConnection(connection);
			if (SignalTower::findNextSignal(current, connection, distance, pass, signal))
				return NextSignal::make(signal, current, connection, distance, pass);
		}

		return nullptr;
	}

	const bool SignalTower::setupNextSignal(Track::Shared track, const Track::Connection leaving, const Signal::Pass pass)
	{
		Distance distance = 0;

		Track::Const current = track;
		auto connection = leaving;
		winston::Track::Const onto = current;
		std::unordered_set<Track::Const> visited;
		bool successful = true;
		while (true)
		{
			visited.insert(current);
			// step onto next track
			successful = current->traverse(connection, onto, true);

			// should always be true!
			if (!successful)
				return false;

			// we looped
			if (visited.find(onto) != visited.end())
				return false;

			connection = onto->whereConnects(current);
			if (onto->type() == Track::Type::Turnout || onto->type() == Track::Type::DoubleSlipTurnout)
			{
				auto nextTurnout = Track::NextSignalProvider::NextTurnout::make(onto, connection);
				auto provider = Track::NextSignalProvider::make(distance, nextTurnout);
				track->setupNextSignal(leaving, pass, provider);
				return true;
			}

			// if pass == backside, check entering connection
			if (pass == Signal::Pass::Backside)
			{
				if (auto signal = onto->signalGuarding(connection))
				{
					distance += signal->distance();
					auto provider = Track::NextSignalProvider::make(distance, signal, onto, connection);
					track->setupNextSignal(leaving, pass, provider);
					return true;
				}
			}

			// if pass == facing, check leaving connection
			connection = onto->otherConnection(connection);
			if (pass == Signal::Pass::Facing)
			{
				if (auto signal = onto->signalGuarding(connection))
				{
					distance += onto->length() - signal->distance();
					auto provider = Track::NextSignalProvider::make(distance, signal, onto, connection);
					track->setupNextSignal(leaving, pass, provider);
					return true;
				}
			}
			distance += onto->length();
			current = onto;
		}
		return false;
	}
}
