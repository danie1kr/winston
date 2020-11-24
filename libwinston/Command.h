#pragma once

#include "WinstonTypes.h"
#include "Util.h"

namespace winston
{
	class Command : public Shared_Ptr<Command>
	{
	public:
		using Payload = std::function<const State(const unsigned long& created)>;

		Command(Payload payload);
		virtual ~Command() = default;

		const State execute();
		inline const unsigned long age();
	private:
		unsigned long created;
		Payload payload;
	};
}