#include <typeinfo>
#include <typeindex>

#include "SignalBox.h"
#include "HAL.h"
#include "Tasks.h"
#include "Railway.h"
#include "Events.h"

namespace winston
{
	SignalBox::SignalBox(Railway::Shared& railway, Mutex& mutex)
		: railway(railway), mutex(mutex)
	{
	}
	/*
	SignalBoxP& SignalBox::create(RailwayP& railway, Mutex& mutex)
	{
		signalBox = std::make_shared<SignalBox>(railway, mutex);
		return signalBox;
	}*/

	void SignalBox::order(Command::Shared command)
	{
		this->commands.push(std::move(command));
	}

	/*
	void SignalBox::notify(Event::Unique event)
	{
		this->events.push(std::move(event));
	}

	void SignalBox::assign(Task::Unique task)
	{
		this->tasks.push(std::move(task));
	}*/

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
