#pragma once

#include "Signal.h"
#include "Track.h"
#include "Railway.h"

namespace winston
{
	class SignalController : public Shared_Ptr<SignalController>
	{
	public:
		SignalController(const std::vector<typename SignalDevice::Shared> devices);
		virtual ~SignalController();

		template<class _Signal>
		const Result attach(winston::Track::Shared track, const winston::Track::Connection connection, winston::Distance distance, const winston::Railway::Callbacks::SignalUpdateCallback& signalUpdateCallback)
		{
			const auto lightsCount = (unsigned int)_Signal::lightsCount();
			if (currentPort + lightsCount > devices[currentDev]->ports)
			{
				currentDev++;
				currentPort = 0;
			}

			if (currentDev > devices.size())
			{
				winston::logger.err("Not enough devices for all signals");
				return Result::OutOfBounds;
			}
			auto s = _Signal::make(currentDev,
				[track, connection, signalUpdateCallback](const winston::Signal::Aspects aspect)->const winston::State {
					return signalUpdateCallback(track, connection, aspect);
				}
			, distance, currentPort);
			track->attachSignal(s, connection);
			currentPort += lightsCount;

			return Result::OK;
		}

		using Shared_Ptr<SignalController>::Shared;
		using Shared_Ptr<SignalController>::make;
	private:
		virtual const Result updateInternal(winston::Signal::Shared signal);
		virtual const Result flushInternal();

		const std::vector<typename SignalDevice::Shared> devices;
		Id currentDev;
		Port currentPort;
	};
}
