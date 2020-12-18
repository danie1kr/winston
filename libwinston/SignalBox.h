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

		SignalBox(Mutex& mutex);

		Railway::Callbacks::TurnoutUpdateCallback injectTurnoutSignalHandling(Railway::Callbacks::TurnoutUpdateCallback callback);

		void setSignalsFor(Turnout::Shared turnout);

		void order(Command::Shared command);
		void work();
	private:

		static Signal::Shared nextSignal(Section::Shared& section, Section::Connection& connection, const bool main);

		std::queue<Command::Shared> commands;
		Mutex& mutex;
	};
}