#include <typeinfo>
#include <typeindex>

#include "SignalBox.h"
#include "HAL.h"
#include "Railway.h"

namespace winston
{
	SignalBox::SignalBox(Railway::Shared& railway, Mutex& mutex)
		: railway(railway), mutex(mutex)
	{
	}

	void SignalBox::setSignalsFor(Turnout::Shared& turnout, const Turnout::Direction& direction)
	{
		Section::Connection from = direction == Turnout::Direction::A_B ? Section::Connection::B : Section::Connection::C;
		Section::Shared onto;
		turnout->traverse(from, onto);
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
