#include "Position.h"
#include "Log.h"

namespace winston
{
	Position::PositionedSignal::PositionedSignal(const Track::Shared track,
		const Track::Connection connection,
		const Signal::Shared signal,
		const Distance distance)
		: track(track), connection(connection), signal(signal), distance(distance)
	{

	}

	Position::Position(Track::Shared track, const Track::Connection reference, const Distance distance)
		: _track(track), reference(reference), dist(distance)
	{

	}

	const std::string Position::trackName() const
	{
		return this->_track->name();
	}

	const unsigned int Position::trackIndex() const
	{
		return this->_track->index;
	}

	Track::Shared Position::track() const
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

	Position::Transit Position::drive(const Distance distance, PassedSignals &passedSignals)
	{
		auto distOnThisTrack = this->dist;
		this->dist += distance;
		auto section = this->_track->section();
		if (this->dist >= 0 && this->dist <= (int)this->_track->length())
		{
			// only signal and we do not jump tracks
			if (auto signal = this->_track->signalFacing(this->reference))
			{
				auto signalDistanceFromPositionView = std::abs((signed)this->_track->length() - (signed)signal->distance());
				if (distOnThisTrack < signalDistanceFromPositionView && this->dist >= signalDistanceFromPositionView)
				{
					passedSignals.push_back(PositionedSignal(this->_track, this->reference, signal, signalDistanceFromPositionView));
				}
			}
			return Transit::Stay;
		}
		else
		{
			// negative = leave at reference, else other direction connection
			auto connection = this->dist < 0 ? this->reference : this->_track->otherConnection(this->reference);

			// travel the remaining track to the other side - for -distance, it is already done
			if (distance > 0)
				this->dist -= this->_track->length();

			this->dist = this->dist < 0 ? -this->dist : this->dist;
			auto current = this->_track;
			while (true)
			{
				// track guarding the exit of the track we are about to leave
				if (auto signal = current->signalGuarding(connection))
				{
					auto signalDistanceFromPositionView = this->_track->length() - signal->distance();
					if (distOnThisTrack < signalDistanceFromPositionView)// && this->dist >= signalDistanceFromPositionView)
					{
						passedSignals.push_back(PositionedSignal(current, connection, signal, signalDistanceFromPositionView));
					}
				}
				Track::Shared onto;

				if (!current->traverse(connection, onto, true))
				{
					logger.err(build("cannot traverse during Position::drive: ", current->name(), " leaving on ", Track::ConnectionToString(connection)));
					return Transit::TraversalError;
				}

				if (this->dist < (int)onto->length())
				{
					distOnThisTrack = this->dist;
					connection = onto->whereConnects(current);
					current = onto;

					// we travelled all the way to a new track which might have a signal we just passed
					if (auto signal = current->signalGuarding(connection))
					{
						auto signalDistanceFromPositionView = std::abs((signed)this->_track->length() - (signed)signal->distance());
						if (distOnThisTrack < signalDistanceFromPositionView && this->dist >= signalDistanceFromPositionView)
						{
							passedSignals.push_back(PositionedSignal(current, connection, signal, signalDistanceFromPositionView));
						}
					}

					break;
				}
				else
				{
					this->dist -= onto->length();
					distOnThisTrack = 0;
				}

				connection = onto->whereConnects(current);
				if(!onto->canTraverse(connection))
					return Transit::TraversalError;
				connection = onto->otherConnection(connection);
				current = onto;
			}
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