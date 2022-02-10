#include "Detector.h"

namespace winston
{
	Detector::Detector(Track::Shared track, const Track::Connection connection, const Distance distance)
		: Shared_Ptr<Detector>(), details{ track, connection, distance }
	{

	}
	const std::string Detector::trackName() const
	{
		return this->details.track->name();
	}

	const Track::Connection Detector::connection() const
	{
		return this->details.connection;
	}

	const Distance Detector::distance() const
	{
		return this->details.distance;
	}
}