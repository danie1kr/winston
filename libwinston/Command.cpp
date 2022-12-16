
#include "Command.h"
#include "HAL.h"

namespace winston
{
	BasicCommand::BasicCommand(const std::string name)
		: Shared_Ptr<BasicCommand>(), created(hal::now())
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		, _name(name)
	{
#else
	{
		(void)name;
#endif
	}

	const Duration BasicCommand::age() const
	{
		return hal::now() - this->created;
	}

#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
	const std::string& BasicCommand::name() const
	{
		return this->_name;
	}
#endif

	Command::Command(Payload payload, const std::string name)
		: BasicCommand(name), payload(payload)

	{

	}

	const State Command::execute()
	{
		return this->payload(this->created);
	}
}