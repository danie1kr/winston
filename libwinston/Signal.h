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
			Stop =			0b00010,
			Go =			0b00100,
			ExpectHalt =	0b01000,
			ExpectGo =		0b10000
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

		static const unsigned int AspectsProtection = (unsigned int)Aspect::Off
			| (unsigned int)Aspect::Stop
			| (unsigned int)Aspect::Go;

		static const unsigned int AspectsKS = (unsigned int)Aspect::Stop
			| (unsigned int)Aspect::Go
			| (unsigned int)Aspect::ExpectHalt;

		static const unsigned int AspectsH = (unsigned int)Aspect::Stop
			| (unsigned int)Aspect::Go;

		static const unsigned int AspectsV = (unsigned int)Aspect::Off
			| (unsigned int)Aspect::ExpectHalt
			| (unsigned int)Aspect::ExpectGo;

		static const unsigned int AspectsHV = AspectsH | AspectsV;

		using Callback = std::function<const State(const Aspect aspect)>;
		Signal(const Callback callback);

		const State set(const Aspect aspect);
		const Aspect aspect();
		virtual bool supports(const Aspect aspect) = 0;
	protected:
		Aspect _aspect;
		const Callback callback;
	};

	template<unsigned int _Aspects>
	class SignalInstance : public Signal, public Shared_Ptr<SignalInstance<_Aspects>>
	{
		SignalInstance(const Callback callback) : Signal(callback) { };

		bool supports(const Aspect aspect)
		{
			return (const unsigned int)aspect & (const unsigned int)_Aspects;
		}

		bool preSignal()
		{
			return (unsigned int)_Aspects & ((unsigned int)Aspect::ExpectHalt | (unsigned int)Aspect::ExpectGo);
		}
	};

	using SignalKS = SignalInstance<Signal::AspectsKS>;
	using SignalH = SignalInstance<Signal::AspectsH>;
	using SignalV = SignalInstance<Signal::AspectsV>;
	using SignalHV = SignalInstance<Signal::AspectsHV>;
}