#include "Winston.h"
#include "Log.h"
#include "HAL.h"

namespace winston
{
	Logger logger;
	RuntimeHardwareState runtimeHardwareState;

	RuntimeHardwareState::RuntimeHardwareState() : state(0) { };

	void logRuntimeStatus()
	{
#define RHS_Print(what) logger.info(#what ": o", runtimePersistence() ? "n" : "ff");
		logger.info("Winston Runtime Status");
		RHS_Print(Persistence);
		RHS_Print(Network);
		RHS_Print(SPI);
		RHS_Print(Railway);
	}

	Port::Port() :
		_device(0), _port(0)
	{

	}

	Port::Port(const size_t device, const size_t port) :
		_device(device), _port(port)
	{

	}

	const size_t Port::device() const { return this->_device; }
	const size_t Port::port() const { return this->_port; }
}