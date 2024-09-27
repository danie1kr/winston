#pragma once

#include "Segment.h"
#include "WinstonTypes.h"
#include <unordered_map>

namespace winston {

	using SectionEntry = std::pair<Track::Shared, Track::Connection>;
	using SectionEntrySet = std::set<SectionEntry>;

	class Section : public BaseSegment<std::string>, public Shared_Ptr<Section>
	{
	public:
		enum class Type : unsigned char
		{
			Free,      // free track
			Transit,   // transit track
			Siding,    // park
			Platform,  // in a station
		};

		Section(const Section::BaseSegment::IdentifyerType name, const Type type, const TrackSet tracks);
		virtual ~Section() = default;

		const Type type;

		using Shared_Ptr<Section>::Shared;
		using Shared_Ptr<Section>::make;
	};
	using SectionSet = std::set<Section::Shared>;
	using SectionList = std::list<Section::Shared>;
}

