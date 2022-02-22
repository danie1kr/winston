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
			this->dist = this->dist < 0 ? -this->dist : this->dist;
			auto current = this->track;
			while (true)
			{
				auto onto = current;
				if (!track->traverse(connection, onto, true))
				{
					logger.err(build("cannot traverse during Position::drive: ", current->name(), " leaving on ", Track::ConnectionToString(connection)));
					return Transit::TraversalError;
				}

				if (this->dist < (int)onto->length())
					break;
				else
					this->dist -= onto->length();

				connection = onto->whereConnects(current);
				connection = onto->otherConnection(connection);
				current = onto;
			}
			this->track = current;
			return this->track->block() == block ? Transit::CrossTrack : Transit::CrossBlock;
		}
	}
}