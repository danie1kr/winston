#pragma once

#include "Track.h"
#include "WinstonTypes.h"
#include <unordered_map>

namespace winston {

	using BlockEntry = std::pair<Track::Shared, Track::Connection>;
	using BlockEntrySet = std::set<BlockEntry>;

	class Block : public Shared_Ptr<Block>
	{
	public:
		enum class Type : unsigned char
		{
			Free,      // free trak
			Transit,   // transit track
			Siding,    // park
			Platform,  // in a station
		};

		Block(const Type type, const Trackset tracks);
		//Block::Shared traverse(Track::Shared& entry, Track::Connection& connection);

		using MarkCallback = std::function<const bool(const Track&)>;
		const bool validate(MarkCallback mark) const;

		const bool contains(Track &track) const;
		const BlockEntrySet entries() const;
		const Trackset tracks() const;

		const Type type;

		using Shared_Ptr<Block>::Shared;
		using Shared_Ptr<Block>::make;
	private:
		BlockEntrySet blockEntrySet;
		const Trackset _tracks;
	};
	using Blockset = std::set<Block::Shared>;
}

