
#pragma once

#include <functional>

#include "WinstonTypes.h"
#include "HAL.h"
#include "Util.h"
#include "span.hpp"
#include "Command.h"


// constexpr to count bits in i
template<size_t> struct BitCounter;
template<> struct BitCounter<0>
{
	static constexpr size_t count() { return 0; }
};
template<> struct BitCounter<1>
{
	static constexpr size_t count() { return 1; }
};
template<size_t i> struct BitCounter
{
	static constexpr size_t count() { return (i & 1) + BitCounter<(i >> 1)>::count(); }
};

// constexpr to get the bit 1 << N so that (1<<N) is the nth set bit in value
template<size_t, size_t> class nThSetBit;
template<> class nThSetBit<0, 0>
{
public:
	static constexpr size_t extract(size_t value)
	{
		(void)value;
		return 0;
	};
};
template<size_t N> class nThSetBit<0, N>
{
public:
	static constexpr size_t extract(size_t value)
	{
		(void)value;
		return 1 << N;
	};
};
template<size_t I> class nThSetBit<I, 0>
{
public:
	static constexpr size_t extract(size_t value)
	{
		(void)value;
		return 0;
	};
};
template<size_t I, size_t N> class nThSetBit
{
public:
	static constexpr size_t extract(size_t value)
	{
		if (value & (1 << N))
		{
			if (I == 1)
				return 1 << N;
			else
				return nThSetBit<I - 1, N - 1>::extract(value);
		}
		else
			return nThSetBit<I, N - 1>::extract(value);

	};
};

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
			static Light make(const Port port, const Aspect aspect = Aspect::Off);
			const Port port;
			const Aspect aspect;
			unsigned int value = 0;
			static const unsigned int range = 1 << 12;
			static const unsigned int maximum = (range / WINSTON_SIGNAL_LIGHT_DIV) - 1;
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

		static const Aspects AspectsProtection = /*(unsigned int)Aspect::Off
			|*/ (unsigned int)Aspect::Halt
			| (unsigned int)Aspect::Go;

		static const Aspects AspectsKS = (unsigned int)Aspect::Halt
			| (unsigned int)Aspect::Go
			| (unsigned int)Aspect::ExpectHalt;

		static const Aspects AspectsH = (unsigned int)Aspect::Halt
			| (unsigned int)Aspect::Go;

		static const Aspects AspectsV = /*(unsigned int)Aspect::Off
			|*/ (unsigned int)Aspect::ExpectHalt
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
	
	template<size_t _Aspects>
	class SignalInstance : public Signal, public Shared_Ptr<SignalInstance<_Aspects>>
	{
	public:
		SignalInstance(const Callback callback = Signal::defaultCallback(), const Length distance = 0, const Port port = 0)
			: Signal(callback, distance)
			, _lights(Sequence<BitCounter<_Aspects>::count() - 1>::generate(_Aspects, port)) {
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

		static constexpr size_t lightsCount() {
			return BitCounter<_Aspects>::count();
		};

		void updateLights(); // { static_assert(false, "do not use"); };

		using Shared_Ptr<SignalInstance<_Aspects>>::Shared;
		using Shared_Ptr<SignalInstance<_Aspects>>::make;

	private:
		typedef std::array<Light, BitCounter<_Aspects>::count()> LightsArray;
		template<int... i> static constexpr LightsArray makeLightInSequence(const Signal::Aspects aspects, const unsigned int startPort)
		{ 
			return LightsArray{ { Signal::Light::make(startPort + i, (Signal::Aspect)nThSetBit<i+1, sizeof(Signal::Aspect) * 8 - 1>::extract(aspects))...}};
		}
		template<int...> class Sequence;
		template<int... i> class Sequence<0, i...>
		{
		public:
			static constexpr LightsArray generate(const Signal::Aspects aspects, const unsigned int startPort) { return makeLightInSequence<0, i...>(aspects, startPort); }
		};

		template<int i, int... j> class Sequence<i, j...>
		{
		public:
			static constexpr LightsArray generate(const Signal::Aspects aspects, const unsigned int startPort) { return Sequence<i - 1, i, j...>::generate(aspects, startPort); }
		};

		LightsArray _lights;
	};

	constexpr Signal::Aspects operator |(Signal::Aspect a, Signal::Aspect b)
	{
		return (unsigned int)a | (unsigned int)b;
	}
	constexpr Signal::Aspects operator |(Signal::Aspects a, Signal::Aspect b)
	{
		return (unsigned int)a | (unsigned int)b;
	}
	constexpr Signal::Aspects operator |(Signal::Aspect a, Signal::Aspects b)
	{
		return (unsigned int)a | (unsigned int)b;
	}

	using SignalKS = SignalInstance<Signal::AspectsKS>;
	using SignalH = SignalInstance<Signal::AspectsH>;
	using SignalV = SignalInstance<Signal::AspectsV>;
	using SignalHV = SignalInstance<Signal::AspectsHV>;

	template<typename T>
	class SignalDevice : public Shared_Ptr<SignalDevice<T>>
	{
		//static_assert(bits <= sizeof(T) * 8, "too many bits for T");
	public:
		SignalDevice(const size_t ports, typename SendDevice<T>::Shared device)
			: device(device), ports(ports)
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

		Command::Shared flushCommand(const Duration waitPeriod = toMilliseconds(40))
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
		size_t ports;
	private:
		TimePoint lastFlush;
		TimePoint lastUpdate;
	};
}