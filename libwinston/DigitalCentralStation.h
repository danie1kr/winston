#pragma once

#include "Util.h"
#include "WinstonTypes.h"
#include "Track.h"
#include "Locomotive.h"
#include "SignalTower.h"

namespace winston
{
	class DigitalCentralStation : public Shared_Ptr<DigitalCentralStation>
	{
	public:
		class TurnoutAddressTranslator : public Shared_Ptr<TurnoutAddressTranslator>
		{
		public:
			virtual Track::Shared turnout(const Address address) const = 0;
			virtual const Address address(winston::Track& track) const = 0;

			virtual Route::Shared route(const Address address) const = 0;
			virtual const Address address(winston::Route::Shared track) const = 0;
		};

		class LocoAddressTranslator
		{
		public:
			virtual Locomotive::Shared locoFromAddress(const Address address) = 0;
			virtual const Address addressOfLoco(const Locomotive& loco) const = 0;
		};

		struct Callbacks : public Railway::Callbacks
		{
			using LocomotiveUpdateCallback = std::function<void(Locomotive& loco,
				bool  busy,
			//	boolean  doubleTracktion,
			//	boolean  transpond,
				bool  forward,
				unsigned char  speed,                                  // In 128 speed range
				uint32_t functions)>;
			LocomotiveUpdateCallback locomotiveUpdateCallback = [](Locomotive& loco,
				bool  busy,
				//	boolean  doubleTracktion,
				//	boolean  transpond,
				bool  forward,
				unsigned char  speed,                                  // In 128 speed range
				uint32_t functions) { return; };

			using SystemInfoCallback = std::function<void(const size_t id, const std::string name, const std::string content)>;
			SystemInfoCallback systemInfoCallback = [=](const size_t id, const std::string name, const std::string content) {
				winston::logger.log(winston::build("Z21: ", name, ": ", content));
			};

			using TrackPowerStatusCallback = std::function<void(const bool powerOn)>;
			TrackPowerStatusCallback trackPowerStatusCallback = [=](const bool powerOn) {
				winston::logger.log(std::string("Z21: Power is ") + std::string(powerOn ? "on" : "off"));
			};

			using ProgrammingTrackStatusCallback = std::function<void(const bool programmingOn)>;
			ProgrammingTrackStatusCallback programmingTrackStatusCallback = [=](const bool programmingOn) {
				winston::logger.log(std::string("Z21: Programming is ") + std::string(programmingOn ? "on" : "off"));
			};

			using ShortCircuitDetectedCallback = std::function<void()>;
			ShortCircuitDetectedCallback shortCircuitDetectedCallback = [=]() {
				winston::logger.log("Z21: Short circuit detected!");
			};

			using ConnectedCallback = std::function<void()>;
			ConnectedCallback connectedCallback = [](){
				return;
			};

			// true: continue processing this address, false skip
			using SpecialAccessoryProcessingCallback = std::function<const bool(const uint16_t address, const uint8_t state)>;
			SpecialAccessoryProcessingCallback specialAccessoryProcessingCallback = [](const uint16_t address, const uint8_t state) -> const bool { return false; };
		};

		class DebugInjector : public Shared_Ptr<DebugInjector>
		{
		public:
			DebugInjector(DigitalCentralStation::Shared& station);
			void injectTurnoutUpdate(Turnout& turnout, const Turnout::Direction direction);
			void injectDoubleSlipTurnoutUpdate(DoubleSlipTurnout& turnout, const DoubleSlipTurnout::Direction direction);
			void injectLocoUpdate(Locomotive& loco, bool busy, bool forward, unsigned char speed, uint32_t functions);
			void injectConnected();
		private:
			DigitalCentralStation::Shared station;
		};

		DigitalCentralStation(TurnoutAddressTranslator::Shared& addressTranslator, LocoAddressTranslator &locoAddressTranslator, SignalTower::Shared& signalTower, const Callbacks callbacks);
		virtual ~DigitalCentralStation() = default;

		virtual const winston::Result connect() = 0;
		virtual const winston::Result tick() = 0;

		virtual void requestTurnoutInfo(Turnout& turnout) = 0;
		virtual void requestDoubleSlipTurnoutInfo(DoubleSlipTurnout& turnout) = 0;
		virtual void requestLocoInfo(const Locomotive& loco) = 0;
		virtual void triggerTurnoutChangeTo(winston::Turnout& turnout, winston::Turnout::Direction direction) = 0;
		virtual void triggerDoubleSlipTurnoutChangeTo(winston::DoubleSlipTurnout& turnout, winston::DoubleSlipTurnout::Direction direction) = 0;
		virtual void triggerLocoDrive(const Address address, const unsigned char speed, const bool forward) = 0;
		virtual void triggerLocoFunction(const Address address, const uint32_t functions) = 0;
		virtual void keepAlive();

		virtual bool isEmergencyStop() const = 0;
		virtual const winston::Result requestEmergencyStop(const bool emergencyStop) = 0;

		void turnoutUpdate(Turnout&, const Turnout::Direction direction);
		void doubleSlipUpdate(DoubleSlipTurnout& turnout, const DoubleSlipTurnout::Direction direction);

		const bool connected() const;

	protected:

		const Result onConnected();
		virtual const Result connectedInternal() = 0;

		TurnoutAddressTranslator::Shared turnoutAddressTranslator;
		LocoAddressTranslator& locoAddressTranslator;
		SignalTower::Shared signalTower;
		const Callbacks callbacks;
		bool emergencyStop;

		bool connectedToDCS;
	};
}

