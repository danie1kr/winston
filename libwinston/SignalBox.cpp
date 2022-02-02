#include <unordered_set>

#include "Winston.h"
#include "SignalBox.h"
#include "HAL.h"
#include "Railway.h"

namespace winston
{
	SignalBox::SignalBox()
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		: stopwatchJournal("SignalBox")
#endif
	{
	}

	Railway::Callbacks::TurnoutUpdateCallback SignalBox::injectTurnoutSignalHandling(Railway::Callbacks::TurnoutUpdateCallback callback)
	{
		return [this, callback](Turnout::Shared turnout, Turnout::Direction direction) -> State {
			State state = callback(turnout, direction);
			if (state == State::Finished)
				this->setSignalsFor(turnout);
			return state;
		};
	}

	void SignalBox::initSignalsForTurnouts(std::set<Turnout::Shared> turnouts)
	{
		for (auto &turnout: turnouts)
			this->setSignalsFor(turnout);
	}

	void SignalBox::setSignalOn(Track::Shared track, const Track::Connection signalGuardedConnection, const Signal::Aspect aspect)
	{
		const auto preSignalAspect = aspect == Signal::Aspect::Go ? Signal::Aspect::ExpectGo : Signal::Aspect::ExpectHalt;

		auto current = track;
		auto connection = signalGuardedConnection;
		// current and from are now the position of mainSignal
		if (Signal::Shared mainSignal = track->signalGuarding(connection))
		{
			mainSignal->aspect(aspect);
			auto otherConnection = current->otherConnection(connection);
			Signal::Shared preSignal;
			/*auto result = */ Track::traverse<Track::TraversalSignalHandling::OppositeDirection>(current, otherConnection, preSignal);
			if (preSignal)
				preSignal->aspect(preSignalAspect);
		}
	}

	void SignalBox::setSignalsFor(Turnout::Shared turnout, const Turnout::Direction direction)
	{
		// very simple strategy setting the signal only according to turnout direction

		// better: get aspect from situation later in path: red for open turnout, green otherwise
		// see below
		Track::Shared signalCurrent = turnout;
		Track::Connection signalConnection = direction == Turnout::Direction::A_B ? Track::Connection::B : Track::Connection::C;
		auto signalToSet = this->nextSignal(signalCurrent, true, signalConnection, true, true);

		if (signalToSet)
		{
			auto aspect = Signal::Aspect::Halt;
			if (turnout->direction() == direction)
			{
				Track::Shared current = turnout;
				auto connection = Track::Connection::A;
				Signal::Shared signal;
				auto result = Track::traverse<Track::TraversalSignalHandling::ForwardDirection>(current, connection, signal);

				switch (result)
				{
				case Track::TraversalResult::Bumper:
				case Track::TraversalResult::Looped:
				case Track::TraversalResult::Signal: aspect = Signal::Aspect::Go; break;
				default:
				case Track::TraversalResult::OpenTurnout: aspect = Signal::Aspect::Halt; break;
				}
			}// else halt == default

			this->setSignalOn(signalCurrent, signalConnection, aspect);
		}
	}

	void SignalBox::setSignalsFor(Turnout::Shared turnout)
	{
		// A_facing = leave turnout at A, find first main signal facing A
		// B_guarding = leave turnout at B, find first pre signal if 

		// the direction
		this->order(Command::make([this, turnout](const TimePoint &created) -> const winston::State { this->setSignalsFor(turnout, turnout->direction()); return State::Finished; }, __PRETTY_FUNCTION__));
		// the closed direction
		this->order(Command::make([this, turnout](const TimePoint &created) -> const winston::State { this->setSignalsFor(turnout, turnout->otherDirection(turnout->direction())); return State::Finished; }, __PRETTY_FUNCTION__));
		// backwards on entry
		this->order(Command::make([this, turnout](const TimePoint &created) -> const winston::State { 


			Track::Shared signalCurrent = turnout;
			auto signalConnection = Track::Connection::A;
			auto signalToSet = this->nextSignal(signalCurrent, true, signalConnection, true, true);

			Track::Shared current = turnout;
			auto connection = turnout->fromDirection();
			Signal::Shared signal;
			auto result = Track::traverse<Track::TraversalSignalHandling::ForwardDirection>(current, connection, signal);

			Signal::Aspect aspect;
			switch (result)
			{
			case Track::TraversalResult::Bumper: 
			case Track::TraversalResult::Looped: 
			case Track::TraversalResult::Signal: aspect = Signal::Aspect::Go; break;
			default:
			case Track::TraversalResult::OpenTurnout: aspect = Signal::Aspect::Halt; break;
			}
			this->setSignalOn(signalCurrent, signalConnection, aspect);

			return State::Finished; 

		}, __PRETTY_FUNCTION__));
	}
	
	Signal::Shared SignalBox::nextSignal(Track::Shared& track, const bool guarding, Track::Connection& leaving, const bool main, const bool includingFirst)
	{
		Track::Connection connection = leaving;
		Track::Shared onto = track;
		Signal::Shared signal;
		Track::TraversalResult result;

		if (connection == Track::Connection::DeadEnd)
			return nullptr;

		if (!includingFirst)
		{
			if(!track->traverse(connection, onto, true))
				return nullptr;
			connection = onto->otherConnection(onto->whereConnects(track));
		}

		if (guarding)
		{
			result = Track::traverse<Track::TraversalSignalHandling::OppositeDirection>(onto, connection, signal);
		}
		else 
		{
			result = Track::traverse<Track::TraversalSignalHandling::ForwardDirection>(onto, connection, signal);
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

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	const std::string SignalBox::statistics(const size_t withTop) const
	{
		return this->stopwatchJournal.toString(withTop);
	}
#endif

	void SignalBox::order(Command::Shared command)
	{
		this->commands.push(std::move(command));
	}

	bool SignalBox::work()
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
}
