#include "Detector.h"

namespace winston
{
	Detector::Detector(Track::Shared track, const Track::Connection connection, const Distance distance)
		: Shared_Ptr<Detector>(), track(track), connection(connection), distance(distance)
	{

	}
}