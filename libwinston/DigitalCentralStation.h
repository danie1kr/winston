#pragma once

#include "Util.h"
#include "WinstonTypes.h"
#include "Track.h"
#include "Locomotive.h"
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
			virtual Turnout::Shared turnout(const Address address) = 0;
			virtual Locomotive::Shared locomotive(const Address address) = 0;
			virtual const Address address(winston::Track::Shared track) = 0;
		};

		struct Callbacks : public Railway::Callbacks
		{
			using LocomotiveUpdateCallback = std::function<void(Locomotive::Shared loco,
				bool  busy,
			//	boolean  doubleTracktion,
			//	boolean  transpond,
				bool  forward,
				unsigned char  speed,                                  // In 128 speed range
				uint32_t functions)>;
			LocomotiveUpdateCallback locomotiveUpdateCallback;

			using SystemInfoCallback = std::function<void(const size_t id, const std::string name, const std::string content)>;
			SystemInfoCallback systemInfoCallback;

			using TrackPowerStatusCallback = std::function<void(const bool powerOn)>;
			TrackPowerStatusCallback trackPowerStatusCallback;

			using ProgrammingTrackStatusCallback = std::function<void(const bool programmingOn)>;
			ProgrammingTrackStatusCallback programmingTrackStatusCallback;

			using ShortCircuitDetectedCallback = std::function<void()>;
			ShortCircuitDetectedCallback shortCircuitDetectedCallback;
		};

		class DebugInjector : public Shared_Ptr<DebugInjector>
		{
		public:
			DebugInjector(DigitalCentralStation::Shared& station);
			void injectTurnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction);
			void injectLocoUpdate(Locomotive::Shared loco, bool busy, bool forward, unsigned char speed, uint32_t functions);
		private:
			DigitalCentralStation::Shared station;
		};

		DigitalCentralStation(AddressTranslator::Shared& addressTranslator, SignalBox::Shared& signalBox, const Callbacks callbacks);
		virtual ~DigitalCentralStation() = default;

		virtual const winston::Result connect() = 0;
		virtual const winston::Result tick() = 0;

		virtual void requestTurnoutInfo(winston::Turnout::Shared turnout) = 0;
		virtual void triggerTurnoutChangeTo(winston::Turnout::Shared turnout, winston::Turnout::Direction direction) = 0;
		virtual void triggerLocoDrive(const Address address, const unsigned char speed, const bool forward) = 0;
		virtual void triggerLocoFunction(const Address address, const uint32_t functions) = 0;


		void turnoutUpdate(Turnout::Shared turnout, const Turnout::Direction direction);

	protected:
		AddressTranslator::Shared addressTranslator;
		SignalBox::Shared signalBox;
		const Callbacks callbacks;
	};
}

