
#include "Command.h"
#include "HAL.h"

namespace winston
{
	Command::Command(Payload payload, const std::string name)
		: payload(payload), created(hal::now()), skip(false)
#ifdef WINSTON_STATISTICS
		, _name(name)
#endif
	{
#ifndef WINSTON_STATISTICS
		(void)name;
#endif

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

	const Duration Command::age() const
	{
		return hal::now() - this->created;
	}

#ifdef WINSTON_STATISTICS
	const std::string& Command::name() const
	{
		return this->_name;
	}
#endif
}