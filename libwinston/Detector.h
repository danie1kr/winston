#pragma once

#include "WinstonTypes.h"
#include "Track.h"
#include "Position.h"

namespace winston
{
	class Detector : public Shared_Ptr<Detector>
	{
	public:
		Detector(Track::Shared track, const Track::Connection connection, const Distance distance);
		const Position &position() const;
	protected:
		const Position pos;
	};

	template<typename T>
	class DetectorAddressable : public Detector, public Shared_Ptr<DetectorAddressable<T>>, public std::enable_shared_from_this<DetectorAddressable<T>>
	{
	public:
		using Callback = std::function<Result(Detector::Shared detector, const T address)>;

		DetectorAddressable(Callback callback, Track::Shared track, const Track::Connection connection, const Distance distance)
			: Detector(track, connection, distance), std::enable_shared_from_this<DetectorAddressable>(), callback(callback)
		{

		}

		const Result detect(T address)
		{
			return this->callback(this->shared_from_this(), address);
		}

		using Shared_Ptr<DetectorAddressable<T>>::Shared;
		using Shared_Ptr<DetectorAddressable<T>>::make;
	private:
		Callback callback;
	};

	template<typename T>
	class DetectorDevice : public Shared_Ptr<DetectorDevice<T>>
	{
	public:
		DetectorDevice(DetectorAddressable<T>& detector) : Shared_Ptr<DetectorDevice<T>>(), detector(detector) {}
	protected:
		DetectorAddressable<T>& detector;
	};
}