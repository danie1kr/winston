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

	void SignalBox::setSignalOn(Track::Shared track, const bool guarding, const Track::Connection connection, const Signal::Aspect aspect, const bool includingFirst)
	{
		const auto preSignalAspect = aspect == Signal::Aspect::Go ? Signal::Aspect::ExpectGo : Signal::Aspect::ExpectHalt;
		
		auto current = track;
		auto from = connection;
		// current and from are now the position of mainSignal
		if (Signal::Shared mainSignal = SignalBox::nextSignal(current, guarding, from, true, includingFirst))
		{
			mainSignal->aspect(aspect);
			// current and from are now the position of mainSignal
			auto otherFrom = current->otherConnection(from);
			if (Signal::Shared preSignal = SignalBox::nextSignal(current, guarding, otherFrom, false, false))
				preSignal->aspect(preSignalAspect);
		}
	}

	void SignalBox::setSignalsFor(Turnout::Shared turnout)
	{
		// make public
		auto setSignals = [](Turnout::Shared turnout, const Turnout::Direction direction)
		{
			Track::Connection from = direction == Turnout::Direction::A_B ? Track::Connection::B : Track::Connection::C;
			Track::Shared current = turnout;
			const auto mainSignalAspect = turnout->direction() == direction ? Signal::Aspect::Go : Signal::Aspect::Halt;
			SignalBox::setSignalOn(current, true, from, mainSignalAspect, true);
		};

		this->order(Command::make([turnout, setSignals](const unsigned long& created) -> const winston::State { setSignals(turnout, turnout->direction()); return State::Finished; }));
		this->order(Command::make([turnout, setSignals](const unsigned long& created) -> const winston::State { setSignals(turnout, turnout->otherDirection(turnout->direction())); return State::Finished;  }));
	}
	
	Signal::Shared SignalBox::nextSignal(Track::Shared& track, const bool guarding, Track::Connection& leaving, const bool main, const bool includingFirst)
	{
		Track::Connection connection = leaving;
		Track::Connection checkConnection = connection;
		Track::Shared onto;
		Track::Shared& current = track;

		std::unordered_set<Track::Shared> visited;

		bool done = false;
		bool skipTraverse = includingFirst;
		while (!done)
		{
			if (!skipTraverse)
			{
				if (!current->traverse(connection, onto, true))
					break;

				Track::Connection backConnection = onto->whereConnects(current);
				connection = onto->otherConnection(backConnection);
				checkConnection = guarding ? backConnection : connection;

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

			if (onto->type() != Track::Type::Turnout)
			{
				auto signal = guarding ? onto->signalGuarding(checkConnection) : onto->signalFacing(checkConnection);
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
				break;
		}

		return nullptr;
	}


	void SignalBox::order(Command::Shared command)
	{
		this->commands.push(std::move(command));
	}

	void SignalBox::work()
	{
		if (this->mutex.lock())
		{
			if (this->commands.size() > 0)
			{
				auto command = std::move(this->commands.front());
				this->commands.pop();
				this->mutex.unlock();

				//auto payload = command->payload();
				if (command->execute() == State::Running)
				{
					while (!this->mutex.lock());
					this->commands.push(std::move(command));
					this->mutex.unlock();
				}
				//else
					//command->finished();
			}
		}

		/*
		if (this->mutex.lock())
		{
			if (this->tasks.size() > 0)
			{
				auto task = std::move(this->tasks.front());
				this->tasks.pop();
				this->mutex.unlock();
				
				auto payload = task->payload();
				if (payload->execute(this->shared_from_this()) == State::Running)
				{
					while (!this->mutex.lock());
					this->tasks.push(std::move(task));
					this->mutex.unlock();
				}
				else
					task->finished();
			}
			else
				this->mutex.unlock();
		}

		if (this->mutex.lock())
		{
			if (this->events.size() > 0)
			{
				auto event = std::move(this->events.front());
				this->events.pop();
				this->mutex.unlock();

				auto payload = event->payload();
				payload->evaluate(this->shared_from_this());
				auto task = Task::make(payload, std::move(event));
				this->assign(std::move(task));/*

				if (auto specificEvent = dynamic_unique_ptr_cast<Event, EventTurnoutStartToggle>(event))
				{
					this->work(std::move(specificEvent));
				}
				else if (auto specificEvent = dynamic_unique_ptr_cast<Event, EventTurnoutFinalizeToggle>(event))
				{
					this->work(std::move(specificEvent));
				}*
			}
			else
				this->mutex.unlock();
		}
*/
	}

	/*void SignalBox::work(EventTurnoutStartToggle::Unique event)
	{
		auto turnout = std::dynamic_pointer_cast<Turnout>(event->turnout());
		auto task = TaskTurnoutStartToggle::make(std::move(event), turnout);
		this->assign(std::move(task));
	}

	void SignalBox::work(EventTurnoutFinalizeToggle::Unique event)
	{
		auto turnout = std::dynamic_pointer_cast<Turnout>(event->turnout());
		auto direction = event->direction();
		auto task = TaskTurnoutFinalizeToggle::make(std::move(event), turnout, direction);
		this->assign(std::move(task));
	}*/
}
