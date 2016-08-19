#pragma once

#include "game/state/stateobject.h"
#include <map>

namespace OpenApoc
{

class UfopaediaEntry;
class Base;
class ResearchDependency;
class ItemDependency;
class Agent;
class Lab;

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
	ResearchTopic();
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
	unsigned man_hours;
	Type type;
	LabSize required_lab_size;
	ProjectDependencies dependencies;

	// Research only
	// FIXME: Some research topics enable multiple ufopaedia entries?
	// FIXME: Is this deprecated?
	StateRef<UfopaediaEntry> ufopaedia_entry;
	unsigned man_hours_progress;
	StateRef<Lab> current_lab;
	unsigned score;
	bool started;
	bool isComplete() const;

	// Manufacture only
	unsigned cost;
	ItemType item_type;
	UString item_produced;
};

class ResearchDependency
{
  public:
	ResearchDependency();
	enum class Type
	{
		Any,
		All,
	};
	static const std::map<Type, UString> TypeMap;
	Type type;

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
	Lab();
	ResearchTopic::LabSize size;
	ResearchTopic::Type type;
	StateRef<ResearchTopic> current_project;
	std::list<StateRef<Agent>> assigned_agents;

	static void setResearch(StateRef<Lab> lab, StateRef<ResearchTopic> topic, sp<GameState> state);
	static void setGoal(StateRef<Lab> lab, unsigned goal);

	static void update(unsigned int ticks, StateRef<Lab> lab, sp<GameState> state);

	int getTotalSkill() const;

	// We keep a count of ticks since the last point of progress to accurately accumulate over
	// periods of ticks smaller than what is required to progress a single 'progress' point.
	// This is also used to 'store' the remaining time if the update granularity is such that is
	// overshoots a project's completion.
	unsigned int ticks_since_last_progress;

	unsigned manufacture_goal;
	unsigned manufacture_done;
	unsigned int manufacture_man_hours_invested;
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
	ResearchState();
	unsigned int num_labs_created;
	std::map<UString, sp<ResearchTopic>> topics;
	std::map<UString, sp<Lab>> labs;
};

} // namespace OpenApoc
