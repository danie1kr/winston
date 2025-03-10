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
			using Change = std::function<const Result(const std::string detectorName, const Locomotive::Shared loco, const bool forward, Segment::Shared segment, const Detector::Change change, const TimePoint when)>;
			using Occupied = std::function<const Result(const std::string detectorName, Segment::Shared segment, const Detector::Change change, const TimePoint when)>;
		};
		
		Detector(const std::string name, Segment::Shared segment, Callbacks::Change changeCallback, Callbacks::Occupied occupiedCallback);
		virtual ~Detector() = default;

		const std::string name;

	protected:
		const Segment::Shared segment;
		const Callbacks::Change changeCallback;
		const Callbacks::Occupied occupiedCallback;

	friend class DetectorDevice;
		const Result change(const Locomotive::Shared loco, const bool forward, const TimePoint when, const Change change) const;
		const Result occupied(const TimePoint when, const Change change) const;
	};

	class DetectorDevice : public Shared_Ptr<DetectorDevice>
	{
	public:

		struct Callbacks : public Detector::Callbacks
		{
			using LocoFromAddress = std::function<Locomotive::Shared(const Address address)>;

			LocoFromAddress locoFromAddress;
			Change change;
			Occupied occupied;
		};

		DetectorDevice(const std::string name);

		using PortSegmentMap = std::map<size_t, Segment::Shared>;
		using PortDetectorMap = std::map<size_t, winston::Detector::Shared>;

		using winston::Shared_Ptr<DetectorDevice>::Shared;
		using winston::Shared_Ptr<DetectorDevice>::make;
		
		virtual ~DetectorDevice() = default;
		virtual const Result init(PortSegmentMap ports, Callbacks callbacks) = 0;
		virtual const bool isReady() = 0;
		const std::string name;

		virtual const Result change(const size_t port, const Address locoAddress, const bool forward, const Detector::Change change);
		virtual const Result occupied(const size_t port, const Detector::Change change);

	protected:
		PortDetectorMap ports;
		Callbacks callbacks;

		const Result initInternal(Callbacks callbacks);
	};
}