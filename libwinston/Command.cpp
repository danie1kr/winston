
#include "Command.h"
#include "HAL.h"

namespace winston
{
	Command::Command(Payload payload, const std::string name)
		: payload(payload), created(hal::now()), skip(false)
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		, _name(name)
	{
#else
	{
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

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	const std::string& Command::name() const
	{
		return this->_name;
	}
#endif
}