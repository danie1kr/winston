#pragma once

#include <unordered_map>
#include "WinstonTypes.h"
#include "Track.h"

namespace winston {

	using BlockEntry = std::pair<Track::Shared, Track::Connection>;
	using BlockEntrySet = std::set<BlockEntry>;

	class Block : public Shared_Ptr<Block>
	{
	public:
		Block(const Address address, const Trackset tracks);
		//Block::Shared traverse(Track::Shared& entry, Track::Connection& connection);
		//bool validate();

		const bool contains(Track::Shared track) const;
		const BlockEntrySet entries() const;
		const Trackset tracks() const;

		using Shared_Ptr<Block>::Shared;
		using Shared_Ptr<Block>::make;
	private:
		BlockEntrySet blockEntrySet;
		const Trackset _tracks;

		const Address address;
	};

	using Blockset = std::set<Block::Shared>;
	using Blockmap = std::unordered_map<Address, Block::Shared>;
}

