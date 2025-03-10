#include "Position.h"
#include "Log.h"

namespace winston
{
	Position::PositionedSignal::PositionedSignal(const Track::Const track,
		const Track::Connection connection,
		const Signal::Shared signal,
		const Distance distance)
		: track(track), connection(connection), signal(signal), distance(distance)
	{

	}

	Position::Position(Track::Const track, const Track::Connection reference, const Distance distance)
		: _track(track), reference(reference), dist(distance)
	{

	}

	const bool Position::operator==(Position const& other) const
	{
		return this->track() == other.track() && this->connection() == other.connection() && this->distance() == other.distance();
	}

	const std::string Position::trackName() const
	{
		return this->_track ? this->_track->name() : "nullptr";
	}

	const unsigned int Position::trackIndex() const
	{
		return this->_track ? this->_track->index : 0;
	}

	Track::Const Position::track() const
	{
		return this->_track;
	}

	const Track::Connection Position::connection() const
	{
		return this->reference;
	}

	const Distance Position::distance() const
	{
		return this->dist;
	}

	const Distance Position::minus(const Position& other) const
	{
		if (other._track == this->_track)
		{
			return this->reference == other.reference ? other.dist - this->dist : (other._track->length() - other.dist) - this->dist;
		}
		else
		{
			// check next/prev track
			//auto otherTrack = this->_track->on(this->dist > 0 ? this->_track->otherConnection(this->reference) : this->reference);
			if (other._track == this->_track->on(this->reference))
			{
				return this->dist + abs(other.dist);
			}
			else if (other._track == this->_track->on(this->_track->otherConnection(this->reference)))
			{
				return this->_track->length() - this->dist + abs(other.dist);
			}
		}
		return 0;
	}

	void Position::collectSignalsInRange(const Distance start, const Distance end, const Track::Const track, const Track::Connection reference, SignalPassedCallback signalPassed)
	{
		if (auto signal = track->signalGuarding(reference))
		{
			if (signal->distance() >= start && signal->distance() <= end)
				signalPassed(track, reference, Signal::Pass::Backside);
		}

		auto otherRef = track->otherConnection(reference);
		if (auto signal = track->signalGuarding(otherRef))
		{
			auto signalDist = track->length() - signal->distance();
			if (signalDist >= start && signalDist <= end)
				signalPassed(track, otherRef, Signal::Pass::Facing);
		}
	}

	void Position::useOtherRef()
	{
		this->dist = this->_track->length() - this->dist;
		this->reference = this->_track->otherConnection(this->reference);
	}

	const Position::Transit Position::drive(const Distance distance, const bool allowCrossSegment, SignalPassedCallback signalPassed)
	{
		TEENSY_CRASHLOG_BREADCRUMB(6, 0x20410);
		if (distance < 0)
			this->useOtherRef();

		auto distOnThisTrack = this->dist;
		this->dist += std::abs(distance);
		auto section = this->_track->section();
		if (this->dist >= 0 && this->dist <= this->_track->length())
		{
			TEENSY_CRASHLOG_BREADCRUMB(6, 0x20411);
			this->collectSignalsInRange(distOnThisTrack, this->dist, this->_track, this->reference, signalPassed);
			return Transit::Stay;
		}
		else
		{
			TEENSY_CRASHLOG_BREADCRUMB(6, 0x20412);
			auto connection = this->reference;
			auto current = this->_track;
			//while (true)
			{
				{
					const size_t __wsex_line = 155; size_t __wsex_it = 0; 
					while (__wsex_it < 1000) 
					{
						if (true) 
						{
							auto onThisTrack = std::min(this->dist, current->length());
							this->collectSignalsInRange(distOnThisTrack, onThisTrack, current, connection, signalPassed);
							if (this->dist < current->length())
								break;
							this->dist -= current->length();
							distOnThisTrack = 0;
							Track::Const onto;
							if (!current->traverse(connection, onto, false)) {
								winston::logger.err(build("cannot traverse during Position::drive: ", current->name(), " leaving on ", Track::ConnectionToString(connection)));
								return Transit::TraversalError;
							};
							if (!allowCrossSegment && current->segment() != onto->segment()) {
								this->dist = current->length();
								this->_track = current;
								this->reference = connection;
								return Transit::SegmentBorder;
							};

							connection = onto->whereConnects(current);
							if (!onto->canTraverse(connection))
								return Transit::TraversalError;

							current = onto;
						}
						else 
							break; 
						++__wsex_it;
					} 
					if (__wsex_it >= 1000) {
						winston::logger.err("there seems to be an infinite loop in: ", "D:\\proj\\dev\\uc\\winston\\winston\\libwinston\\Position.cpp", " at #", __wsex_line);
					}
				};
			};
			TEENSY_CRASHLOG_BREADCRUMB(6, 0x20419);
			this->_track = current;
			this->reference = connection;
			return this->_track->section() == section ? Transit::CrossTrack : Transit::CrossSection;
		}
	}

	const bool Position::nextSignal(PositionedSignal &positionedSignal) const
	{
		return false;
	}

	Position Position::nullPosition()
	{
		return Position(nullptr, Track::Connection::A, 0);
	}

	const bool Position::valid() const
	{
		return this->_track ? true : false;
	}
}