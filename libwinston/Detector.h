#pragma once

#include "WinstonTypes.h"
#include "Track.h"
#include "Position.h"
#include "Locomotive.h"
#include "Segment.h"

namespace winston
{
	class Detector : public Shared_Ptr<Detector>
	{
	public:
		enum class Change: uint8_t
		{
		    Entered = 0,
			Left = 1
		};

		struct Callbacks
		{
			using Change = std::function<const Result(const std::string detectorName, const Locomotive::Shared loco, Segment::Shared segment, const Change change, const TimePoint when)>;
			using Occupied = std::function<const Result(const std::string detectorName, Segment::Shared segment, const Change change)>;
		};
		
		Detector(const std::string name, Segment::Shared segment, Callbacks::Change changeCallback);
		virtual ~Detector() = default;

		const std::string name;

	protected:
		const Segment::Shared segment;
		const Callbacks::Change changeCallback;

	friend class DetectorDevice;
		const Result change(const Locomotive::Shared loco, const TimePoint when, const Change change) const;
		const Result occupied(const Locomotive::Shared loco, const TimePoint when, const Change change) const;
	};

	class DetectorDevice : public Shared_Ptr<DetectorDevice>
	{
	public:

		struct Callbacks
		{
			using LocoFromAddress = std::function<Locomotive::Shared(const Address address)>;
		};

		DetectorDevice(const std::string name, Callbacks::LocoFromAddress locofromAddressCallback);

		using PortSegmentMap = std::map<size_t, Segment::Shared>;
		using PortDetectorMap = std::map<size_t, winston::Detector::Shared>;
		
		virtual ~DetectorDevice() = default;
		virtual const Result init(PortSegmentMap ports, Detector::Callbacks::Change changeCallback) = 0;
		const std::string name;

	protected:
		PortDetectorMap ports;

		const Result change(const size_t port, const Address locoAddress, const Detector::Change change);
		const Result occupied(const size_t port, const Detector::Change change);

	private:
		Callbacks::LocoFromAddress locofromAddressCallback;
	};
}