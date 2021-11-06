#include <typeinfo>
#include <typeindex>
#include <unordered_set>

#include "SignalBox.h"
#include "HAL.h"
#include "Railway.h"

namespace winston
{
	SignalBox::SignalBox(Mutex& mutex)
		: mutex(mutex)
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

	void SignalBox::setSignalOn(Track::Shared track, const bool guarding, const Track::Connection connection, const Signal::Aspect aspect, const bool includingFirst)
	{
		const auto preSignalAspect = aspect == Signal::Aspect::Go ? Signal::Aspect::ExpectGo : Signal::Aspect::ExpectHalt;
		
		auto current = track;
		auto from = connection;
		// current and from are now the position of mainSignal
		if (Signal::Shared mainSignal = SignalBox::nextSignal(current, guarding, from, true, includingFirst))
		{
			mainSignal->aspect(aspect);
			/*if (aspect == Signal::Aspect::Go && mainSignal->preSignal())
			{
				auto preCurrent = current;
				auto preFrom = from;
				if(Signal::Shared preOfMainSignal = SignalBox::nextSignal(preCurrent, guarding, preFrom, true, false))
					mainSignal->aspect(preOfMainSignal->shows(Signal::Aspect::Go) ? Signal::Aspect::ExpectGo : Signal::Aspect::ExpectHalt);
			}*/
			// current and from are now the position of mainSignal
			auto otherFrom = current->otherConnection(from);
			if (Signal::Shared preSignal = SignalBox::nextSignal(current, false, otherFrom, false, false))
				preSignal->aspect(preSignalAspect);
		}
		/*else if (current->type() == Track::Type::Bumper)
		{
			mainSignal->aspect(Signal::Aspect::Go);
			// current and from are now the position of mainSignal
			auto otherFrom = current->otherConnection(from);
			if (Signal::Shared preSignal = SignalBox::nextSignal(current, false, otherFrom, false, false))
				preSignal->aspect(Signal::Aspect::ExpectGo);
		}*/
	}

	void SignalBox::setSignalsFor(Turnout::Shared turnout)
	{
		// make public
		auto setSignals = [](Turnout::Shared turnout, const Turnout::Direction direction)
		{
			Track::Connection from = direction == Turnout::Direction::A_B ? Track::Connection::B : Track::Connection::C;
			Track::Shared current = turnout;
			const auto mainSignalAspect = turnout->direction() == direction ? Signal::Aspect::Go : Signal::Aspect::Halt;
			SignalBox::setSignalOn(current, false, from, mainSignalAspect, false);
		};

		// A_facing = leave turnout at A, find first main signal facing A
		// B_guarding = leave turnout at B, find first pre signal if 

		// the direction
		this->order(Command::make([turnout, setSignals](const unsigned long& created) -> const winston::State { setSignals(turnout, turnout->direction()); return State::Finished; }));
		// the closed direction
		this->order(Command::make([turnout, setSignals](const unsigned long& created) -> const winston::State { setSignals(turnout, turnout->otherDirection(turnout->direction())); return State::Finished; }));
		// backwards on entry
		this->order(Command::make([this, turnout](const unsigned long& created) -> const winston::State { 


			Track::Shared signalCurrent = turnout;
			auto signalConnection = Track::Connection::A;
			auto signalToSet = this->nextSignal(signalCurrent, false, signalConnection, true, true);

			Track::Shared current = turnout;
			auto connection = Track::Connection::A;
			Signal::Shared signal;
			auto result = Track::traverse<Track::TraversalSignalHandling::OppositeDirection>(current, connection, signal);

			Signal::Aspect aspect;
			switch (result)
			{
			case Track::TraversalResult::Bumper: 
			case Track::TraversalResult::Looped: 
			case Track::TraversalResult::Signal: aspect = Signal::Aspect::Go; break;
			case Track::TraversalResult::OpenTurnout: aspect = Signal::Aspect::Halt; break;
			}
			this->setSignalOn(signalCurrent, false, signalConnection, aspect, false);

			/*
			Track::Shared signalCurrent = turnout;
			Track::Shared onto;
			auto signalConnection = Track::Connection::A;
			auto signal = this->nextSignal(signalCurrent, false, signalConnection, true, true);
			auto canTraverse = true;
			auto connection = signalConnection;
			auto current = signalCurrent;
			std::unordered_set<Track::Shared> visited;
			bool looped = false;
			bool forward = false;
			Signal::Aspect aspect = Signal::Aspect::Halt;
			while (canTraverse = current->traverse(connection, onto, forward))
			{
				if (visited.contains(onto))
				{
					aspect = Signal::Aspect::Go;
					looped = true;
					break;
				}
				// it should be sufficient to search for the next signal instead of trying to loop
				forward = true;
				connection = onto->otherConnection(onto->whereConnects(current));
				visited.insert(onto);
				current = onto;
				if (onto->type() == Track::Type::Bumper && connection == Track::Connection::DeadEnd)
				{
					aspect = Signal::Aspect::Go;
					break;
				}
			}
			//auto aspect = (looped || (onto && onto->type() == Track::Type::Bumper)) ? Signal::Aspect::Go : Signal::Aspect::Halt;
			this->setSignalOn(signalCurrent, false, signalConnection, aspect, false);
			*/
			return State::Finished; 

		}));
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

		/*
		Track::Connection connection = leaving;
		//Track::Connection checkConnection = connection;
		Track::Shared onto;
		Track::Shared& current = track;

		std::unordered_set<Track::Shared> visited;

		Signal::Shared lastSignal = nullptr;

		bool done = false;
		bool skipTraverse = includingFirst;
		while (!done)
		{
			if (!skipTraverse)
			{
				if (!current->traverse(connection, onto, true))
					break;

				connection = onto->otherConnection(onto->whereConnects(current));
				//checkConnection = guarding ? backConnection : connection;

				// we looped somehow
				if (visited.contains(onto))
					break;
			}
			else
			{
				onto = current;
				skipTraverse = false;
			}
			visited.insert(onto);

			auto currentSignal = guarding ? current->signalGuarding(connection) : current->signalFacing(connection);
			if (currentSignal)
				lastSignal = currentSignal;

			if (onto->type() != Track::Type::Turnout)
			{
				auto signal = guarding ? onto->signalGuarding(connection) : onto->signalFacing(connection);
				if (signal)
				{
					if ((signal->mainSignal() && main) || (signal->preSignal() && !main))
					{
						track = onto;
						return signal;
					}
					else if (signal->mainSignal() && !main)
						return nullptr;
				}
			}
			
			if (onto->type() == Track::Type::Bumper)
			{
				return nullptr;
				//track = onto;
				//return lastSignal;
			}

			current = onto;
		}

		return nullptr;*/
	}


	void SignalBox::order(Command::Shared command)
	{
		this->commands.push(std::move(command));
	}

	bool SignalBox::work()
	{
		if (this->mutex.lock())
		{
			if (this->commands.size() > 0)
			{
				auto command = std::move(this->commands.front());
				this->commands.pop();
				this->mutex.unlock();

				if (command->execute() == State::Running)
				{
					while (!this->mutex.lock());
					this->commands.push(std::move(command));
					this->mutex.unlock();
				}
			}
			return true;
		}
		return false;
	}
}
