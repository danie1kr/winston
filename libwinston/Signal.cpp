#include "Signal.h"

namespace winston
{
	Signal::Signal(const Callback callback)
		: callback(callback), _aspect(Aspect::Halt)
	{

	}

	Signal::Callback Signal::defaultCallback()
	{
		return [](const Aspect aspect)->const State { return State::Finished; };
	}

	const State Signal::aspect(const Signal::Aspect aspect)
	{
		this->_aspect = aspect;
		return this->callback(this->_aspect);
	}

	const Signal::Aspect Signal::aspect()
	{
		return this->_aspect;
	}

	const bool Signal::shows(Aspect aspect)
	{
		return (const unsigned int)this->_aspect & (const unsigned int)aspect;
	}
}