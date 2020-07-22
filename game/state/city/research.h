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
class Agent;
class Lab;
class GameState;
class AEquipmentType;
class VEquipmentType;
class Image;

class ItemDependency
{
  public:
	ItemDependency() = default;
	std::map<StateRef<AEquipmentType>, int> agentItemsRequired;
	std::map<StateRef<VEquipmentType>, int> vehicleItemsRequired;
	std::map<StateRef<AEquipmentType>, int> agentItemsConsumed;
	std::map<StateRef<VEquipmentType>, int> vehicleItemsConsumed;

	bool satisfied(StateRef<Base> base) const;
	void consume(StateRef<Base> base);
	void produceRemains(StateRef<Base> base);
};

class ProjectDependencies
{
  public:
	ProjectDependencies() = default;
	std::list<ResearchDependency> research;
	ItemDependency items;

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
	enum class LabSize
	{
		Small,
		Large,
	};
	enum class ItemType
	{
		VehicleEquipment,
		AgentEquipment,
		VehicleEquipmentAmmo,
		Craft,
	};

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
	sp<Image> picture;
	bool started = false;
	bool isComplete() const;
	bool hidden = false;
	void forceComplete();

	// Manufacture only
	int cost = 0;
	ItemType item_type = ItemType::VehicleEquipment;
	UString itemId;
};

class ResearchDependency
{
  public:
	ResearchDependency() = default;
	enum class Type
	{
		Any,
		All,
	};
	Type type = Type::Any;

	std::set<StateRef<ResearchTopic>> topics;

	bool satisfied() const;
};

class Lab : public StateObject<Lab>
{
  public:
	Lab() = default;
	~Lab() override;
	ResearchTopic::LabSize size = ResearchTopic::LabSize::Small;
	ResearchTopic::Type type = ResearchTopic::Type::BioChem;
	StateRef<ResearchTopic> current_project;
	std::list<StateRef<Agent>> assigned_agents;

	static void setResearch(StateRef<Lab> lab, StateRef<ResearchTopic> topic, sp<GameState> state);
	static void setQuantity(StateRef<Lab> lab, unsigned quantity);

	static void update(unsigned int ticks, StateRef<Lab> lab, sp<GameState> state);

	int getTotalSkill() const;
	unsigned getQuantity() const;
	static void removeAgent(StateRef<Lab> lab, StateRef<Agent> &agent);

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
	StateRefMap<ResearchTopic> topics;
	// Is not serialized
	std::list<sp<ResearchTopic>> topic_list;
	void updateTopicList();
	void resortTopicList();
	StateRefMap<Lab> labs;

	// Is the research of the item finished?
	template <class T> bool isComplete(StateRef<T> item) const
	{
		size_t prefLen = item->getPrefix().length();
		UString researchId(ResearchTopic::getPrefix() + item->id.substr(prefLen));

		auto it = topics.find(researchId);
		return it == topics.end() ? true : it->second->isComplete();
	}
};

} // namespace OpenApoc
