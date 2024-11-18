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
		: track(track), reference(reference), dist(distance)
	{

	}
	const std::string Position::trackName() const
	{
		return this->track->name();
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
		if (other.track == this->track)
		{
			return this->reference == other.reference ? other.dist - this->dist : (other.track->length() - other.dist) - this->dist;
		}
		else
		{
			// check next/prev track
			//auto otherTrack = this->track->on(this->dist > 0 ? this->track->otherConnection(this->reference) : this->reference);
			if (other.track == this->track->on(this->reference))
			{
				return this->dist + abs(other.dist);
			}
			else if (other.track == this->track->on(this->track->otherConnection(this->reference)))
			{
				return this->track->length() - this->dist + abs(other.dist);
			}
		}
		return 0;
	}

	Position::Transit Position::drive(const Distance distance, PassedSignals &passedSignals)
	{
		auto distOnThisTrack = this->dist;
		this->dist += distance;
		auto section = this->track->section();
		if (this->dist >= 0 && this->dist <= (int)this->track->length())
		{
			// only signal and we do not jump tracks
			if (auto signal = this->track->signalFacing(this->reference))
			{
				auto signalDistanceFromPositionView = track->length() - signal->distance();
				if (distOnThisTrack < signalDistanceFromPositionView && this->dist >= signalDistanceFromPositionView)
				{
					passedSignals.push_back(PositionedSignal(this->track, this->reference, signal, signalDistanceFromPositionView));
				}
			}
			return Transit::Stay;
		}
		else
		{
			// negative = leave at reference, else other direction connection
			auto connection = this->dist < 0 ? this->reference : this->track->otherConnection(this->reference);

			// travel the remaining track to the other side - for -distance, it is already done
			if (distance > 0)
				this->dist -= this->track->length();

			this->dist = this->dist < 0 ? -this->dist : this->dist;
			auto current = this->track;
			while (true)
			{
				// track guarding the exit of the track we are about to leave
				if (auto signal = current->signalGuarding(connection))
				{
					auto signalDistanceFromPositionView = track->length() - signal->distance();
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
					if (auto signal = current->signalFacing(connection))
					{
						auto signalDistanceFromPositionView = track->length() - signal->distance();
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
			this->track = current;
			this->reference = connection;
			return this->track->section() == section ? Transit::CrossTrack : Transit::CrossSection;
		}
	}

	const bool Position::nextSignal(PositionedSignal &positionedSignal) const
	{
		return false;
	}
}