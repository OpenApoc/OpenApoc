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
      score(0), started(false)
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
			return false;
	}
}

bool ItemDependency::satisfied(StateRef<Base> base) const
{
	// FIXME: Add item tracking
	return true;
}

bool ProjectDependencies::satisfied(StateRef<Base> base) const
{
	for (auto &r : this->research)
	{
		if (r.satisfied() == false)
			return false;
	}
	for (auto &i : this->items)
	{
		if (i.satisfied(base) == false)
			return false;
	}
	return true;
}

template <>
sp<ResearchTopic> StateObject<ResearchTopic>::get(const GameState &state, const UString &id)
{
	auto it = state.research.topics.find(id);
	if (it == state.research.topics.end())
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

ResearchState::ResearchState() {}

} // namespace OpenApoc
