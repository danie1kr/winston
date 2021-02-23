#pragma once

#include "WinstonTypes.h"
#include "Util.h"
#include <functional>

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
		Signal(const Callback callback = defaultCallback());

		const State aspect(const Aspect aspect);
		const Aspects aspect();
		const bool shows(Aspect aspect);
		virtual const bool supports(const Aspect aspect, const bool any) = 0;
		virtual const bool preSignal() = 0;
		virtual const bool mainSignal() = 0;
	protected:
		Aspects _aspect;
		const Callback callback;
	};
	
	template<unsigned int _Aspects>
	class SignalInstance : public Signal, public Shared_Ptr<SignalInstance<_Aspects>>
	{
	public:
		SignalInstance(const Callback callback = Signal::defaultCallback()) : Signal(callback) { };

		const bool supports(const Aspect aspect, const bool any)
		{
			const unsigned int eval = (const unsigned int)aspect & (const unsigned int)_Aspects;
			if (any)
				return eval != 0;
			else
				return eval == (const unsigned int)aspect;
		}

		const bool preSignal()
		{
			return ((const unsigned int)_Aspects & ((const unsigned int)Aspect::ExpectHalt | (const unsigned int)Aspect::ExpectGo)) != 0;
		}

		const bool mainSignal()
		{
			return ((const unsigned int)_Aspects & ((const unsigned int)Aspect::Go | (const unsigned int)Aspect::Halt)) != 0;
		}
		using Shared_Ptr<SignalInstance<_Aspects>>::Shared;
		using Shared_Ptr<SignalInstance<_Aspects>>::make;
	};

	using SignalKS = SignalInstance<Signal::AspectsKS>;
	using SignalH = SignalInstance<Signal::AspectsH>;
	using SignalV = SignalInstance<Signal::AspectsV>;
	using SignalHV = SignalInstance<Signal::AspectsHV>;
}