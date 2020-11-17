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
			winston::hal::init();

			this->systemSetup();

			this->railway->init();
			this->digitalCentralStation->connect();

			this->systemSetupComplete();
		};

		static const std::string name()
		{
			return _Railway::element_type::name();
		}

		void loop() {
			this->systemLoop();
		};
		using Railway = _Railway;

	protected:
		virtual void systemSetup() = 0;
		virtual void systemSetupComplete() = 0;
		virtual void systemLoop() = 0;

		// the railway
		_Railway railway;
		SignalBox::Shared signalBox;
		NullMutex nullMutex;

		// the z21 digital central station
		_DigitalCentralStation digitalCentralStation;
		_AddressTranslator addressTranslator;
		DigitalCentralStation::DebugInjector::Shared stationDebugInjector;
	};
}

