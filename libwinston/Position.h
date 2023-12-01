#pragma once

#include "Track.h"
#include "WinstonTypes.h"
namespace winston
{
	class Position
	{
	public:
		enum class Transit
		{
			Stay = 0,
			CrossTrack = 1,
			CrossSection = 2,
			TraversalError = 3
		};

		Position(Track::Shared track, const Track::Connection reference, const Distance distance);
		const std::string trackName() const;
		const Track::Connection connection() const;
		const Distance distance() const;
		const Distance minus(const Position& other) const;
		Transit drive(const Distance distance);
	private:
		// the track we are on
		Track::Shared track;

		// the connection we use as reference for the distance
		Track::Connection reference;
		Distance dist;
	};
};