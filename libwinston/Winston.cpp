//#include "Winston.h"
#include "WinstonTypes.h"
#include "Log.h"
#include "HAL.h"

namespace winston
{
#define RUNTIME_LAMBDAS(what) \
	const bool runtime##what() { return runtimeHardwareState.enabled(RuntimeHardwareState::Type::what); }; \
	void runtimeEnable##what () { runtimeHardwareState.enable(RuntimeHardwareState::Type::what); };
	RUNTIME_LAMBDAS(Serial);
	RUNTIME_LAMBDAS(SPI);
	RUNTIME_LAMBDAS(Persistence);
	RUNTIME_LAMBDAS(Network);
	RUNTIME_LAMBDAS(Railway);

	GPIOPinDevice::GPIOPinDevice(const Pin pin) 
		: Shared_Ptr<GPIOPinDevice>(), pin(pin)
	{ }

	GPIODigitalPinOutputDevice::GPIODigitalPinOutputDevice(const Pin pin, const State initial)
		: GPIOPinDevice(pin), Shared_Ptr<GPIODigitalPinOutputDevice>()
	{ }

	GPIODigitalPinInputDevice::GPIODigitalPinInputDevice(const Pin pin, const Mode direction)
		: GPIOPinDevice(pin), Shared_Ptr<GPIODigitalPinInputDevice>()
	{ }

	RuntimeHardwareState runtimeHardwareState;
	RuntimeHardwareState::RuntimeHardwareState() : state(0) { };

	Looper::Looper()
		: Shared_Ptr<Looper>()
	{

	}

	const bool Looper::connected() const
	{
		return true;
	}

	void logRuntimeStatus()
	{
#define RHS_Print(what) LOG_INFO(#what ": o", runtime##what() ? "n" : "ff");
		LOG_INFO("Winston Runtime Status");
		RHS_Print(Persistence);
		RHS_Print(Network);
		RHS_Print(SPI);
		RHS_Print(Railway);
	}
	/*
	Port::Port() :
		_device(0), _port(0)
	{

	}

	Port::Port(const size_t device, const size_t port) :
		_device(device), _port(port)
	{

	}

	const size_t Port::device() const { return this->_device; }
	const size_t Port::port() const { return this->_port; }*/
}