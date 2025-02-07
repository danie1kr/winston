#include "Detector.h"

namespace winston
{
	Detector::Detector(const std::string name, Segment::Shared segment, Callbacks::Change changeCallback, Callbacks::Occupied occupiedCallback)
		: Shared_Ptr<Detector>(), name(name), segment(segment), changeCallback(changeCallback), occupiedCallback(occupiedCallback)
	{

	}

	const Result Detector::change(const Locomotive::Shared loco, const bool forward, const TimePoint when, const Change change) const
	{
		return this->changeCallback(this->name, loco, forward, this->segment, change, when);
	}

	const Result Detector::occupied(const TimePoint when, const Change change) const
	{
		return this->occupiedCallback(this->name, this->segment, change, when);
	}

	DetectorDevice::DetectorDevice(const std::string name)
		: Shared_Ptr<DetectorDevice>(), name(name)
	{

	};

	const Result DetectorDevice::initInternal(Callbacks callbacks)
	{
		if (!callbacks.change) return Result::InvalidParameter;
		if (!callbacks.occupied) return Result::InvalidParameter;
		if (!callbacks.locoFromAddress) return Result::InvalidParameter;

		this->callbacks = callbacks;

		return Result::OK;
	}

	const Result DetectorDevice::change(const size_t port, const Address locoAddress, const bool forward, const Detector::Change change)
	{
		if (this->ports.find(port) != this->ports.end())
		{
			auto detector = this->ports.find(port)->second;
			auto loco = this->callbacks.locoFromAddress(locoAddress);

			if (!loco)
			{
				LOG_WARN("DetectorDevice::change for unkown loco: ", locoAddress);
				return Result::NotFound;
			}
				
			return detector->change(loco, forward, hal::now(), change);
		}
		else
		{
			LOG_ERROR("DetectorDevice::change on unkown port: ", port);
			return Result::NotFound;
		}
	}

	const Result DetectorDevice::occupied(const size_t port, const Detector::Change change)
	{
		if (this->ports.find(port) != this->ports.end())
		{
			auto detector = this->ports.find(port)->second;
			return detector->occupied(hal::now(), change);
		}
		else
		{
			LOG_ERROR("DetectorDevice::change on unkown port: ", port);
			return Result::NotFound;
		}
	}
}