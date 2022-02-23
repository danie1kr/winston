#include "Position.h"
#include "Log.h"

namespace winston
{
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

	Position::Transit Position::drive(const Distance distance)
	{
		this->dist += distance;
		auto block = this->track->block();
		if (this->dist >= 0 && this->dist <= (int)this->track->length())
			return Transit::Stay;
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
				Track::Shared onto;
				if (!current->traverse(connection, onto, true))
				{
					logger.err(build("cannot traverse during Position::drive: ", current->name(), " leaving on ", Track::ConnectionToString(connection)));
					return Transit::TraversalError;
				}

				if (this->dist < (int)onto->length())
				{
					connection = onto->whereConnects(current);
					current = onto;
					break;
				}
				else
					this->dist -= onto->length();

				connection = onto->whereConnects(current);
				if(!onto->canTraverse(connection))
					return Transit::TraversalError;
				connection = onto->otherConnection(connection);
				current = onto;
			}
			this->track = current;
			this->reference = connection;
			return this->track->block() == block ? Transit::CrossTrack : Transit::CrossBlock;
		}
	}
}