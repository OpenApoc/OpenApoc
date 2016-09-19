#include "game/state/agent.h"
#include "game/state/organisation.h"
#include "game/state/aequipment.h"
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

StateRef<Agent> AgentGenerator::createAgent(GameState &state, StateRef<Organisation> org, AgentType::Role role) const
{
	std::list<sp<AgentType>> types;
	for (auto &t : state.agent_types)
		if (t.second->role == role && t.second->playable)
			types.insert(types.begin(), t.second);
	auto type = listRandomiser(state.rng, types);

	return createAgent(state, org, {&state, AgentType::getId(state, type)});
}

StateRef<Agent> AgentGenerator::createAgent(GameState &state, StateRef<Organisation> org, StateRef<AgentType> type) const
{
	UString ID = UString::format("%s%u", Agent::getPrefix().cStr(), this->num_created);

	auto agent = mksp<Agent>();

	agent->owner = org;
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

	agent->appearance = randBoundsExclusive(state.rng, 0, type->appearance_count);

	agent->portrait =
	    randBoundsInclusive(state.rng, 0, (int)type->portraits[agent->gender].size() - 1);

	AgentStats s;
	s.health = randBoundsInclusive(state.rng, type->min_stats.health, type->max_stats.health);
	s.accuracy = randBoundsInclusive(state.rng, type->min_stats.accuracy, type->max_stats.accuracy);
	s.reactions =
	    randBoundsInclusive(state.rng, type->min_stats.reactions, type->max_stats.reactions);
	s.speed = randBoundsInclusive(state.rng, type->min_stats.speed, type->max_stats.speed);
	s.stamina = randBoundsInclusive(state.rng, type->min_stats.stamina, type->max_stats.stamina);
	s.bravery = randBoundsInclusive(state.rng, type->min_stats.bravery/10, type->max_stats.bravery/10)*10;
	s.strength = randBoundsInclusive(state.rng, type->min_stats.strength, type->max_stats.strength);
	s.morale = 100;
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
	agent->modified_stats = s;

	if (type->inventory)
	{
		// Player gets no default equipment
		if (org == state.getPlayer())
		{
		}
		// Aliens get equipment based on player score
		if (org == state.getAliens())
		{
			// FIXME: actually get player score here
			int playerScore = 50000;

			auto l = EquipmentSet::getByScore(state, playerScore)->generateEquipmentList(state);

			for (auto t : l)
			{
				agent->addEquipment(state, { &state, t->id });
			}
		}
		// Every human org but civilians and player gets equipment based on tech level
		else if (org != state.getCivilian())
		{
			auto l = EquipmentSet::getByLevel(state, org->tech_level)->generateEquipmentList(state);

			for (auto t : l)
			{
				agent->addEquipment(state, { &state, t->id });
			}
		}
	}
	else
	{
		agent->addEquipment(state, type->built_in_weapon_right);
		agent->addEquipment(state, type->built_in_weapon_left);
	}

	agent->updateSpeed();

	// Everything worked, add agent to state
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

void Agent::addEquipment(GameState &state, StateRef<AEquipmentType> type)
{
	Vec2<int> pos = { -1, 0 };
	for (auto &slot : this->type->equipment_layout_slots)
	{
		if (canAddEquipment(slot.bounds.p0, type))
		{
			pos = slot.bounds.p0;
			break;
		}
	}
	if (pos.x == -1)
	{
		LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found", type.id.cStr(), this->name.cStr());
		return;
	}

	this->addEquipment(state, pos, type);
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
	object->ownerAgent = StateRef<Agent>(&state, shared_from_this());
	this->equipment.emplace_back(object);
	updateSpeed();
}

void Agent::removeEquipment(sp<AEquipment> object) { this->equipment.remove(object); updateSpeed(); }

// Fixme: calculate how much equipment slows us down here
void Agent::updateSpeed() 
{

}

StateRef<BattleUnitAnimationPack> Agent::getAnimationPack() { return type->animation_packs[appearance]; }


StateRef<AEquipmentType> Agent::getItemInHands() 
{
	sp<AEquipment> e;
	e = getFirstItemInSlot(AgentType::EquipmentSlotType::RightHand);
	if (e)
		return{ e->type };
	e = getFirstItemInSlot(AgentType::EquipmentSlotType::LeftHand);
	if (e)
		return{ e->type };
	return nullptr;
}

sp<AEquipment> Agent::getFirstItemInSlot(AgentType::EquipmentSlotType type)
{
	for (auto e : equipment)
	{
		for (auto s : this->type->equipment_layout_slots)
		{
			if (s.bounds.p0 == e->equippedPosition)
			{
				if (s.type == type)
				{
					return e;
				}
			}
		}
	}
	return nullptr;
}

// FIXME: Properly return worn armor image pack if present
StateRef<BattleUnitImagePack> Agent::getImagePack(AgentType::BodyPart bodyPart) 
{
	switch (bodyPart)
	{
	case AgentType::BodyPart::Body:
		break;
	case AgentType::BodyPart::Legs:
		break;
	case AgentType::BodyPart::Helmet:
		break;
	case AgentType::BodyPart::LeftArm:
		break;
	case AgentType::BodyPart::RightArm:
		break;
	}
	return type->image_packs[appearance][bodyPart];
}
} // namespace OpenApoc
