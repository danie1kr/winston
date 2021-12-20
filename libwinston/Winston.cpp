#include "Winston.h"
#include "Log.h"
#include "HAL.h"

namespace winston
{
	Logger logger;

	void error(std::string error)
	{
		hal::text("error: " + error);
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