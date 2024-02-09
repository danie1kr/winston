#pragma once

#include "Track.h"
#include "WinstonTypes.h"
#include <unordered_map>

namespace winston {

	using SectionEntry = std::pair<Track::Shared, Track::Connection>;
	using SectionEntrySet = std::set<SectionEntry>;

	class Section : public Shared_Ptr<Section>
	{
	public:
		enum class Type : unsigned char
		{
			Free,      // free track
			Transit,   // transit track
			Siding,    // park
			Platform,  // in a station
		};

		Section(const std::string name, const Type type, const TrackSet tracks);

		using MarkCallback = std::function<const bool(const Track&)>;
		const bool validate(MarkCallback mark) const;

		const bool contains(Track &track) const;
		const TrackSet tracks() const;

		const Type type;

		using Shared_Ptr<Section>::Shared;
		using Shared_Ptr<Section>::make;

		const std::string name;
	private:
		const TrackSet _tracks;
		const SectionEntrySet buildEntriesSet() const;

	public:
		const SectionEntrySet entriesSet;
	};
	using SectionSet = std::set<Section::Shared>;
	using SectionList = std::list<Section::Shared>;
}

