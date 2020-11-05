#pragma once

#include <memory>
#include <queue>
#include "Events.h"
#include "Util.h"
#include "Signal.h"
#include "WinstonTypes.h"
#include "Railway.h"

namespace winston
{
	class SignalBox : public Shared_Ptr<SignalBox>
	{
	public:

		enum class Callback
		{
			TurnoutToggled,
			Count
		};

		SignalBox(Railway::Shared& railway, Mutex& mutex);
		//static SignalBoxP& create(RailwayP& railway, Mutex& mutex);
		void notify(Event::Unique event);
		void assign(Task::Unique task);
		void work();
	private:
		std::queue<Event::Unique> events;
		std::queue<Task::Unique> tasks;

		Mutex& mutex;
		Railway::Shared railway;

		void work(EventTurnoutStartToggle::Unique event);
		void work(EventTurnoutFinalizeToggle::Unique event);
	};
	//extern SignalBoxP signalBox;
}