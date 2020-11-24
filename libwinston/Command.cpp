#include "Command.h"
#include "HAL.h"

namespace winston
{
	Command::Command(Payload payload)
		: payload(payload), created(hal::now())
	{

	}

	const State Command::execute()
	{
		return this->payload(this->created);
	}

	const unsigned long Command::age()
	{
		return hal::now() - this->created;
	}
}