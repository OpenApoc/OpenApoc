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

const std::map<ResearchDependency::Type, UString> ResearchDependency::TypeMap = {
    {Type::Any, "any"}, {Type::All, "all"},
};

ResearchTopic::ResearchTopic()
    : man_hours(0), man_hours_progress(0), type(Type::BioChem), required_lab_size(LabSize::Small),
      score(0)
{
}

bool ResearchTopic::isComplete() const { return this->man_hours_progress >= this->man_hours; }

ResearchDependency::ResearchDependency() : type(Type::Any){};

bool ResearchDependency::satisfied() const
{
	if (this->topics.empty())
	{
		// No dependencies == always satisfied
		return true;
	}
	switch (this->type)
	{
		case Type::Any:
		{
			for (auto &r : this->topics)
			{
				if (r->isComplete())
					return true;
			}
			return false;
		}
		case Type::All:
		{
			for (auto &r : this->topics)
			{
				if (!r->isComplete())
					return false;
			}
			return true;
		}
		default:
			LogError("Unexpected ResearchDependency Type");
	}
}

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
