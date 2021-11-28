#include "Signal.h"

namespace winston
{
	Signal::Light Signal::Light::make(const Port port)
	{
		return { .port = port };
	}

	Signal::Signal(const Callback callback, const Length distance)
		: callback(callback), _distance(distance), _aspect((unsigned int)Aspect::Off)
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
			//if(this->_aspect & (unsigned int)Signal::Aspect::Halt)
			//	this->_aspect = this->_aspect & (unsigned int)Signal::MaskMainSignalAspect; // & pre-signal Off
			//else
				this->_aspect = (this->_aspect & (unsigned int)Signal::MaskMainSignalAspect) | (unsigned int)aspect; // & pre-signal Off
		}
		else
		{
			// Go: keep pre signal
			// Halt: pre signal off
			//if ((unsigned int)aspect & (unsigned int)Signal::Aspect::Go)
				this->_aspect = (this->_aspect & (unsigned int)Signal::MaskPreSignalAspect) | (unsigned int)aspect;
			//else
			//	this->_aspect = (unsigned int)aspect;
		}
		this->updateLights();
		return this->callback(this->_aspect);
	}

	const Signal::Aspects Signal::aspect() const
	{
		return this->_aspect;
	}

	const bool Signal::shows(Aspect aspect) const
	{
		return (const unsigned int)this->_aspect & (const unsigned int)aspect;
	}

	const Length Signal::distance() const
	{
		return this->_distance;
	}

	template<> void SignalKS::updateLights()
	{
		// Halt, Go, ExpectHalt
		this->_lights[0].value = this->shows(Aspect::Halt) ? Light::maximum : 0;
		this->_lights[1].value = this->shows(Aspect::Go) ? Light::maximum : 0;
		this->_lights[2].value = this->shows(Aspect::ExpectHalt) ? Light::maximum : 0;
	}
	template<> void SignalH::updateLights()
	{
		// Halt, Go
		this->_lights[0].value = this->shows(Aspect::Halt) ? Light::maximum : 0;
		this->_lights[1].value = this->shows(Aspect::Go) ? Light::maximum : 0;
	}
	template<> void SignalV::updateLights()
	{
		// ExpectHalt, ExpectGo
		this->_lights[0].value = this->shows(Aspect::ExpectHalt) ? Light::maximum : 0;
		this->_lights[1].value = this->shows(Aspect::ExpectGo) ? Light::maximum : 0;
	}
	template<> void SignalHV::updateLights()
	{
		// Halt, Go, ExpectHalt, ExpectGo
		this->_lights[0].value = this->shows(Aspect::Halt) ? Light::maximum : 0;
		this->_lights[1].value = this->shows(Aspect::Go) ? Light::maximum : 0;
		this->_lights[2].value = this->shows(Aspect::ExpectHalt) ? Light::maximum : 0;
		this->_lights[3].value = this->shows(Aspect::ExpectGo) ? Light::maximum : 0;
	}
}