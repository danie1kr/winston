#include "Detector.h"

namespace winston
{
	Detector::Detector(Track::Shared track, const Track::Connection connection, const Distance distance)
		: Shared_Ptr<Detector>(), pos( track, connection, distance )
	{

	}
	const Position& Detector::position() const
	{
		return this->pos;
	}
}