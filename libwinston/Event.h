#pragma once

#include <functional>
#include "WinstonTypes.h"
#include "Callback.h"

namespace winston
{
	class Event : public Uniqe_Ptr<Event>
	{
	public:
		Event(Callback::Shared taskFinishedCallback);
		virtual ~Event() = default;
		void finished();
	private:
		Callback::Shared callback;
	};
}