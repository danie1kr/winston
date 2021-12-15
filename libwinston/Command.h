#pragma once

#include "WinstonTypes.h"
#include "Util.h"

namespace winston
{
	class Command : public Shared_Ptr<Command>
	{
	public:
		using Payload = std::function<const State(const unsigned long long& created)>;

		Command(Payload payload);
		virtual ~Command() = default;

		const State execute();
		void obsolete() noexcept;
		inline const unsigned long long age() const;
	private:
		Payload payload;
		unsigned long long created;
		bool skip;
	};
}