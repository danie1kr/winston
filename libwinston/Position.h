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
			TraversalError = 3,
			SegmentBorder = 4
		};
		
		using SignalPassedCallback = std::function<const Result(const winston::Track::Const track, const winston::Track::Connection connection, const Signal::Pass pass)>;

		struct PositionedSignal
		{
			const Track::Const track;
			const Track::Connection connection;
			const Signal::Shared signal;
			const Distance distance;

			PositionedSignal(const Track::Const track,
				const Track::Connection connection,
				const Signal::Shared signal,
				const Distance distance);
			~PositionedSignal() = default;
		};
		using PassedSignals = std::list<PositionedSignal>;

		Position(Track::Const track, const Track::Connection reference, const Distance distance);
		~Position() = default;

		const bool operator==(Position const& other) const;

		const std::string trackName() const;
		const unsigned int trackIndex() const;
		const Track::Connection connection() const;
		const Distance distance() const;
		const Distance minus(const Position& other) const;
		const Transit drive(const Distance distance, const bool allowCrossSegment, SignalPassedCallback signalPassed);
		const bool nextSignal(PositionedSignal &positionedSignal) const;
		static void collectSignalsInRange(const Distance start, const Distance end, const Track::Const track, const Track::Connection reference, SignalPassedCallback signalPassed);

		static Position nullPosition();
		const bool valid() const;

		Track::Const track() const;
		void useOtherRef();

	private:

		// the track we are on
		Track::Const _track;

		// the connection we use as reference for the distance
		Track::Connection reference;
		Distance dist;
	};
};