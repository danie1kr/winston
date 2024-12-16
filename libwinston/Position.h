#pragma once

#include "Track.h"
#include "Signal.h"
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

		struct PositionedSignal
		{
			const Track::Shared track;
			const Track::Connection connection;
			const Signal::Shared signal;
			const Distance distance;

			PositionedSignal(const Track::Shared track,
				const Track::Connection connection,
				const Signal::Shared signal,
				const Distance distance);
			~PositionedSignal() = default;
		};
		using PassedSignals = std::list<PositionedSignal>;

		Position(Track::Shared track, const Track::Connection reference, const Distance distance);
		~Position() = default;
		const std::string trackName() const;
		const unsigned int trackIndex() const;
		const Track::Connection connection() const;
		const Distance distance() const;
		const Distance minus(const Position& other) const;
		Transit drive(const Distance distance, PassedSignals &passedSignals);
		const bool nextSignal(PositionedSignal &positionedSignal) const;

		static Position nullPosition();
		const bool valid() const;

		Track::Shared track() const;

	private:
		// the track we are on
		Track::Shared _track;

		// the connection we use as reference for the distance
		Track::Connection reference;
		Distance dist;
	};
};