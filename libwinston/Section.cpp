#include "Section.h"
#include <list>

namespace winston {
	Section::Section(const Section::BaseSegment::IdentifyerType name, const Type type, const TrackSet tracks) :
		BaseSegment<std::string>(name, tracks), Shared_Ptr<Section>(), type(type)
	{
	}
}