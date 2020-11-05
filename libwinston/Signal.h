#pragma once

#include "WinstonTypes.h"
#include "Task.h"
#include <functional>

namespace winston
{
	class Signal
	{
	public:
		enum class Aspect
		{
			Stop,
			Go,
			Slow,
		};
		//Device& device, const std::vector<Port> ports, 
		using Callback = std::function<Task::State(Aspect aspect)>;
		Signal(Callback callback);

		Task::State set(Aspect aspect);
		Aspect state();
		/*
		virtual void stop();
		virtual void slow();
		virtual void go();*/
	protected:
		Aspect aspect;
		Callback callback;
	};
	/*
	class SectionSignal;
	class PreSignal;

	class PreSignal : public Signal
	{
	public:
		PreSignal(Device& device, const Port stop, const Port go);

	};

	class SectionSignal : public Signal
	{
	public:
		SectionSignal(Device& device, const Port stop, const Port go);
	};

	class SectionWithPreSignal : public SectionSignal
	{
	public:
		SectionWithPreSignal(Device& device, const Port stop, const Port go, PreSignal& preSignal);
		void stop();
		void go();
	private:
		PreSignal& preSignal;
	};

	class BlockSignal : public Signal
	{

	};

	class EntrySignal : public Signal
	{

	};

	class ExitSignal : public Signal
	{

	};*/
}