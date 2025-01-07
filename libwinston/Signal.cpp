#include "Signal.h"

namespace winston
{
	Signal::Light Signal::Light::make(const Port port, const Aspect aspect)
	{
		return { port, aspect };
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

	Signal::Signal(const Id deviceId, Callback callback, const Length distance)
		: deviceId(deviceId), callback(callback), _distance(distance), _aspect((unsigned int)Aspect::Go), authorityHalt{ { false, false } }

	{

	}

	Signal::Callback Signal::defaultCallback()
	{
		return [](const Aspects aspect)->const State { return State::Finished; };
	}

	void Signal::init() { }

	const State Signal::aspect(const Aspect aspect, const Authority authority)
	{
		this->authorityHalt[(size_t)authority] = aspect & Signal::Aspect::Halt;

		if ((unsigned int)aspect & Signal::MaskPreSignalAspect)
			this->_aspect = (this->_aspect & (unsigned int)Signal::MaskMainSignalAspect) | aspect;
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
		if (this->authorityHalt[(size_t)Authority::Turnout] || this->authorityHalt[(size_t)Authority::Occupancy])
			//                           unset ::Go             and set ::Halt
			return (this->_aspect & ~((unsigned int)Aspect::Go)) | Aspect::Halt;
		else
			return this->_aspect;
	}

	void Signal::grabAuthorities(Signal::Shared other)
	{
		auto authorities = other->authorities();
		this->authorityHalt = authorities;
		/*auto thisAuthorities = this->authorities();
		this->authorities(other->authorities());
		other->authorities(thisAuthorities);
		
		auto hasHalt = this->shows(Aspect::Halt);
		this->aspect(other->shows(Aspect::Halt) ? Aspect::Halt : Aspect::Go);
		other->aspect(hasHalt ? Aspect::Halt : Aspect::Go);*/
	}
	void Signal::clearAuthorities()
	{
		this->authorityHalt[(size_t)Authority::Turnout] = false;
		this->authorityHalt[(size_t)Authority::Occupancy] = false;
	}

	void Signal::authorities(const Authorities authorities)
	{
		this->authorityHalt = authorities;
	}

	const Signal::Authorities Signal::authorities() const
	{
		return this->authorityHalt;
	}

	const bool Signal::shows(Aspect aspect) const
	{
		return (const unsigned int)this->aspect() & (const unsigned int)aspect;
	}

	const Length Signal::distance() const
	{
		return this->_distance;
	}

	template<> void SignalKS::init() { }
	template<> void SignalKS::updateLights()
	{
		// Go, Halt, ExpectHalt
		this->_lights[0].value = this->shows(Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
		this->_lights[1].value = this->shows(Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
		this->_lights[2].value = this->shows(Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
	}
	template<> void SignalH::init() { }
	template<> void SignalH::updateLights()
	{
		// Go, Halt
		this->_lights[0].value = this->shows(Aspect::Go) ? Light::maximum(Aspect::Go) : 0;
		this->_lights[1].value = this->shows(Aspect::Halt) ? Light::maximum(Aspect::Halt) : 0;
	}
	template<> void SignalV::init() { }
	template<> void SignalV::updateLights()
	{
		// ExpectHalt, ExpectGo
		this->_lights[0].value = this->shows(Aspect::ExpectHalt) ? Light::maximum(Aspect::ExpectHalt) : 0;
		this->_lights[1].value = this->shows(Aspect::ExpectGo) ? Light::maximum(Aspect::Go) : 0;
	}
	template<> void SignalHV::init() { }
	template<> void SignalHV::updateLights()
	{
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

	SignalDevice::SignalDevice(const Id id, const size_t ports)
		: id(id), ports(ports)
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

	NextSignal::NextSignal(const Signal::Shared signal, const Distance distance, const Signal::Pass pass)
		: signal(signal), distance(distance), pass(pass)
	{
	}

	NextSignals::NextSignals()
		: nextSignals{ }
	{

	}

	void NextSignals::put(NextSignal::Const next, const bool forward, const Signal::Pass pass)
	{
		this->nextSignals[NextSignals::index(forward, pass)] = next;
	}
	
	const NextSignal::Const NextSignals::get(const bool forward, const Signal::Pass pass) const
	{
		return this->nextSignals[NextSignals::index(forward, pass)];
	}

	const bool NextSignals::contains(const Signal::Const signal) const
	{
		if (this->nextSignals[index(true, Signal::Pass::Facing)] && this->nextSignals[index(true, Signal::Pass::Facing)]->signal == signal)
			return true;
		if (this->nextSignals[index(false, Signal::Pass::Facing)] && this->nextSignals[index(false, Signal::Pass::Facing)]->signal == signal)
			return true;
		if (this->nextSignals[index(true, Signal::Pass::Backside)] && this->nextSignals[index(true, Signal::Pass::Backside)]->signal == signal)
			return true;
		if (this->nextSignals[index(false, Signal::Pass::Backside)] && this->nextSignals[index(false, Signal::Pass::Backside)]->signal == signal)
			return true;
		return false;
	}
		
	size_t constexpr NextSignals::index(const bool forward, const Signal::Pass pass)
	{
		return (size_t)pass + (size_t)forward * 2;
	};
}