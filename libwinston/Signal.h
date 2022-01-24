#pragma once

#include "WinstonTypes.h"
#include "HAL.h"
#include "Util.h"
#include <functional>
#include "span.hpp"
#include "Command.h"

namespace winston
{
	class Signal : public Shared_Ptr<Signal>
	{
	public:
		enum class Aspect
		{
			Off =			0b00001,
			Halt =			0b00010,
			Go =			0b00100,
			ExpectHalt =	0b01000,
			ExpectGo =		0b10000
		};
		using Aspects = unsigned int;

		struct Light
		{
			static Light make(const Port port);
			const Port port;
			unsigned int value = 0;
			static const unsigned int range = 1 << 12;
			static const unsigned int maximum = range - 1;
		};

		/*
		
		
		==== S1 ==== S2 ==== S3 ====
		    Halt
		
		==== S1 ==== S2 ==== S3 ====
			 H       H       H

		==== S1 ==== S2 ==== S3 ====
			 V       H
			 =S2


		==== S1 ==== S2 ==== S3 ====
			V		 HV		 H
			= S2     V: H == Halt ? Off : S3

		==== S1 ==== S2 ==== S3 ====
			KS		KS		KS
			= S2 == Halt ? 

		==== S1 ====B1==== S2 ====B2==== S3 ====
			KS
			: B1 == free ? (S2 == Halt ? ExpectHalt : Go) : Halt

			V
			: S2 == Halt ? ExpectHalt : ExpectGo

			HV
			V: B1 == free ? (S2 == Halt ? ExpectHalt : ExpectGo) : Off
			H: B1 == free ? Go : Halt

		*/

		static const Aspects AspectsProtection = (unsigned int)Aspect::Off
			| (unsigned int)Aspect::Halt
			| (unsigned int)Aspect::Go;

		static const Aspects AspectsKS = (unsigned int)Aspect::Halt
			| (unsigned int)Aspect::Go
			| (unsigned int)Aspect::ExpectHalt;

		static const Aspects AspectsH = (unsigned int)Aspect::Halt
			| (unsigned int)Aspect::Go;

		static const Aspects AspectsV = (unsigned int)Aspect::Off
			| (unsigned int)Aspect::ExpectHalt
			| (unsigned int)Aspect::ExpectGo;

		static const Aspects MaskPreSignalAspect =
			(unsigned int)Aspect::ExpectHalt | (unsigned int)Aspect::ExpectGo;
		static const Aspects MaskMainSignalAspect =
			(unsigned int)Aspect::Halt | (unsigned int)Aspect::Go;

		static const Aspects AspectsHV = AspectsH | AspectsV;

		using Callback = std::function<const State(const Aspects aspect)>;
		static Callback defaultCallback();
		Signal(const Callback callback = defaultCallback(), const Length distance = 0);

		const State aspect(const Aspect aspect);
		const Aspects aspect() const;
		const bool shows(Aspect aspect) const;
		virtual const bool supports(const Aspect aspect, const bool any) const = 0;
		virtual const bool preSignal() const = 0;
		virtual const bool mainSignal() const = 0;

		const Length distance() const;
		
		virtual const std::span<const Light> lights() const = 0;
		virtual const unsigned int lightsCount() const = 0;
	protected:

		virtual void updateLights() = 0;

		const Callback callback;
		const Length _distance;
		Aspects _aspect;
	};

	inline const bool operator&(const Signal::Aspect a, const Signal::Aspect b)
	{
		return static_cast<const bool>(static_cast<const unsigned int>(a) & static_cast<const unsigned int>(b));
	}
	
	template<unsigned int _Aspects, unsigned int _Lights>
	class SignalInstance : public Signal, public Shared_Ptr<SignalInstance<_Aspects, _Lights>>
	{
	public:
		SignalInstance(const Callback callback = Signal::defaultCallback(), const Length distance = 0, const Port port = Port())
			: Signal(callback, distance)
			, _lights(Sequence<_Lights-1>::generate(port.device(), port.port())) {
		};

		const bool supports(const Aspect aspect, const bool any) const
		{
			const unsigned int eval = (const unsigned int)aspect & (const unsigned int)_Aspects;
			if (any)
				return eval != 0;
			else
				return eval == (const unsigned int)aspect;
		}

		const bool preSignal() const
		{
			return ((const unsigned int)_Aspects & ((const unsigned int)Aspect::ExpectHalt | (const unsigned int)Aspect::ExpectGo)) != 0;
		}

		const bool mainSignal() const
		{
			return ((const unsigned int)_Aspects & ((const unsigned int)Aspect::Go | (const unsigned int)Aspect::Halt)) != 0;
		}

		const std::span<const Light> lights() const {
			return this->_lights;
		}

		const unsigned int lightsCount() const {
			return _Lights;
		};

		void updateLights(); // { static_assert(false, "do not use"); };

		using Shared_Ptr<SignalInstance<_Aspects, _Lights>>::Shared;
		using Shared_Ptr<SignalInstance<_Aspects, _Lights>>::make;

	private:
		typedef std::array<Light, _Lights> LightsArray;

		template<int... i> static constexpr LightsArray makeLightInSequence(const size_t device, const size_t startPort) { return LightsArray{ { Signal::Light::make(Port(device, startPort + i))... } }; }
		template<int...> struct Sequence;
		template<int... i> struct Sequence<0, i...>
		{
			static constexpr LightsArray generate(const size_t device, const size_t startPort) { return makeLightInSequence<0, i...>(device, startPort); }
		};

		template<int i, int... j> struct Sequence<i, j...>
		{
			static constexpr LightsArray generate(const size_t device, const size_t startPort) { return Sequence<i - 1, i, j...>::generate(device, startPort); }
		};

		LightsArray _lights;
	};

	using SignalKS = SignalInstance<Signal::AspectsKS, 3>;
	using SignalH = SignalInstance<Signal::AspectsH, 2>;
	using SignalV = SignalInstance<Signal::AspectsV, 2>;
	using SignalHV = SignalInstance<Signal::AspectsHV, 4>;

	template<typename T>
	class SignalDevice : public Shared_Ptr<SignalDevice<T>>
	{
		//static_assert(bits <= sizeof(T) * 8, "too many bits for T");
	public:
		SignalDevice(const size_t devices, const size_t portsPerDevice, typename SendDevice<T>::Shared device)
			: device(device), devices(devices), portsPerDevice(portsPerDevice)
		{

		}

		const Result update(winston::Signal::Shared signal)
		{
			this->lastUpdate = hal::now();
			return this->updateInternal(signal);
		}

		const Result flush()
		{
			this->lastFlush = hal::now();
			return this->flushInternal();
		}

		Command::Shared flushCommand(const Duration waitPeriod = std::chrono::milliseconds(40))
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

		using Shared_Ptr<SignalDevice<T>>::Shared;
		using Shared_Ptr<SignalDevice<T>>::make;
	protected:
		virtual const Result updateInternal(winston::Signal::Shared signal) = 0;
		virtual const Result flushInternal() = 0;
		typename SendDevice<T>::Shared device;
		size_t devices;
		size_t portsPerDevice;
	private:
		TimePoint lastFlush;
		TimePoint lastUpdate;
	};
}