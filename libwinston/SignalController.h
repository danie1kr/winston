#pragma once

#include "Signal.h"
#include "Track.h"
#include "Railway.h"

namespace winston
{
	class SignalController : public SignalDevice, Shared_Ptr<SignalController>
	{
	private:
		class SignalOfDevice : public Shared_Ptr<SignalOfDevice>
		{
		public:
			SignalOfDevice(SignalDevice::Shared device);
			virtual ~SignalOfDevice();
			SignalDevice::Shared device;
		};
	protected:
		template<typename _Signal>
		class Signal : public _Signal, public SignalOfDevice, public Shared_Ptr<Signal<_Signal>>
		{
		public:
			Signal(SignalDevice::Shared device, const winston::Signal::Callback callback = winston::Signal::defaultCallback(), const Length distance = 0, const Port port = 0) :
				_Signal(device->id, callback, distance, port), SignalOfDevice(device) {

			}

			using Shared_Ptr<Signal<_Signal>>::Shared;
			using Shared_Ptr<Signal<_Signal>>::make;
		};

	public:
		SignalController(const Id id, const std::vector<typename SignalDevice::Shared> devices);
		virtual ~SignalController();

		template<class _Signal>
		const Result attach(winston::Track::Shared track, const winston::Track::Connection connection, winston::Distance distance, const winston::Railway::Callbacks::SignalUpdateCallback& signalUpdateCallback)
		{
			constexpr auto lightsCount = (unsigned int)_Signal::lightsCount();

			if (currentPort + lightsCount > devices[currentDev]->ports)
			{
				currentDev++;
				currentPort = 0;
			}
			/*
			// alignment on 24 ports of TLC5947
			else if (currentPort / devices[currentDev]->ports != (currentPort + lightsCount) / devices[currentDev]->ports)
				currentPort = 24;
			if (currentDev == 1 && currentPort > 24 && currentPort % 3 > 0)
			{
				currentPort += (3-currentPort % 3);
			}*/

			if (currentDev > devices.size())
			{
				LOG_ERROR("Not enough devices for all signals");
				return Result::OutOfBounds;
			}
			auto s = Signal<_Signal>::make(devices[currentDev],
				[track, connection, signalUpdateCallback](const winston::Signal::Aspects aspect)->const winston::State {
					return signalUpdateCallback(*track, connection, aspect);
				}
			, distance, currentPort);
			track->attachSignal(s, connection);
			currentPort += lightsCount;

			return Result::OK;
		}

		using Shared_Ptr<SignalController>::Shared;
		using Shared_Ptr<SignalController>::make;
	private:
		virtual const Result updateInternal(const winston::Signal &signal);
		virtual const Result flushInternal();

		const size_t countPorts(const std::vector<typename SignalDevice::Shared> devices) const;

		const std::vector<typename SignalDevice::Shared> devices;
		Id currentDev;
		Port currentPort;
	};

	template<>
	const Result SignalController::attach<winston::SignalAlwaysHalt>(winston::Track::Shared track, const winston::Track::Connection connection, winston::Distance distance, const winston::Railway::Callbacks::SignalUpdateCallback& signalUpdateCallback);

}
