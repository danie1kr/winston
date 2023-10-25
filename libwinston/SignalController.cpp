#include "SignalController.h"

namespace winston
{
	SignalController::SignalController(const std::vector<typename SignalDevice::Shared> devices)
		: devices(devices), currentDev{ 0 }, currentPort{ 0 }
	{

	}

	SignalController::~SignalController()
	{
	};

	const Result SignalController::updateInternal(winston::Signal::Shared signal)
	{
		return devices[signal->device]->updateInternal(signal);
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

}