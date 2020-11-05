
#include "Event.h"

namespace winston
{
	Event::Event(Callback::Shared taskFinishedCallback)
		: Uniqe_Ptr<Event>(), callback(taskFinishedCallback->use())
	{
	}

	void Event::finished()
	{
		this->callback->execute();
	}
}