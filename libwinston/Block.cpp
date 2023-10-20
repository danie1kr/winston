#include "Block.h"

namespace winston {
	Block::Block(const Address address, const Type type, const Trackset tracks) : Shared_Ptr<Block>(), type(type), address(address), blockEntrySet(), _tracks(tracks)
	{
	}

	const bool Block::contains(Track::Shared track) const
	{
		return this->_tracks.find(track) != this->_tracks.end();
	}

	const BlockEntrySet Block::entries() const
	{
		BlockEntrySet set;
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

	const Trackset Block::tracks() const
	{
		return this->_tracks;
	}

	const bool Block::isType(const Type type) const
	{
		return this->type == type;
	}
}