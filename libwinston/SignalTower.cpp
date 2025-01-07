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

	void SignalTower::setSignalsFor(Track& turnout, const Track::Connection connectionStartFrom)
	{
		Track::Const signalCurrent = turnout.const_from_this();
		auto signalConnection = connectionStartFrom;
		Signal::Shared signal;
		auto result = findSignalsFor(signalCurrent, signalConnection, signal);
		this->setSignalsFor(result, signalCurrent, signalConnection, signal);
		
#ifdef WINSTON_DETECTOR_SIGNALING
		// is there a loco behind this signal?
		// loco behind signal == loco.nextSignals.contains(signal)
		for (auto loco : this->locomotiveShed)
		{
			loco->updateNextSignals();
			if (loco->isNextSignal(signal))
				this->setSignalsForLocoPassing(signalCurrent, signalConnection, Signal::Pass::Backside);
		}
#endif
		/*
		// align signal with what is set on the other block entry (if there is a loco, take over it's occupied authority)
		{
			if (signal)
			{
				// 
				auto otherBlockSignal = this->nextSignal(signalCurrent, true, signalConnection, true, false);
				if (auto otherBlockSignal = this->nextSignal(signalCurrent, true, signalConnection, true, false))
				{
					signal->grabAuthorities(otherBlockSignal);
					if (otherBlockSignal->shows(Signal::Aspect::Halt))
						signal->aspect(Signal::Aspect::Halt, Signal::Authority::Occupancy);
				}
			}
		}
		*/
	}

	void SignalTower::setSignalsFor(Track& track)
	{
		/* wrong :(
		if (track.type() == Track::Type::Turnout)
		{
			Signal::Shared signalA, signalB, signalC;
			Track::Const trackA = track.const_from_this(), trackB = track.const_from_this(), trackC = track.const_from_this();
			Track::Connection connectionA = Track::Connection::A, connectionB = Track::Connection::B, connectionC = Track::Connection::C;
			auto resultA = this->findSignalsFor(trackA, connectionA, signalA);
			auto resultB = this->findSignalsFor(trackB, connectionB, signalB);
			auto resultC = this->findSignalsFor(trackC, connectionC, signalC);
			
			if(signalB && signalC)
				signalB->swapAuthorities(signalC);
			else
			{
				if (signalB)
					signalB->clearAuthorities();
				if (signalC)
					signalC->clearAuthorities();
			}
			/*
			this->setSignalsFor(resultA, trackA, connectionA, signalA);
			this->setSignalsFor(resultB, trackB, connectionB, signalB);
			this->setSignalsFor(resultC, trackC, connectionC, signalC);
		}
		else if (track.type() == Track::Type::DoubleSlipTurnout)
		{
			Signal::Shared signalA, signalB, signalC, signalD;
			Track::Const trackA = track.const_from_this(), trackB = track.const_from_this(), trackC = track.const_from_this(), trackD = track.const_from_this();
			Track::Connection connectionA = Track::Connection::A, connectionB = Track::Connection::B, connectionC = Track::Connection::C, connectionD = Track::Connection::D;
			auto resultA = this->findSignalsFor(trackA, connectionA, signalA);
			auto resultB = this->findSignalsFor(trackB, connectionB, signalB);
			auto resultC = this->findSignalsFor(trackC, connectionC, signalC);
			auto resultD = this->findSignalsFor(trackD, connectionD, signalD);

			if (signalA && signalD)
				signalA->swapAuthorities(signalD);
			else
			{
				if (signalA)
					signalA->clearAuthorities();
				if (signalD)
					signalD->clearAuthorities();
			}
			if (signalB && signalC)
				signalB->swapAuthorities(signalC);
			else
			{
				if (signalB)
					signalB->clearAuthorities();
				if (signalC)
					signalC->clearAuthorities();
			}
			/*
			this->setSignalsFor(resultA, trackA, connectionA, signalA);
			this->setSignalsFor(resultB, trackB, connectionB, signalB);
			this->setSignalsFor(resultC, trackC, connectionC, signalC);
			this->setSignalsFor(resultD, trackD, connectionD, signalD);
		}
		//else*/
			track.eachConnection([this](Track& track, const Track::Connection connection) {
				this->order(Command::make([this, &track, connection](const TimePoint& created) -> const winston::State {

					this->setSignalsFor(track, connection);
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

	Signal::Shared SignalTower::otherBlockSignal(const Track::Const &track, const Track::Connection &connection)
	{
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

		/*
		if (pass == Signal::Pass::Facing)
		{
			// we entered a protected track:
			// - set this signal to red, set pre-signal to red if exists
			// - set other signal into this track to red, update its pre-signal
			// - set signal of the left track to green, update its pre-signal

			this->setSignalOn(*track, connection, Signal::Aspect::Halt, Signal::Aspect::Off);

			auto otherBlockSignalTrack = track;
			auto otherBlockSignalConnection = connection;
			if (auto otherBlockSignal = this->otherBlockSignal(otherBlockSignalTrack, otherBlockSignalConnection))
				this->setSignalOn(*otherBlockSignalTrack, otherBlockSignalConnection, Signal::Aspect::Halt, Signal::Aspect::Off);

			auto thisSignal = track->signalGuarding(connection);

			auto signalOfLeftBlockTrack = track;
			auto signalOfLeftBlockConnection = connection;
			if (auto signalOfLeftBlock = this->nextSignal(signalOfLeftBlockTrack, false, signalOfLeftBlockConnection, true, false))
				this->setSignalOn(*signalOfLeftBlockTrack, signalOfLeftBlockConnection, Signal::Aspect::Go, Signal::Aspect::Off);

		}
		else
		{

		}
		*/
	}

	const bool SignalTower::findNextSignal(Track::Const track, Track::Connection leaving, Distance& traveled, const Signal::Pass pass, Signal::Shared& signal)
	{
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
	}
	
	NextSignal::Const SignalTower::nextSignal(const Position position, const Signal::Pass pass)
	{
		NextSignals signals;
		Track::Const current = position.track();
		auto connection = current->otherConnection(position.connection());
		auto signal = current->signalGuarding(connection);
		if (signal)
		{
			// signal on the current track
			const auto signalDistanceFromReference = current->length() - signal->distance();
			if (signalDistanceFromReference >= position.distance())
				return NextSignal::make(signal, position.distance() - signalDistanceFromReference, pass);
		}
		//else
		{
			// signal on the following track
			Distance distance = current->length() - position.distance();
			if (SignalTower::findNextSignal(current, connection, distance, pass, signal))
				return NextSignal::make(signal, distance, pass);
		}
		return nullptr;
	}
}
