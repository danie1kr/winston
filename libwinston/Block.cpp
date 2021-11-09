#include "Block.h"

namespace winston {
	Block::Block(const Address address, const Trackset tracks) : Shared_Ptr<Block>(), address(address), tracks(tracks)
	{
	}

	/*Block::Shared Block::traverse(Track::Shared& entry, Track::Connection& connection)
	{
		if (!this->blockEntrySet.contains(std::make_pair(entry, connection)))
		{
			return nullptr;
		}

		Track::traverse()

		return Block::Shared();
	}*/

	const bool Block::contains(Track::Shared track) const
	{
		return this->tracks.contains(track);
	}

	const BlockEntrySet Block::entries() const
	{
		BlockEntrySet set;
		for (auto& track : this->tracks)
		{
#define CHECK_ENTRY_AND_ADD(connection) { Track::Shared t = track->on(connection); if (!this->tracks.contains(t)) set.insert(std::make_pair(track, connection));}
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
			}
		}
		return set;
	}

	/*bool Block::validate()
	{
		return false;
	}*/
}