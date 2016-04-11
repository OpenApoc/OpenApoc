#include "game/state/research.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const std::map<ResearchTopic::Type, UString> ResearchTopic::TypeMap = {
    {Type::BioChem, "biochem"}, {Type::Physics, "physics"},
};

const std::map<ResearchTopic::LabSize, UString> ResearchTopic::LabSizeMap = {
    {LabSize::Small, "small"}, {LabSize::Large, "large"},
};

ResearchTopic::ResearchTopic()
    : man_hours(0), man_hours_progress(0), type(Type::BioChem), required_lab_size(LabSize::Small),
      score(0)
{
}

bool ResearchTopic::isComplete() const { return this->man_hours_progress >= this->man_hours; }

template <>
sp<ResearchTopic> StateObject<ResearchTopic>::get(const GameState &state, const UString &id)
{
	auto it = state.research.find(id);
	if (it == state.research.end())
	{
		LogError("No research topic matching ID \"%s\"", id.c_str());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<ResearchTopic>::getPrefix()
{
	static UString prefix = "RESEARCH_";
	return prefix;
}
template <> const UString &StateObject<ResearchTopic>::getTypeName()
{
	static UString name = "ResearchTopic";
	return name;
}

} // namespace OpenApoc
