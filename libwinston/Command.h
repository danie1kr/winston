#pragma once

#include <chrono>
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
#ifdef WINSTON_STATISTICS
		const std::string& name() const;
#endif
		void obsolete() noexcept;
		inline const Duration age() const;
	private:
		Payload payload;
		const TimePoint created;
		bool skip;
#ifdef WINSTON_STATISTICS
		const std::string _name;
#endif
	};
}