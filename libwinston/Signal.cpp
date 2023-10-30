#include "Signal.h"

namespace winston
{
	Signal::Light Signal::Light::make(const Port port, const Aspect aspect)
	{
		return { port, aspect };
	}

	constexpr unsigned int Signal::Light::maximum(const Aspect aspect)
	{
		switch (aspect)
		{
		case Aspect::Off: return 0;
		case Aspect::Go: return (range / 12) - 1;
		case Aspect::Halt: return (range / 12) - 1;
		case Aspect::ExpectGo: return (range / 8) - 1;
		case Aspect::ExpectHalt: return (range / 4) - 1;
		}
		return 0;
	}

	const std::string Signal::buildAspects(const Aspects first)
	{
		std::string s;
		if (first & (unsigned int)Signal::Aspect::Off) s += "Off, ";
		if (first & (unsigned int)Signal::Aspect::Go) s += "Go, ";
		if (first & (unsigned int)Signal::Aspect::Halt) s += "Halt, ";
		if (first & (unsigned int)Signal::Aspect::ExpectHalt) s += "ExpectHalt, ";
		if (first & (unsigned int)Signal::Aspect::ExpectGo) s += "ExpectGo";
		return s;
	}

	Signal::Signal(Callback callback, const Length distance)
		: callback(callback), _distance(distance), _aspect((unsigned int)Aspect::Go), _forced(0)
	{

	}

	Signal::Callback Signal::defaultCallback()
	{
		return [](const Aspects aspect)->const State { return State::Finished; };
	}

	void Signal::init() { }

	const State Signal::aspect(const Aspect aspect)
	{
		// pre-signal off if main signal shows Halt
		/*if ((unsigned int)aspect & Signal::MaskPreSignalAspect)
		{
			if(this->_aspect & (unsigned int)Signal::Aspect::Halt)
		//		this->_aspect = this->_aspect & (unsigned int)Signal::MaskMainSignalAspect; // & pre-signal Off
		//	else
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
		}*/

		if ((unsigned int)aspect & Signal::MaskPreSignalAspect)
			this->_aspect = (this->_aspect & (unsigned int)Signal::MaskMainSignalAspect) | (unsigned int)aspect;
		if (aspect & Signal::Aspect::Go)
			this->_aspect = (this->_aspect & (unsigned int)Signal::MaskPreSignalAspect) | aspect;
		if (aspect & Signal::Aspect::Halt)
			this->_aspect = (unsigned int)aspect;
		if (aspect & Signal::Aspect::Off)
			this->_aspect = (unsigned int)Signal::Aspect::Off;

		this->updateLights();
		return this->callback(this->aspect());
	}

	const Signal::Aspects Signal::aspect() const
	{
		if (this->_forced)
			return this->_forced;
		else
			return this->_aspect;
	}

	const bool Signal::shows(Aspect aspect) const
	{
		return (const unsigned int)this->aspect() & (const unsigned int)aspect;
	}

	const Length Signal::distance() const
	{
		return this->_distance;
	}

	void Signal::overwrite(const Aspects aspect)
	{
		this->_forced = aspect;
		this->updateLights();
	}

	template<> void SignalKS::init() { }
	template<> void SignalKS::updateLights()
	{
		/*if (this->_forced)
		{
			if ((const unsigned int)this->_forced & (const unsigned int)Aspect::Off)
			{
				this->_lights[0].value = 0;
				this->_lights[1].value = 0;
				this->_lights[2].value = 0;
			}
			else
			{
				this->_lights[0].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
				this->_lights[1].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
				this->_lights[2].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
			}
			return;
		}*/
		// Go, Halt, ExpectHalt
		this->_lights[0].value = this->shows(Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
		this->_lights[1].value = this->shows(Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
		this->_lights[2].value = this->shows(Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
	}
	template<> void SignalH::init() { }
	template<> void SignalH::updateLights()
	{
		/*if (this->_forced)
		{
			if ((const unsigned int)this->_forced & (const unsigned int)Aspect::Off)
			{
				this->_lights[0].value = 0;
				this->_lights[1].value = 0;
			}
			else
			{
				this->_lights[0].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
				this->_lights[1].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
			}
			return;
		}*/
		// Go, Halt
		this->_lights[0].value = this->shows(Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
		this->_lights[1].value = this->shows(Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
	}
	template<> void SignalV::init() { }
	template<> void SignalV::updateLights()
	{
		/*if (this->_forced)
		{
			if ((const unsigned int)this->_forced & (const unsigned int)Aspect::Off)
			{
				this->_lights[0].value = 0;
				this->_lights[1].value = 0;
			}
			else
			{
				this->_lights[0].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
				this->_lights[1].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
			}
			return;
		}*/
		// ExpectHalt, ExpectGo
		this->_lights[0].value = this->shows(Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
		this->_lights[1].value = this->shows(Aspect::ExpectGo) ? Light::maximum(Aspect::Go) : 0;
	}
	template<> void SignalHV::init() { }
	template<> void SignalHV::updateLights()
	{
		/*if (this->_forced)
		{
			if ((const unsigned int)this->_forced & (const unsigned int)Aspect::Off)
			{
				this->_lights[0].value = 0;
				this->_lights[1].value = 0;
				this->_lights[2].value = 0;
				this->_lights[3].value = 0;
			}
			else
			{
				this->_lights[0].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
				this->_lights[1].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
				this->_lights[2].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
				this->_lights[3].value = ((const unsigned int)this->_forced & (const unsigned int)Aspect::ExpectGo) ? Light::maximum(Aspect::ExpectGo) : 0;
			}
			return;
		}*/
		// Go, Halt, ExpectHalt, ExpectGo
		this->_lights[0].value = this->shows(Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
		this->_lights[1].value = this->shows(Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
		this->_lights[2].value = this->shows(Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
		this->_lights[3].value = this->shows(Aspect::ExpectGo) ? Light::maximum(Aspect::ExpectGo) : 0;
	}

	template<> void SignalAlwaysHalt::init()
	{
		this->_aspect = (unsigned int)Aspect::Halt;
	}

	template<> void SignalAlwaysHalt::updateLights()
	{
		this->_lights[0].value = Light::maximum(Aspect::Halt);
	}

	SignalDevice::SignalDevice(const size_t ports)
		: ports(ports)
	{

	}

	SignalDevice::~SignalDevice()
	{

	}

	const Result SignalDevice::update(const winston::Signal &signal)
	{
		this->lastUpdate = hal::now();
		return this->updateInternal(signal);
	}

	const Result SignalDevice::flush()
	{
		this->lastFlush = hal::now();
		return this->flushInternal();
	}

	Command::Shared SignalDevice::flushCommand(const Duration waitPeriod)
	{
		return Command::make([=](const TimePoint& created) -> const State
			{
				auto now = hal::now();
				if ((now - lastFlush) > waitPeriod && lastUpdate > lastFlush)
				{
					this->flush();
					return State::Running;
				}
				else
					return State::Delay;
			}, __PRETTY_FUNCTION__);
	}
}