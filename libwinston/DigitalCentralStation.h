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
			virtual const unsigned int address(winston::Section::Shared section) = 0;
		};

		class DebugInjector : public Shared_Ptr<DebugInjector>
		{
		public:
			DebugInjector(DigitalCentralStation::Shared& station);
			void injectTurnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction);
		private:
			DigitalCentralStation::Shared station;
		};

		struct Callbacks
		{
			using TurnoutUpdateCallback = std::function<void(Turnout::Shared turnout, const Turnout::Direction direction)>;
			TurnoutUpdateCallback turnoutUpdateCallback;

			using SystemInfoCallback = std::function<void(const size_t id, const std::string name, const std::string content)>;
			SystemInfoCallback systemInfoCallback;

			using TrackPowerStatusCallback = std::function<void(const bool powerOn)>;
			TrackPowerStatusCallback trackPowerStatusCallback;

			using ProgrammingTrackStatusCallback = std::function<void(const bool programmingOn)>;
			ProgrammingTrackStatusCallback programmingTrackStatusCallback;

			using ShortCircuitDetectedCallback = std::function<void()>;
			ShortCircuitDetectedCallback shortCircuitDetectedCallback;

			using SendMessageCallback = std::function<bool(const std::string& ip, const unsigned short& port, std::vector<unsigned char>& data)>;
			SendMessageCallback sendMessageCallback;
		};

		DigitalCentralStation(AddressTranslator::Shared& addressTranslator, SignalBox::Shared& signalBox, const Callbacks callbacks);
		virtual ~DigitalCentralStation() = default;

		virtual void connect() = 0;

		virtual void requestTurnoutInfo(Turnout::Shared turnout) = 0;
		void turnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction);

		//void setTurnoutUpdateCallback(TurnoutUpdateCallback callback);

	protected:
		AddressTranslator::Shared addressTranslator;
		SignalBox::Shared signalBox;
		//_RailwayP railway;
		const Callbacks callbacks;
	};
}

