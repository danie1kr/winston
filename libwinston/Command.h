#pragma once

#include "WinstonConfig.h"
#include "WinstonTypes.h"
#include "Util.h"

namespace winston
{
	class BasicCommand : public Shared_Ptr<BasicCommand>
	{
	public:
		BasicCommand(const std::string name = "??");
		virtual ~BasicCommand() = default;

		virtual const State execute() = 0;
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		const std::string& name() const;
#endif
		inline const Duration age() const;

	protected:
		const TimePoint created;

	private:
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		const std::string _name;
#endif
	};

	class Command : public BasicCommand, public Shared_Ptr<Command>
	{
	public:
		using Payload = std::function<const State(const TimePoint& created)>;

		Command(Payload payload, const std::string name = "??");
		virtual ~Command() = default;

		const State execute();

		using Shared_Ptr<Command>::Shared;
		using Shared_Ptr<Command>::make;
	private:
		Payload payload;
	};
}