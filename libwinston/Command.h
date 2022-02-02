#pragma once

#include "WinstonConfig.h"
#include "WinstonTypes.h"
#include "Util.h"

namespace winston
{
	class Command : public Shared_Ptr<Command>
	{
	public:
		using Payload = std::function<const State(const TimePoint& created)>;

		Command(Payload payload, const std::string name = "??");
		virtual ~Command() = default;

		const State execute();
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		const std::string& name() const;
#endif
		void obsolete() noexcept;
		inline const Duration age() const;
	private:
		Payload payload;
		const TimePoint created;
		bool skip;
#if defined(WINSTON_STATISTICS) && defined(WINSTON_STATISTICS_DETAILLED)
		const std::string _name;
#endif
	};
}