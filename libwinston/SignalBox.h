#pragma once

#include <memory>
#include <queue>
#include "Command.h"
#include "Util.h"
#include "Signal.h"
#include "WinstonTypes.h"
#include "Railway.h"

namespace winston
{
	class SignalBox : public Shared_Ptr<SignalBox>, std::enable_shared_from_this<SignalBox>
	{
	public:

		enum class Callback
		{
			TurnoutToggled,
			Count
		};

		SignalBox(Railway::Shared& railway, Mutex& mutex);
		//static SignalBoxP& create(RailwayP& railway, Mutex& mutex);
		//void notify(Event::Unique event);
		//void assign(Task::Unique task);
		void order(Command::Shared command);
		void work();
	private:
		std::queue<Command::Shared> commands;
		/*
		std::queue<Event::Unique> events;
		std::queue<Task::Unique> tasks;
		*/
		Mutex& mutex;
		Railway::Shared railway;
	};
	//extern SignalBoxP signalBox;
}