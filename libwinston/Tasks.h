#pragma once

#include "WinstonTypes.h"
#include "Task.h"
#include "Signal.h"
#include "Rail.h"

namespace winston
{
	/*class TaskSignal : public Task
	{
	public:
		TaskSignal(Signal& signal, Signal::Aspect aspect);
		void execute();
	private:
		Signal& signal;
		Signal::Aspect aspect;
	};

	class TaskDrive : public Task, Uniqe_Ptr<Task>
	{
	public:
		TaskDrive();
		const State execute();
	};*/

	class TaskTurnoutStartToggle : public Task, Uniqe_Ptr<TaskTurnoutStartToggle>
	{
	public:
		TaskTurnoutStartToggle(Event::Unique event, Turnout::Shared& turnout);
		const State execute();

		using Uniqe_Ptr<TaskTurnoutStartToggle>::Unique;
		using Uniqe_Ptr<TaskTurnoutStartToggle>::make;

	private:
		Turnout::Shared turnout;
	};

	class TaskTurnoutFinalizeToggle : public Task, Uniqe_Ptr<TaskTurnoutFinalizeToggle>
	{
	public:
		TaskTurnoutFinalizeToggle(Event::Unique event, Turnout::Shared& turnout, const Turnout::Direction direction);
		const State execute();

		using Uniqe_Ptr<TaskTurnoutFinalizeToggle>::Unique;
		using Uniqe_Ptr<TaskTurnoutFinalizeToggle>::make;
	private:
		Turnout::Shared turnout;
		Turnout::Direction direction;
	};
}