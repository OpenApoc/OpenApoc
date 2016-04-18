#include "game/state/research.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const std::map<ResearchTopic::Type, UString> ResearchTopic::TypeMap = {
    {Type::BioChem, "biochem"}, {Type::Physics, "physics"}, {Type::Engineering, "engineering"},
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

template <>
const UString &StateObject<ResearchTopic>::getId(const GameState &state,
                                                 const sp<ResearchTopic> ptr)
{
	static const UString emptyString = "";
	for (auto &r : state.research.topics)
	{
		if (r.second == ptr)
			return r.first;
	}
	LogError("No research matching pointer %p", ptr.get());
	return emptyString;
}

template <> sp<Lab> StateObject<Lab>::get(const GameState &state, const UString &id)
{
	auto it = state.research.labs.find(id);
	if (it == state.research.labs.end())
	{
		LogError("No lab matching ID \"%s\"", id.c_str());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<Lab>::getPrefix()
{
	static UString prefix = "LAB_";
	return prefix;
}
template <> const UString &StateObject<Lab>::getTypeName()
{
	static UString name = "Lab";
	return name;
}

template <> const UString &StateObject<Lab>::getId(const GameState &state, const sp<Lab> ptr)
{
	static const UString emptyString = "";
	for (auto &l : state.research.labs)
	{
		if (l.second == ptr)
			return l.first;
	}
	LogError("No lab matching pointer %p", ptr.get());
	return emptyString;
}

ResearchState::ResearchState() : num_labs_created(0) {}

void Lab::setResearch(StateRef<Lab> lab, StateRef<ResearchTopic> topic)
{
	if (topic)
	{
		if (topic->current_lab)
		{
			assert(topic->current_lab->current_project == topic);
			topic->current_lab->current_project = "";
			topic->current_lab = "";
		}
	}
	if (lab->current_project)
	{
		lab->current_project->current_lab = "";
	}
	lab->current_project = topic;
	if (topic)
	{
		topic->current_lab = lab;
	}
}

int Lab::getTotalSkill() const
{
	int totalLabSkill = 0;
	for (auto &agent : this->assigned_agents)
	{
		switch (this->type)
		{
			case ResearchTopic::Type::Physics:
				totalLabSkill += agent->current_stats.physics_skill;
				break;
			case ResearchTopic::Type::BioChem:
				totalLabSkill += agent->current_stats.biochem_skill;
				break;
			case ResearchTopic::Type::Engineering:
				totalLabSkill += agent->current_stats.engineering_skill;
				break;
			default:
				// TODO Workshop 'labs'?
				LogError("Unexpected lab type");
		}
	}
	return totalLabSkill;
}

} // namespace OpenApoc
