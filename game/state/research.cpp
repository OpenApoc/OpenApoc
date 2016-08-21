#include "game/state/research.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/state/base/facility.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "gameevent.h"

namespace OpenApoc
{

const std::map<ResearchTopic::Type, UString> ResearchTopic::TypeMap = {
    {Type::BioChem, "biochem"}, {Type::Physics, "physics"}, {Type::Engineering, "engineering"},
};

const std::map<ResearchTopic::LabSize, UString> ResearchTopic::LabSizeMap = {
    {LabSize::Small, "small"}, {LabSize::Large, "large"},
};

const std::map<ResearchTopic::ItemType, UString> ResearchTopic::ItemTypeMap = {
    {ItemType::VehicleEquipment, "vehicleequipment"},
    {ItemType::AgentEquipment, "agentequipment"},
    {ItemType::VehicleEquipmentAmmo, "vehicleequipmentammo"},
    {ItemType::Craft, "craft"},
};

const std::map<ResearchDependency::Type, UString> ResearchDependency::TypeMap = {
    {Type::Any, "any"}, {Type::All, "all"}, {Type::Unused, "unused"},
};

ResearchTopic::ResearchTopic()
    : man_hours(0), man_hours_progress(0), type(Type::BioChem), required_lab_size(LabSize::Small),
      item_type(ItemType::VehicleEquipment), score(0), started(false), cost(0), order(0)
{
}

bool ResearchTopic::isComplete() const
{
	return (this->type != ResearchTopic::Type::Engineering) &&
	       (this->man_hours_progress >= this->man_hours);
}

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
		case Type::Unused:
			return false;
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

bool ItemDependency::satisfied(StateRef<Base>) const
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
		LogError("No research topic matching ID \"%s\"", id.cStr());
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

Lab::Lab()
    : ticks_since_last_progress(0), manufacture_goal(0), manufacture_done(0),
      manufacture_man_hours_invested(0)
{
}

template <> sp<Lab> StateObject<Lab>::get(const GameState &state, const UString &id)
{
	auto it = state.research.labs.find(id);
	if (it == state.research.labs.end())
	{
		LogError("No lab matching ID \"%s\"", id.cStr());
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

void ResearchState::updateTopicList()
{
	topic_list.clear();
	for (auto &t : topics)
		topic_list.push_back(t.second);
	resortTopicList();
}

void ResearchState::resortTopicList()
{
	topic_list.sort([](sp<ResearchTopic> a, sp<ResearchTopic> b) {
		if (a->isComplete() != b->isComplete())
			return b->isComplete();
		else
			return a->order < b->order;
	});
}

void Lab::setResearch(StateRef<Lab> lab, StateRef<ResearchTopic> topic, sp<GameState> state)
{
	if (topic)
	{
		if (topic->current_lab)
		{
			LogAssert(topic->current_lab->current_project == topic);
			// FIXME: What was the purpose of this? There should be no way to cancel research by
			// selecting it! Commenting for now
			/*
			topic->current_lab->current_project = "";
			topic->current_lab = "";
			*/
			return;
		}
		if (lab->current_project == topic)
			return;
	}
	if (lab->current_project)
	{
		// Refund if haven't started working on the manufacturing topic
		if (lab->type == ResearchTopic::Type::Engineering &&
		    lab->manufacture_man_hours_invested == 0)
			state->player->balance += lab->current_project->cost;
		lab->current_project->current_lab = "";
		lab->manufacture_man_hours_invested = 0;
		lab->manufacture_done = 0;
		lab->manufacture_goal = 0;
	}
	lab->current_project = topic;
	if (topic)
	{
		LogAssert(!(topic->required_lab_size == ResearchTopic::LabSize::Large &&
		            lab->size == ResearchTopic::LabSize::Small));

		switch (lab->type)
		{
			case ResearchTopic::Type::BioChem:
			case ResearchTopic::Type::Physics:
				topic->current_lab = lab;
				break;
			case ResearchTopic::Type::Engineering:
				LogAssert(state->player->balance >= topic->cost);
				state->player->balance -= topic->cost;
				lab->manufacture_goal = 1;
				break;
			default:
				break;
		}
	}
}

unsigned Lab::getQuantity() const { return manufacture_goal - manufacture_done; }

void Lab::setQuantity(StateRef<Lab> lab, unsigned quantity)
{
	if (lab->type != ResearchTopic::Type::Engineering)
		LogError("Cannot set goal for a research lab");
	else
	{
		LogAssert(quantity >= 1 && quantity <= 50);
		lab->manufacture_goal = lab->manufacture_done + quantity;
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

void Lab::update(unsigned int ticks, StateRef<Lab> lab, sp<GameState> state)
{
	if (lab->current_project)
	{
		auto skill = lab->getTotalSkill();
		if (skill <= 0)
		{
			// If there's nobody assigned projects obviously don't progress
			return;
		}
		// A little complication as we want to be correctly calculating progress in an integer when
		// working with sub-single progress 'unit' time units.
		// This also leaves any remaining ticks in the lab's ticks_since_last_progress, so they will
		// get added onto the next project that lab undertakes at the first update.
		unsigned ticks_per_progress_hour = TICKS_PER_HOUR / lab->getTotalSkill();
		unsigned ticks_remaining_to_progress = ticks + lab->ticks_since_last_progress;

		unsigned progress_hours;

		switch (lab->current_project->type)
		{
			case ResearchTopic::Type::Physics:
			case ResearchTopic::Type::BioChem:
				progress_hours = std::min(ticks_remaining_to_progress / ticks_per_progress_hour,
				                          lab->current_project->man_hours -
				                              lab->current_project->man_hours_progress);
				break;
			case ResearchTopic::Type::Engineering:
				progress_hours = std::min(ticks_remaining_to_progress / ticks_per_progress_hour,
				                          lab->current_project->man_hours * lab->getQuantity() -
				                              lab->manufacture_man_hours_invested);
				break;
			default:
				LogError("Unexpected lab type");
		}

		unsigned ticks_left =
		    ticks_remaining_to_progress - progress_hours * ticks_per_progress_hour;
		lab->ticks_since_last_progress = ticks_left;

		switch (lab->current_project->type)
		{
			case ResearchTopic::Type::Physics:
			case ResearchTopic::Type::BioChem:
				lab->current_project->man_hours_progress += progress_hours;
				if (lab->current_project->isComplete())
				{
					auto event = new GameResearchEvent(GameEventType::ResearchCompleted,
					                                   lab->current_project, lab);
					fw().pushEvent(event);
					Lab::setResearch(lab, {state.get(), ""}, state);
				}
				break;
			case ResearchTopic::Type::Engineering:
				lab->manufacture_man_hours_invested += progress_hours;
				if (lab->manufacture_man_hours_invested >= lab->current_project->man_hours)
				{
					// Add item to base
					bool found = false;
					UString item_name;
					for (auto &base : state->player_bases)
					{
						for (auto &facility : base.second->facilities)
						{
							if (facility->type->capacityType == FacilityType::Capacity::Workshop &&
							    facility->lab == lab)
							{
								switch (lab->current_project->item_type)
								{
									case ResearchTopic::ItemType::VehicleEquipment:
									{
										auto item = base.second->inventoryVehicleEquipment.find(
										    lab->current_project->item_produced);
										if (item != base.second->inventoryVehicleEquipment.end())
											item->second++;
										else
											base.second->inventoryVehicleEquipment
											    [lab->current_project->item_produced] = 1;
									}
									break;
									case ResearchTopic::ItemType::VehicleEquipmentAmmo:
									{
										auto item = base.second->inventoryVehicleAmmo.find(
										    lab->current_project->item_produced);
										if (item != base.second->inventoryVehicleAmmo.end())
											item->second++;
										else
											base.second->inventoryVehicleAmmo[lab->current_project
											                                      ->item_produced] =
											    1;
									}
									break;
									case ResearchTopic::ItemType::AgentEquipment:
									{
										auto item = base.second->inventoryAgentEquipment.find(
										    lab->current_project->item_produced);
										if (item != base.second->inventoryAgentEquipment.end())
											item->second++;
										else
											base.second->inventoryAgentEquipment
											    [lab->current_project->item_produced] = 1;
									}
									break;
									case ResearchTopic::ItemType::Craft:
									{
										auto type = state->vehicle_types[lab->current_project
										                                     ->item_produced];

										auto v = mksp<Vehicle>();
										v->type = {state.get(), type};
										v->name = UString::format("%s %d", type->name,
										                          ++type->numCreated);
										v->city = {state.get(), "CITYMAP_HUMAN"};
										v->currentlyLandedBuilding = {state.get(),
										                              base.second->building};
										v->homeBuilding = {state.get(), base.second->building};
										v->owner = state->getPlayer();
										v->health = type->health;
										UString vID = UString::format("%s%d", Vehicle::getPrefix(),
										                              state->lastVehicle++);
										state->vehicles[vID] = v;
										v->currentlyLandedBuilding->landed_vehicles.insert(
										    {state.get(), vID});
										v->equipDefaultEquipment(*state);
									}
									break;
								}
								found = true;
							}
							if (found)
								break;
						}
						if (found)
							break;
					}

					lab->manufacture_done++;

					if (lab->manufacture_done >= lab->manufacture_goal)
					{
						auto event = new GameManufactureEvent(
						    GameEventType::ManufactureCompleted, lab->current_project,
						    lab->manufacture_done, lab->manufacture_goal, lab);
						fw().pushEvent(event);
						Lab::setResearch(lab, {state.get(), ""}, state);
					}
					else
					{
						if (state->player->balance >= lab->current_project->cost)
						{
							state->player->balance -= lab->current_project->cost;
							lab->manufacture_man_hours_invested -= lab->current_project->man_hours;
						}
						else
						{
							auto event = new GameManufactureEvent(
							    GameEventType::ManufactureHalted, lab->current_project,
							    lab->manufacture_done, lab->manufacture_goal, lab);
							fw().pushEvent(event);
							Lab::setResearch(lab, {state.get(), ""}, state);
						}
					}
				}
				break;
			default:
				LogError("Unexpected lab type");
		}
	}
}

} // namespace OpenApoc
