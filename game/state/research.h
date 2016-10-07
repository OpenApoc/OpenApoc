#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <list>
#include <map>
#include <set>

namespace OpenApoc
{

class UfopaediaEntry;
class Base;
class ResearchDependency;
class ItemDependency;
class Agent;
class Lab;
class GameState;

class ProjectDependencies
{
  public:
	ProjectDependencies() = default;
	std::list<ResearchDependency> research;
	std::list<ItemDependency> items;

	bool satisfied(StateRef<Base> base) const;
};

class ResearchTopic : public StateObject<ResearchTopic>
{
  public:
	ResearchTopic() = default;
	enum class Type
	{
		BioChem,
		Physics,
		Engineering,
	};
	static const std::map<Type, UString> TypeMap;
	enum class LabSize
	{
		Small,
		Large,
	};
	static const std::map<LabSize, UString> LabSizeMap;
	enum class ItemType
	{
		VehicleEquipment,
		AgentEquipment,
		VehicleEquipmentAmmo,
		Craft,
	};
	static const std::map<ItemType, UString> ItemTypeMap;

	// Shared Research & Manufacture
	UString name;
	UString description;
	unsigned man_hours = 0;
	Type type = Type::BioChem;
	LabSize required_lab_size = LabSize::Small;
	ProjectDependencies dependencies;
	unsigned order = 0;

	// Research only
	unsigned man_hours_progress = 0;
	// This is the entry that gets shown when you press "Yes" when asked to view
	StateRef<UfopaediaEntry> ufopaedia_entry;
	StateRef<Lab> current_lab;
	unsigned score = 0;
	bool started = false;
	bool isComplete() const;

	// Manufacture only
	int cost = 0;
	ItemType item_type = ItemType::VehicleEquipment;
	UString item_produced;
};

class ResearchDependency
{
  public:
	ResearchDependency() = default;
	enum class Type
	{
		Any,
		All,
		Unused
	};
	static const std::map<Type, UString> TypeMap;
	Type type = Type::Any;

	std::set<StateRef<ResearchTopic>> topics;

	bool satisfied() const;
};

class ItemDependency
{
  public:
	ItemDependency() = default;
	// FIXME: Replace with StateRef<GenericItemClass> or something?
	std::map<UString, int> items;

	bool satisfied(StateRef<Base> base) const;
};

class Lab : public StateObject<Lab>
{
  public:
	Lab() = default;
	ResearchTopic::LabSize size = ResearchTopic::LabSize::Small;
	ResearchTopic::Type type = ResearchTopic::Type::BioChem;
	StateRef<ResearchTopic> current_project;
	std::list<StateRef<Agent>> assigned_agents;

	static void setResearch(StateRef<Lab> lab, StateRef<ResearchTopic> topic, sp<GameState> state);
	static void setQuantity(StateRef<Lab> lab, unsigned quantity);

	static void update(unsigned int ticks, StateRef<Lab> lab, sp<GameState> state);

	int getTotalSkill() const;
	unsigned getQuantity() const;

	// We keep a count of ticks since the last point of progress to accurately accumulate over
	// periods of ticks smaller than what is required to progress a single 'progress' point.
	// This is also used to 'store' the remaining time if the update granularity is such that is
	// overshoots a project's completion.
	unsigned int ticks_since_last_progress = 0;

	unsigned manufacture_goal = 0;
	unsigned manufacture_done = 0;
	unsigned int manufacture_man_hours_invested = 0;
};

class ResearchCompleteData
{
  public:
	StateRef<ResearchTopic> topic;
	StateRef<Lab> lab;
	std::list<StateRef<UfopaediaEntry>> ufopaedia_entries;
};

class ResearchState
{
  public:
	ResearchState() = default;
	unsigned int num_labs_created = 0;
	StateRefMap<ResearchTopic> topics;
	// Is not serialized
	std::list<sp<ResearchTopic>> topic_list;
	void updateTopicList();
	void resortTopicList();
	StateRefMap<Lab> labs;
};

} // namespace OpenApoc
