#pragma once

#ifdef WINSTON_PLATFORM_TEENSY
//#include "pgmspace.h"
#else
//#define FLASHMEM
#endif

#include "HAL.h"
#include "Railway.h"
#include "SignalBox.h"
#include "Log.h"
#include "DigitalCentralStation.h"

namespace winston
{
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation, Features _features = Features::None>
	class ModelRailwaySystem : public DigitalCentralStation::LocoAddressTranslator
	{
	public:
		ModelRailwaySystem()
#ifdef WINSTON_STATISTICS
			: stopWatchJournal("MRS") 
#endif
		{ };
		virtual ~ModelRailwaySystem() { } ;

		void setup() {
			this->systemSetup();

			this->railway->init(_features & Features::Blocks);

			this->digitalCentralStation->connect();

			this->railway->turnouts([=](const Tracks track, winston::Turnout::Shared turnout) {
				
				this->signalBox->order(winston::Command::make([this, turnout](const TimePoint &created) -> const winston::State
					{
						this->digitalCentralStation->requestTurnoutInfo(turnout);
						winston::hal::delay(50);
						this->signalBox->setSignalsFor(turnout);
						winston::hal::delay(100);
						return winston::State::Finished;
					}, __PRETTY_FUNCTION__));
			});

			for(const auto &loco : this->locomotiveShed ) {

				this->signalBox->order(winston::Command::make([this, loco](const TimePoint &created) -> const winston::State
					{
						this->digitalCentralStation->requestLocoInfo(loco);
						winston::hal::delay(150);
						return winston::State::Finished;
					}, __PRETTY_FUNCTION__));
			}

			this->signalBox->order(winston::Command::make([](const TimePoint &created) -> const winston::State
				{
					logger.log("Init tasks complete");
					return winston::State::Finished;
				}, __PRETTY_FUNCTION__));

			this->populateLocomotiveShed();
			this->systemSetupComplete();
		};

		static const std::string name()
		{
			return _Railway::element_type::name();
		}

		tl::optional<Locomotive&> locoFromAddress(const Address address)
		{
			auto it = std::find_if(this->locomotiveShed.begin(), this->locomotiveShed.end(), [address](const auto& loco) { return loco.address() == address; });
			if (it == this->locomotiveShed.end())
				return tl::nullopt;
			else
				return *it;
		}

		const Address addressOfLoco(const Locomotive& loco) const
		{
			return loco.address();
		}

		inline const State turnoutChangeTo(winston::Turnout::Shared turnout, winston::Turnout::Direction direction)
		{
			this->digitalCentralStation->triggerTurnoutChangeTo(turnout, direction);
			return turnout->startToggle();
		}

		inline const State locoFunction(const winston::Address address, const uint32_t functions)
		{
			this->digitalCentralStation->triggerLocoFunction(address, functions);
			return winston::State::Finished;
		}

		inline const State locoDrive(const Address address, const unsigned char speed, const bool forward)
		{
			this->digitalCentralStation->triggerLocoDrive(address, speed, forward);
			return winston::State::Finished;
		}

		bool loop()
		{
			{
#ifdef WINSTON_STATISTICS
				StopwatchJournal::Event tracer(this->stopWatchJournal, "digitalCentralStation loop");
#endif
				this->digitalCentralStation->tick();
			}
			return this->systemLoop();
		};
		using Railway = _Railway;

#ifdef WINSTON_STATISTICS
		const std::string statistics(const size_t withTop = 0) const { return this->stopWatchJournal.toString(withTop); }
		const std::string statisticsSignalBox(const size_t withTop = 0) const { return this->signalBox->statistics(withTop); }

	protected:
		StopwatchJournal stopWatchJournal;
#endif
	protected:
		virtual void systemSetup() = 0;
		virtual void systemSetupComplete() = 0;
		virtual bool systemLoop() = 0;
		virtual void populateLocomotiveShed() = 0;

		void addLocomotive(const winston::Locomotive::Callbacks callbacks, const Address address, std::string name, const NFCAddress nfcAddress)
		{
			//auto loco = Locomotive();
			Position pos(this->railway->track(0), winston::Track::Connection::A, 0);
			this->locomotiveShed.emplace_back(callbacks, address, pos, name, nfcAddress);
		}

		Result loadLocomotives()
		{

		}

		Result storeLocomotives()
		{

		}

		// the railway
		_Railway railway;
		using Tracks = typename _Railway::element_type::Tracks;

		SignalBox::Shared signalBox;

		// the z21 digital central station
		_DigitalCentralStation digitalCentralStation;
		_AddressTranslator addressTranslator;
		DigitalCentralStation::DebugInjector::Shared stationDebugInjector;

		// the locos
		std::vector<Locomotive> locomotiveShed;
	};
}

