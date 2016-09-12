#include "game/state/agent.h"
#include "game/state/battle/aequipment.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

template <> sp<AgentType> StateObject<AgentType>::get(const GameState &state, const UString &id)
{
	auto it = state.agent_types.find(id);
	if (it == state.agent_types.end())
	{
		LogError("No agent_type matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<AgentType>::getPrefix()
{
	static UString prefix = "AGENTTYPE_";
	return prefix;
}
template <> const UString &StateObject<AgentType>::getTypeName()
{
	static UString name = "AgentType";
	return name;
}
template <>
const UString &StateObject<AgentType>::getId(const GameState &state, const sp<AgentType> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agent_types)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No agent_type matching pointer %p", ptr.get());
	return emptyString;
}

template <> sp<Agent> StateObject<Agent>::get(const GameState &state, const UString &id)
{
	auto it = state.agents.find(id);
	if (it == state.agents.end())
	{
		LogError("No agent matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<Agent>::getPrefix()
{
	static UString prefix = "AGENT_";
	return prefix;
}
template <> const UString &StateObject<Agent>::getTypeName()
{
	static UString name = "Agent";
	return name;
}
template <> const UString &StateObject<Agent>::getId(const GameState &state, const sp<Agent> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agents)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No agent matching pointer %p", ptr.get());
	return emptyString;
}

StateRef<Agent> AgentGenerator::createAgent(GameState &state, AgentType::Role role) const
{
	std::list<sp<AgentType>> types;
	for (auto &t : state.agent_types)
		if (t.second->role == role && t.second->playable)
			types.insert(types.begin(), t.second);
	auto type = listRandomiser(state.rng, types);

	return createAgent(state, {&state, AgentType::getId(state, type)});
}

StateRef<Agent> AgentGenerator::createAgent(GameState &state, StateRef<AgentType> type) const
{
	UString ID = UString::format("%s%u", Agent::getPrefix().cStr(), this->num_created);

	auto agent = mksp<Agent>();

	agent->type = type;
	agent->gender = probabilityMapRandomizer(state.rng, type->gender_chance);

	auto firstNameList = this->first_names.find(agent->gender);
	if (firstNameList == this->first_names.end())
	{
		LogError("No first name list for gender");
		return nullptr;
	}

	auto firstName = listRandomiser(state.rng, firstNameList->second);
	auto secondName = listRandomiser(state.rng, this->second_names);
	agent->name = UString::format("%s %s", firstName, secondName);

	agent->portrait =
	    randBoundsInclusive(state.rng, 0, (int)type->portraits[agent->gender].size() - 1);

	AgentStats s;
	s.health = randBoundsInclusive(state.rng, type->min_stats.health, type->max_stats.health);
	s.accuracy = randBoundsInclusive(state.rng, type->min_stats.accuracy, type->max_stats.accuracy);
	s.reactions =
	    randBoundsInclusive(state.rng, type->min_stats.reactions, type->max_stats.reactions);
	s.speed = randBoundsInclusive(state.rng, type->min_stats.speed, type->max_stats.speed);
	s.stamina = randBoundsInclusive(state.rng, type->min_stats.stamina, type->max_stats.stamina);
	s.bravery = randBoundsInclusive(state.rng, type->min_stats.bravery, type->max_stats.bravery);
	s.strength = randBoundsInclusive(state.rng, type->min_stats.strength, type->max_stats.strength);
	s.psi_energy =
	    randBoundsInclusive(state.rng, type->min_stats.psi_energy, type->max_stats.psi_energy);
	s.psi_attack =
	    randBoundsInclusive(state.rng, type->min_stats.psi_attack, type->max_stats.psi_attack);
	s.psi_defence =
	    randBoundsInclusive(state.rng, type->min_stats.psi_defence, type->max_stats.psi_defence);
	s.physics_skill = randBoundsInclusive(state.rng, type->min_stats.physics_skill,
	                                      type->max_stats.physics_skill);
	s.biochem_skill = randBoundsInclusive(state.rng, type->min_stats.biochem_skill,
	                                      type->max_stats.biochem_skill);
	s.engineering_skill = randBoundsInclusive(state.rng, type->min_stats.engineering_skill,
	                                          type->max_stats.engineering_skill);

	agent->initial_stats = s;
	agent->current_stats = s;

	// FIXME: Default equipment for agents with no inventory?
	// FIXME: Equip units according to score?

	// Everything worked, add agent to stats
	this->num_created++;
	state.agents[ID] = agent;
	return {&state, ID};
}

bool Agent::canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> type) const
{
	Vec2<int> slotOrigin;
	bool slotFound = false;
	// Check the slot this occupies hasn't already got something there
	for (auto &slot : this->type->equipment_layout_slots)
	{
		if (slot.bounds.within(pos))
		{
			slotOrigin = slot.bounds.p0;
			slotFound = true;
			break;
		}
	}
	// If this was not within a slot fail
	if (!slotFound)
	{
		return false;
	}
	// Check that the equipment doesn't overlap with any other and doesn't
	// go outside a slot of the correct type
	Rect<int> bounds{pos, pos + type->equipscreen_size};
	for (auto &otherEquipment : this->equipment)
	{
		// Something is already in that slot, fail
		if (otherEquipment->equippedPosition == slotOrigin)
		{
			return false;
		}
		Rect<int> otherBounds{otherEquipment->equippedPosition,
		                      otherEquipment->equippedPosition +
		                          otherEquipment->type->equipscreen_size};
		if (otherBounds.intersects(bounds))
		{
			LogInfo("Equipping \"%s\" on \"%s\" at {%d,%d} failed: Intersects with other equipment",
			        type->name.cStr(), this->name.cStr(), pos.x, pos.y);
			return false;
		}
	}

	return true;
}

void Agent::addEquipment(GameState &state, Vec2<int> pos, StateRef<AEquipmentType> type)
{
	if (!this->canAddEquipment(pos, type))
	{
		LogError("Trying to add \"%s\" at {%d,%d} on agent \"%s\" failed", type.id.cStr(), pos.x,
		         pos.y, this->name.cStr());
	}
	auto equipment = mksp<AEquipment>();
	equipment->type = type;
	equipment->ammo = type->max_ammo;
	this->addEquipment(state, pos, equipment);
}

void Agent::addEquipment(GameState &state, Vec2<int> pos, sp<AEquipment> object)
{
	if (!this->canAddEquipment(pos, object->type))
	{
		LogError("Trying to add \"%s\" at {%d,%d} on agent  \"%s\" failed", object->type.id.cStr(),
		         pos.x, pos.y, this->name.cStr());
	}

	LogInfo("Equipped \"%s\" with equipment \"%s\"", this->name.cStr(), object->type->name.cStr());
	object->equippedPosition = pos;
	this->equipment.emplace_back(object);
}

void Agent::removeEquipment(sp<AEquipment> object) { this->equipment.remove(object); }

} // namespace OpenApoc
