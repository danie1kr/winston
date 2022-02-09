#pragma once

#include "WinstonTypes.h"
#include "Track.h"

namespace winston
{
	class Detector : public Shared_Ptr<Detector>
	{
	public:
		Detector(Track::Shared track, const Track::Connection connection, const Distance distance);
	protected:
		Track::Shared track;
		const Track::Connection connection;
		const Distance distance;
	};

	template<typename T>
	class DetectorAddressable : public Detector, public Shared_Ptr<DetectorAddressable<T>>
	{
	public:
		using Callback = std::function<void(Track::Shared track, const Track::Connection connection, const Distance distance, const T address)>;

		DetectorAddressable(Callback callback, Track::Shared track, const Track::Connection connection, const Distance distance)
			: Detector(track, connection, distance), callback(callback)
		{

		}
		using Shared_Ptr<DetectorAddressable<T>>::Shared;
		using Shared_Ptr<DetectorAddressable<T>>::make;
	private:
		Callback callback;
	};
}