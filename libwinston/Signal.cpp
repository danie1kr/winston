#include "Signal.h"

namespace winston
{
	Signal::Signal(const Callback callback)
		: callback(callback), _aspect(Aspect::Stop)
	{

	}

	const State Signal::set(const Aspect aspect)
	{
		this->_aspect = aspect;
		return this->callback(this->_aspect);
	}

	const Signal::Aspect Signal::aspect()
	{
		return this->_aspect;
	}
}