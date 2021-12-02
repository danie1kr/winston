#include "Command.h"
#include "HAL.h"

namespace winston
{
	Command::Command(Payload payload)
		: payload(payload), created(hal::now()), skip(false)
	{

	}

	void Command::obsolete() noexcept
	{
		this->skip = true;
	}

	const State Command::execute()
	{
		if (this->skip)
			return State::Skipped;
		return this->payload(this->created);
	}

	const unsigned long long Command::age() const
	{
		return hal::now() - this->created;
	}
}