#pragma once

#include "Util.h"
#include "WinstonTypes.h"
#include "Rail.h"
#include "SignalBox.h"

namespace winston
{
	class DigitalCentralStation : public Shared_Ptr<DigitalCentralStation>
	{
	public:
		class AddressTranslator : public Shared_Ptr<AddressTranslator>
		{
		public:
			AddressTranslator();
			virtual Turnout::Shared turnout(const unsigned int address) = 0;
		};

		class DebugInjector : public Shared_Ptr<DebugInjector>
		{
		public:
			DebugInjector(DigitalCentralStation::Shared& station);
			void injectTurnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction);
		private:
			DigitalCentralStation::Shared station;
		};

		using TurnoutUpdateCallback = std::function<void(Turnout::Shared turnout, const Turnout::Direction direction)>;
		
		struct Callbacks
		{
			TurnoutUpdateCallback turnoutUpdateCallback;
		};

		DigitalCentralStation(AddressTranslator::Shared& addressTranslator, SignalBox::Shared& signalBox, const Callbacks callbacks);
		virtual ~DigitalCentralStation() = default;

		void turnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction);

		//void setTurnoutUpdateCallback(TurnoutUpdateCallback callback);

	protected:
		AddressTranslator::Shared addressTranslator;
		SignalBox::Shared signalBox;
		//_RailwayP railway;
		const Callbacks callbacks;
	};
}

