#include "Signal.h"

namespace winston
{
	Signal::Signal(const Callback callback)
		: callback(callback), _aspect((unsigned int)Aspect::Off)
	{

	}

	Signal::Callback Signal::defaultCallback()
	{
		return [](const Aspects aspect)->const State { return State::Finished; };
	}

	const State Signal::aspect(const Aspect aspect)
	{
		// pre-signal off if main signal shows Halt
		if ((unsigned int)aspect & Signal::MaskPreSignalAspect)
		{
			if(this->_aspect & (unsigned int)Signal::Aspect::Halt)
				this->_aspect = this->_aspect & (unsigned int)Signal::MaskMainSignalAspect; // & pre-signal Off
			else
				this->_aspect = (this->_aspect & (unsigned int)Signal::MaskMainSignalAspect) | (unsigned int)aspect; // & pre-signal Off
		}
		else
		{
			// Go: keep pre signal
			// Halt: pre signal off
			if ((unsigned int)aspect & (unsigned int)Signal::Aspect::Go)
				this->_aspect = (this->_aspect & (unsigned int)Signal::MaskPreSignalAspect) | (unsigned int)aspect;
			else
				this->_aspect = (unsigned int)aspect;
		}
		return this->callback(this->_aspect);
	}

	const Signal::Aspects Signal::aspect()
	{
		return this->_aspect;
	}

	const bool Signal::shows(Aspect aspect)
	{
		return (const unsigned int)this->_aspect & (const unsigned int)aspect;
	}
}