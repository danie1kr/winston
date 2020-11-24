#pragma once

#include "WinstonTypes.h"
#include "Util.h"

namespace winston
{
	/*class Payload : public Shared_Ptr<Payload>
	{
	public:

		Payload(Callback execute);

		inline const State execute(SignalBox::Shared& signalBox);

	private:
		Callback callback;
	};*/

	class Command : public Shared_Ptr<Command>
	{
	public:
		//using Shared_Ptr<Command>::Shared;
		//using Shared_Ptr<Command>::make;
		using Payload = std::function<const State(const unsigned long& created)>;

		Command(Payload payload);
		virtual ~Command() = default;

		const State execute();
		//void finished();
		inline const unsigned long age();
	private:
		unsigned long created;
		Payload payload;
		//Callback::Shared chainFinishedCallback;
		//Payload::Shared _payload;
	};
}