#include <algorithm>

#include "SignalController.h"

namespace winston
{

	SignalController::SignalOfDevice::SignalOfDevice(SignalDevice::Shared device) :
		Shared_Ptr<SignalOfDevice>(), device(device) {

	}

	SignalController::SignalOfDevice::~SignalOfDevice() { }

	SignalController::SignalController(const Id id, const std::vector<typename SignalDevice::Shared> devices)
		: SignalDevice(id, this->countPorts(devices)), devices(devices), currentDev{ 0 }, currentPort{ 0 }
	{

	}

	SignalController::~SignalController()
	{
	};

	const size_t SignalController::countPorts(const std::vector<typename SignalDevice::Shared> devices) const
	{
		size_t count = 0;
		for (const auto& device : devices)
			count += device->ports;

		return count;
	}

	const Result SignalController::updateInternal(const winston::Signal& signal)
	{
		const auto s = dynamic_cast<const SignalController::SignalOfDevice*>(&signal);
		if (s)
		{
			return s->device->updateInternal(signal);
		}
		else
		{
			winston::logger.err("unknown device for signal");
			return Result::InvalidParameter;
		}
	}

	const Result SignalController::flushInternal()
	{
		for (const auto& dev : this->devices)
		{
			auto r = dev->flushInternal();
			if (r != Result::OK)
				return r;
		}

		return Result::OK;
	}

	template<>
	const Result SignalController::attach<winston::SignalAlwaysHalt>(winston::Track::Shared track, const winston::Track::Connection connection, winston::Distance distance, const winston::Railway::Callbacks::SignalUpdateCallback& signalUpdateCallback)
	{
		auto s = Signal<SignalAlwaysHalt>::make(devices[1],
			[track, connection, signalUpdateCallback](const winston::Signal::Aspects aspect)->const winston::State {
				return signalUpdateCallback(*track, connection, aspect);
			}
		, distance, 47);
		track->attachSignal(s, connection);

		return Result::OK;
	}
}