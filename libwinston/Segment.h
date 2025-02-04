#pragma once

#include "Track.h"
#include "WinstonTypes.h"
#include "Log.h"
#include <unordered_map>

namespace winston {

	using SegmentEntry = std::pair<Track::Const, Track::Connection>;
	using SegmentEntrySet = std::set<SegmentEntry>;

	template<class _Identifier>
	class BaseSegment
	{
	public:
		using IdentifyerType = _Identifier;

		BaseSegment(const IdentifyerType id, const TrackSet tracks)
			: id{ id }, _tracks(tracks), entriesSet(this->buildEntriesSet()), _length(this->calculateLength())
		{
		}

		using MarkCallback = std::function<const bool(const Track&)>;
		const bool validate(MarkCallback mark) const
		{
			if (this->_tracks.size() == 0)
				return true;

			// check that all elements of the track set are reached
			auto current = *this->_tracks.begin();
			std::list<Track::Shared> tracks;
			tracks.push_back(current);

			for (auto it = tracks.begin(); it != tracks.end(); ++it)
			{
				current = *it;
				if (mark(*current))
				{
					TrackSet others;
					current->collectAllConnections(others);

					for (auto other : others)
					{
						if (this->_tracks.find(other) != this->_tracks.end() &&
							std::find(tracks.begin(), tracks.end(), other) == tracks.end())
						{
							tracks.push_back(other);
						}
					}
				}
			}

			// size must match
			if (this->_tracks.size() != tracks.size())
				return false;

			// content of tracks must be in _tracks
			return std::is_permutation(tracks.begin(), tracks.end(), this->_tracks.begin());
		}

		const bool contains(const Track& track) const
		{
			return std::find_if(this->_tracks.begin(), this->_tracks.end(), [&track](const Track::Shared& t) { return &track == t.get(); }) != this->_tracks.end();
		}

		const TrackSet tracks() const
		{
			return this->_tracks;
		}

		const bool from(const BaseSegment<_Identifier> &other, SegmentEntry &entry) const
		{
			for (auto const &[track, connection] : this->entriesSet)
			{
				for (auto const& [trackOther, connectionOther] : other.entriesSet)
				{
#pragma message("optimize with better track functions")
					if (track->whereConnects(trackOther) == connection && trackOther->whereConnects(track) == connectionOther)
					{
						entry = std::make_pair(track, connection);
						return true;
					}
				}
			}
			return false;
		}

		const IdentifyerType id;
	protected:
		const TrackSet _tracks;
		const SegmentEntrySet buildEntriesSet() const
		{
			SegmentEntrySet set;
			for (auto& track : this->_tracks)
			{
#define CHECK_ENTRY_AND_ADD(connection) { Track::Shared t = track->on(connection); if (this->_tracks.find(t) == this->_tracks.end()) set.insert(std::make_pair(track, connection));}
				switch (track->type())
				{
					// a bumper is always an entry
				case Track::Type::Bumper:
					set.insert(std::make_pair(track, Track::Connection::A));
					break;
				case Track::Type::Rail:
					CHECK_ENTRY_AND_ADD(Track::Connection::A);
					CHECK_ENTRY_AND_ADD(Track::Connection::B);
					break;
				case Track::Type::Turnout:
					CHECK_ENTRY_AND_ADD(Track::Connection::A);
					CHECK_ENTRY_AND_ADD(Track::Connection::B);
					CHECK_ENTRY_AND_ADD(Track::Connection::C);
					break;
				case Track::Type::DoubleSlipTurnout:
					CHECK_ENTRY_AND_ADD(Track::Connection::A);
					CHECK_ENTRY_AND_ADD(Track::Connection::B);
					CHECK_ENTRY_AND_ADD(Track::Connection::C);
					CHECK_ENTRY_AND_ADD(Track::Connection::D);
					break;
				}
			}
			return set;
		}
#undef CHECK_ENTRY_AND_ADD

	private:
		const Length calculateLength() const
		{
			Length l = 0.f;
			for (auto& track : this->_tracks)
			{
				switch (track->type())
				{
				case Track::Type::Bumper:
				case Track::Type::Rail:
				{
					l += track->length();
					break;
				}
				case Track::Type::Turnout:
				case Track::Type::DoubleSlipTurnout:
					return 0.f;
				default:
				{
					logger.err("Track type not supported in BaseSegment::calculateLength");
					return 0.f;
				}
				}
			}
			return l;
		}

	public:
		const SegmentEntrySet entriesSet;
	
		const Length length(const Track::Const track, const Track::Connection entered) const
		{
			if (this->_length)
				return this->_length;
			else
			{
				Length len = 0;
				auto current = track;
				auto connection = entered;
				//while (current->segment() == this->id)
				WHILE_SAFE(current->segment() == this->id,
				{
					len += current->length();
					auto onto = current;
					if (!current->traverse(connection, onto, false))
						return 0.f;
					connection = onto->whereConnects(current);
					current = onto;
				});

				return len;
			}
		}
	private:
		const Length _length;

	};

	class Segment : public BaseSegment<Id>, public Shared_Ptr<Segment>
	{
	public:
		Segment(const IdentifyerType id, const TrackSet tracks);
		virtual ~Segment() = default;
		using Shared_Ptr<Segment>::Shared;
		using Shared_Ptr<Segment>::make;
	};

	using SegmentSet = std::set<Segment::Shared>;
	using SegmentList = std::list<Segment::Shared>;
}

