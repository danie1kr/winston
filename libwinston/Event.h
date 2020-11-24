#pragma once

#include <functional>
#include "WinstonTypes.h"
#include "Callback.h"
/*
namespace winston
{
	class Payload : public Shared_Ptr<Payload>
	{
	public:
		using EventCallback = std::function<const State(SignalBox::Shared& signalBox)>;
		using TaskCallback = std::function<const State(SignalBox::Shared& signalBox)>;

		Payload(EventCallback evaluate, TaskCallback execute);

		inline const State evaluate(SignalBox::Shared& signalBox);
		inline const State execute(SignalBox::Shared& signalBox);

	private:
		EventCallback eventCallback;
		TaskCallback taskCallback;
	};

	class Event : public Uniqe_Ptr<Event>
	{
	public:
		Event(Payload::Shared payload, Callback::Shared taskFinishedCallback);
		virtual ~Event() = default;

		Payload::Shared payload();
		void finished();
	private:
		Callback::Shared callback;
		Payload::Shared _payload;
	};
}*/