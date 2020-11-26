#pragma once
#include "HAL.h"
#include "Railway.h"
#include "SignalBox.h"
#include "DigitalCentralStation.h"

namespace winston
{
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation>
	class ModelRailwaySystem
	{
	public:
		ModelRailwaySystem() { };
		virtual ~ModelRailwaySystem() { } ;

		void setup() {
			Result result;
			
			winston::hal::init();

			this->systemSetup();

			this->railway->init();
			result = this->digitalCentralStation->connect();

			this->populateLocomotiveShed();
			this->systemSetupComplete();
		};

		static const std::string name()
		{
			return _Railway::element_type::name();
		}

		Locomotive::Shared get(const Address address)
		{
			auto it = std::find_if(this->locomotiveShed.begin(), this->locomotiveShed.end(), [address](const auto& loco) { return loco->address() == address; });
			if (it == this->locomotiveShed.end())
				return nullptr;
			else
				return *it;
		}

		void loop() {
			this->systemLoop();
		};
		using Railway = _Railway;

	protected:
		virtual void systemSetup() = 0;
		virtual void systemSetupComplete() = 0;
		virtual void systemLoop() = 0;
		virtual void populateLocomotiveShed() = 0;


		void addLocomotive(Address address, std::string name)
		{
			auto loco = Locomotive::make(address, name);
			this->locomotiveShed.push_back(loco);
		}

		// the railway
		_Railway railway;
		SignalBox::Shared signalBox;
		NullMutex nullMutex;

		// the z21 digital central station
		_DigitalCentralStation digitalCentralStation;
		_AddressTranslator addressTranslator;
		DigitalCentralStation::DebugInjector::Shared stationDebugInjector;

		// the locos
		std::vector<Locomotive::Shared> locomotiveShed;
	};
}

