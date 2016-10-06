#include "game/state/agent.h"
#include "game/state/aequipment.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"

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

template <>
sp<AgentBodyType> StateObject<AgentBodyType>::get(const GameState &state, const UString &id)
{
	auto it = state.agent_body_types.find(id);
	if (it == state.agent_body_types.end())
	{
		LogError("No agent_body_type matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<AgentBodyType>::getPrefix()
{
	static UString prefix = "AGENTBODYTYPE_";
	return prefix;
}
template <> const UString &StateObject<AgentBodyType>::getTypeName()
{
	static UString name = "AgentBodyType";
	return name;
}
template <>
const UString &StateObject<AgentBodyType>::getId(const GameState &state,
                                                 const sp<AgentBodyType> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agent_body_types)
	{
		if (a.second == ptr)
			return a.first;
	}
	LogError("No agent_type matching pointer %p", ptr.get());
	return emptyString;
}

template <>
sp<AgentEquipmentLayout> StateObject<AgentEquipmentLayout>::get(const GameState &state,
                                                                const UString &id)
{
	auto it = state.agent_equipment_layouts.find(id);
	if (it == state.agent_equipment_layouts.end())
	{
		LogError("No agent_body_type matching ID \"%s\"", id.cStr());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<AgentEquipmentLayout>::getPrefix()
{
	static UString prefix = "AGENTEQUIPMENTLAYOUT_";
	return prefix;
}
template <> const UString &StateObject<AgentEquipmentLayout>::getTypeName()
{
	static UString name = "AgentEquipmentLayout";
	return name;
}
template <>
const UString &StateObject<AgentEquipmentLayout>::getId(const GameState &state,
                                                        const sp<AgentEquipmentLayout> ptr)
{
	static const UString emptyString = "";
	for (auto &a : state.agent_equipment_layouts)
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

AgentEquipmentLayout::EquipmentSlotType AgentType::getArmorSlotType(BodyPart bodyPart)
{
	switch (bodyPart)
	{
		case AgentType::BodyPart::Body:
			return AgentEquipmentLayout::EquipmentSlotType::ArmorBody;
			break;
		case AgentType::BodyPart::Legs:
			return AgentEquipmentLayout::EquipmentSlotType::ArmorLegs;
			break;
		case AgentType::BodyPart::Helmet:
			return AgentEquipmentLayout::EquipmentSlotType::ArmorHelmet;
			break;
		case AgentType::BodyPart::LeftArm:
			return AgentEquipmentLayout::EquipmentSlotType::ArmorLeftHand;
			break;
		case AgentType::BodyPart::RightArm:
			return AgentEquipmentLayout::EquipmentSlotType::ArmorRightHand;
			break;
	}
	LogError("Unknown body part?");
	return AgentEquipmentLayout::EquipmentSlotType::None;
}

AgentEquipmentLayout::EquipmentLayoutSlot *
AgentType::getFirstSlot(AgentEquipmentLayout::EquipmentSlotType type)
{
	for (auto &s : equipment_layout->slots)
	{
		if (s.type == type)
		{
			return &s;
		}
	}
	return nullptr;
}

StateRef<Agent> AgentGenerator::createAgent(GameState &state, StateRef<Organisation> org,
                                            AgentType::Role role) const
{
	std::list<sp<AgentType>> types;
	for (auto &t : state.agent_types)
		if (t.second->role == role && t.second->playable)
			types.insert(types.begin(), t.second);
	auto type = listRandomiser(state.rng, types);

	return createAgent(state, org, {&state, AgentType::getId(state, type)});
}

StateRef<Agent> AgentGenerator::createAgent(GameState &state, StateRef<Organisation> org,
                                            StateRef<AgentType> type) const
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
	s.bravery =
	    randBoundsInclusive(state.rng, type->min_stats.bravery / 10, type->max_stats.bravery / 10) *
	    10;
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

	// Everything worked, add agent to state (before we add his equipment)
	this->num_created++;
	state.agents[ID] = agent;

	// Fill initial equipment list
	std::list<sp<AEquipmentType>> initialEquipment;
	if (type->inventory)
	{
		// Player gets no default equipment
		if (org == state.getPlayer())
		{
		}
		// Aliens get equipment based on player score
		else if (org == state.getAliens())
		{
			// FIXME: actually get player score here
			int playerScore = 50000;

			initialEquipment =
			    EquipmentSet::getByScore(state, playerScore)->generateEquipmentList(state);
		}
		// Every human org but civilians and player gets equipment based on tech level
		else if (org != state.getCivilian())
		{
			initialEquipment =
			    EquipmentSet::getByLevel(state, org->tech_level)->generateEquipmentList(state);
		}
	}
	else
	{
		initialEquipment.push_back(type->built_in_weapon_right);
		initialEquipment.push_back(type->built_in_weapon_left);
	}

	// Add initial equipment
	for (auto t : initialEquipment)
	{
		if (!t)
			continue;
		if (t->type == AEquipmentType::Type::Ammo)
		{
			agent->addEquipmentByType(state, {&state, t->id},
			                          AgentEquipmentLayout::EquipmentSlotType::General);
		}
		else
		{
			agent->addEquipmentByType(state, {&state, t->id});
		}
	}

	agent->updateSpeed();
	agent->modified_stats.restoreTU();

	return {&state, ID};
}

bool Agent::isBodyStateAllowed(AgentType::BodyState bodyState) const
{
	static const std::list<AgentEquipmentLayout::EquipmentSlotType> armorslots = {
	    AgentEquipmentLayout::EquipmentSlotType::ArmorBody,
	    AgentEquipmentLayout::EquipmentSlotType::ArmorHelmet,
	    AgentEquipmentLayout::EquipmentSlotType::ArmorLeftHand,
	    AgentEquipmentLayout::EquipmentSlotType::ArmorLegs,
	    AgentEquipmentLayout::EquipmentSlotType::ArmorRightHand};

	if (type->bodyType->allowed_body_states.find(bodyState) !=
	    type->bodyType->allowed_body_states.end())
		return true;
	if (bodyState == AgentType::BodyState::Flying)
	{
		for (auto t : armorslots)
		{
			auto e = getFirstItemInSlot(t);
			if (e && e->type->provides_flight)
			{
				return true;
			}
		}
	}
	return false;
}

bool Agent::isMovementStateAllowed(AgentType::MovementState movementState) const
{
	return type->bodyType->allowed_movement_states.find(movementState) !=
	       type->bodyType->allowed_movement_states.end();
}

bool Agent::isFacingAllowed(Vec2<int> facing) const
{
	return type->bodyType->allowed_facing.find(facing) != type->bodyType->allowed_facing.end();
}

sp<AEquipment> Agent::getArmor(AgentType::BodyPart bodyPart) const
{
	auto a = getFirstItemInSlot(AgentType::getArmorSlotType(bodyPart));
	if (a)
	{
		return a;
	}
	return nullptr;
}

AgentEquipmentLayout::EquipmentSlotType Agent::canAddEquipment(Vec2<int> pos,
                                                               StateRef<AEquipmentType> type) const
{
	Vec2<int> slotOrigin;
	bool slotFound = false;
	bool slotIsArmor = false;
	AgentEquipmentLayout::EquipmentSlotType slotType;
	// Check the slot this occupies hasn't already got something there
	for (auto &slot : this->type->equipment_layout->slots)
	{
		if (slot.bounds.within(pos))
		{
			// None is not a valid equipment slot
			if (slot.type == AgentEquipmentLayout::EquipmentSlotType::None)
			{
				LogError("Agent has equipment slot \"None\"??");
				return AgentEquipmentLayout::EquipmentSlotType::None;
			}
			// If we are equipping into an armor slot, ensure the item being equipped is armor
			if (slot.type == AgentEquipmentLayout::EquipmentSlotType::ArmorBody ||
			    slot.type == AgentEquipmentLayout::EquipmentSlotType::ArmorLegs ||
			    slot.type == AgentEquipmentLayout::EquipmentSlotType::ArmorHelmet ||
			    slot.type == AgentEquipmentLayout::EquipmentSlotType::ArmorLeftHand ||
			    slot.type == AgentEquipmentLayout::EquipmentSlotType::ArmorRightHand)
			{
				slotIsArmor = true;
				if (type->type != AEquipmentType::Type::Armor)
					break;
			}
			slotOrigin = slot.bounds.p0;
			slotType = slot.type;
			slotFound = true;

			break;
		}
	}
	// If this was not within a slot fail
	if (!slotFound)
	{
		return AgentEquipmentLayout::EquipmentSlotType::None;
	}

	// Check that the equipment doesn't overlap with any other and doesn't
	// go outside a slot of the correct type
	// For agents, armor on the agent body does indeed overlap with each other,
	// So there is no point checking if there's an overlap (there will be)
	if (slotIsArmor)
	{
		// This could would check that every slot is of appropriate type,
		// But for units armor this is unnecesary
		/*
		for (int y = 0; y < type->equipscreen_size.y; y++)
		{
		    for (int x = 0; x < type->equipscreen_size.x; x++)
		    {
		        Vec2<int> slotPos = { x, y };
		        slotPos += pos;
		        bool validSlot = false;
		        for (auto &slot : this->type->equipment_layout->slots)
		        {
		            if (slot.bounds.within(slotPos) && (
		                slot.type == AgentType::EquipmentSlotType::ArmorBody
		                || slot.type == AgentType::EquipmentSlotType::ArmorLegs
		                || slot.type == AgentType::EquipmentSlotType::ArmorHelmet
		                || slot.type == AgentType::EquipmentSlotType::ArmorLeftHand
		                || slot.type == AgentType::EquipmentSlotType::ArmorRightHand
		                ))
		            {
		                validSlot = true;
		                break;
		            }
		        }
		        if (!validSlot)
		        {
		            LogInfo("Equipping \"%s\" on \"%s\" at {%d,%d} failed: Armor intersecting both
		armor and non-armor slot",
		                type->name.cStr(), this->name.cStr(), pos.x, pos.y);
		            return false;
		        }
		    }
		}
		*/
	}
	else
	{
		Rect<int> bounds{pos, pos + type->equipscreen_size};
		for (auto &otherEquipment : this->equipment)
		{
			// Something is already in that slot, fail
			if (otherEquipment->equippedPosition == slotOrigin)
			{
				return AgentEquipmentLayout::EquipmentSlotType::None;
			}
			Rect<int> otherBounds{otherEquipment->equippedPosition,
			                      otherEquipment->equippedPosition +
			                          otherEquipment->type->equipscreen_size};
			if (otherBounds.intersects(bounds))
			{
				LogInfo(
				    "Equipping \"%s\" on \"%s\" at {%d,%d} failed: Intersects with other equipment",
				    type->name.cStr(), this->name.cStr(), pos.x, pos.y);
				return AgentEquipmentLayout::EquipmentSlotType::None;
			}
		}
	}

	return slotType;
}

// If type is null we look for any slot, if type is not null we look for slot that can fit the type
Vec2<int> Agent::findFirstSlotByType(AgentEquipmentLayout::EquipmentSlotType slotType,
                                     StateRef<AEquipmentType> type)
{
	Vec2<int> pos = {-1, 0};
	if (slotType == AgentEquipmentLayout::EquipmentSlotType::None)
	{
		LogError("Trying to find equipment slot None??");
		return pos;
	}
	for (auto &slot : this->type->equipment_layout->slots)
	{
		if (slot.type == slotType && (!type ||
		                              canAddEquipment(slot.bounds.p0, type) !=
		                                  AgentEquipmentLayout::EquipmentSlotType::None))
		{
			pos = slot.bounds.p0;
			break;
		}
	}
	return pos;
}

void Agent::addEquipmentByType(GameState &state, StateRef<AEquipmentType> type)
{
	Vec2<int> pos;
	bool slotFound = false;
	for (auto &slot : this->type->equipment_layout->slots)
	{
		if (canAddEquipment(slot.bounds.p0, type) != AgentEquipmentLayout::EquipmentSlotType::None)
		{
			pos = slot.bounds.p0;
			slotFound = true;
			break;
		}
	}
	if (!slotFound)
	{
		LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found", type.id.cStr(),
		         this->name.cStr());
		return;
	}

	this->addEquipmentByType(state, pos, type);
}

void Agent::addEquipmentByType(GameState &state, StateRef<AEquipmentType> type,
                               AgentEquipmentLayout::EquipmentSlotType slotType)
{
	if (slotType == AgentEquipmentLayout::EquipmentSlotType::None)
	{
		LogError("Trying to add equipment to slot None??");
		return;
	}
	Vec2<int> pos = findFirstSlotByType(slotType, type);
	if (pos.x == -1)
	{
		LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found", type.id.cStr(),
		         this->name.cStr());
		return;
	}

	this->addEquipmentByType(state, pos, type);
}

void Agent::addEquipmentByType(GameState &state, Vec2<int> pos, StateRef<AEquipmentType> type)
{
	if (this->canAddEquipment(pos, type) == AgentEquipmentLayout::EquipmentSlotType::None)
	{
		LogError("Trying to add \"%s\" at {%d,%d} on agent \"%s\" failed", type.id.cStr(), pos.x,
		         pos.y, this->name.cStr());
	}
	auto equipment = mksp<AEquipment>();
	equipment->type = type;
	if (type->ammo_types.size() > 0)
	{
		equipment->payloadType = *type->ammo_types.begin();
		equipment->ammo = equipment->payloadType->max_ammo;
	}
	else
	{
		equipment->ammo = type->max_ammo;
	}
	this->addEquipment(state, pos, equipment);
}

void Agent::addEquipment(GameState &state, sp<AEquipment> object,
                         AgentEquipmentLayout::EquipmentSlotType slotType)
{
	if (slotType == AgentEquipmentLayout::EquipmentSlotType::None)
	{
		LogError("Trying to add equipment to slot None??");
		return;
	}
	Vec2<int> pos = findFirstSlotByType(slotType, object->type);
	if (pos.x == -1)
	{
		LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found", type.id.cStr(),
		         this->name.cStr());
		return;
	}

	this->addEquipment(state, pos, object);
}

void Agent::addEquipment(GameState &state, Vec2<int> pos, sp<AEquipment> object)
{
	auto slotType = this->canAddEquipment(pos, object->type);
	if (slotType == AgentEquipmentLayout::EquipmentSlotType::None)
	{
		LogError("Trying to add \"%s\" at {%d,%d} on agent  \"%s\" failed", object->type.id.cStr(),
		         pos.x, pos.y, this->name.cStr());
	}

	LogInfo("Equipped \"%s\" with equipment \"%s\"", this->name.cStr(), object->type->name.cStr());
	object->equippedPosition = pos;
	object->ownerAgent = StateRef<Agent>(&state, shared_from_this());
	object->equippedSlotType = slotType;
	if (slotType == AgentEquipmentLayout::EquipmentSlotType::RightHand)
	{
		rightHandItem = object;
	}
	if (slotType == AgentEquipmentLayout::EquipmentSlotType::LeftHand)
	{
		leftHandItem = object;
	}
	this->equipment.emplace_back(object);
	updateSpeed();
}

void Agent::removeEquipment(sp<AEquipment> object)
{
	this->equipment.remove(object);
	if (unit)
	{
		unit->updateDisplayedItem();
	}
	if (object->equippedSlotType == AgentEquipmentLayout::EquipmentSlotType::RightHand)
	{
		rightHandItem = nullptr;
	}
	if (object->equippedSlotType == AgentEquipmentLayout::EquipmentSlotType::LeftHand)
	{
		leftHandItem = nullptr;
	}
	object->equippedSlotType = AgentEquipmentLayout::EquipmentSlotType::None;
	updateSpeed();
}

void Agent::updateSpeed()
{
	// Fixme: calculate how much equipment slows us down here
	initial_stats.restoreTU();
	current_stats.restoreTU();
}

StateRef<BattleUnitAnimationPack> Agent::getAnimationPack() const
{
	return type->animation_packs[appearance];
}

StateRef<AEquipmentType> Agent::getDominantItemInHands(StateRef<AEquipmentType> itemLastFired) const
{
	sp<AEquipment> e1 = getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::RightHand);
	sp<AEquipment> e2 = getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::LeftHand);
	// If there is only one item - return it, if none - return nothing
	if (!e1 && !e2)
		return nullptr;
	else if (e1 && !e2)
		return e1->type;
	else if (e2 && !e1)
		return e2->type;
	// If item was last fired and still in hands - return it
	if (itemLastFired)
	{
		if (e1 && e1->type == itemLastFired)
			return e1->type;
		if (e2 && e2->type == itemLastFired)
			return e2->type;
	}
	// Calculate item priorities:
	// - Firing (whichever fires sooner)
	// - CanFire >> Two-Handed >> Weapon >> Usable Item >> Others
	int e1Priority =
	    e1->isFiring()
	        ? 1440 - e1->weapon_fire_ticks_remaining
	        : (e1->canFire() ? 4 : (e1->type->two_handed
	                                    ? 3
	                                    : (e1->type->type == AEquipmentType::Type::Weapon
	                                           ? 2
	                                           : (e1->type->type != AEquipmentType::Type::Ammo &&
	                                              e1->type->type != AEquipmentType::Type::Armor &&
	                                              e1->type->type != AEquipmentType::Type::Loot)
	                                                 ? 1
	                                                 : 0)));
	int e2Priority =
	    e2->isFiring()
	        ? 1440 - e2->weapon_fire_ticks_remaining
	        : (e2->canFire() ? 4 : (e2->type->two_handed
	                                    ? 3
	                                    : (e2->type->type == AEquipmentType::Type::Weapon
	                                           ? 2
	                                           : (e2->type->type != AEquipmentType::Type::Ammo &&
	                                              e2->type->type != AEquipmentType::Type::Armor &&
	                                              e2->type->type != AEquipmentType::Type::Loot)
	                                                 ? 1
	                                                 : 0)));
	// Right hand has priority in case of a tie
	if (e1Priority >= e2Priority)
		return e1->type;
	return e2->type;
}

sp<AEquipment> Agent::getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType type,
                                         bool lazy) const
{
	if (lazy)
	{
		if (type == AgentEquipmentLayout::EquipmentSlotType::RightHand)
			return rightHandItem;
		if (type == AgentEquipmentLayout::EquipmentSlotType::LeftHand)
			return leftHandItem;
	}
	for (auto e : equipment)
	{
		for (auto s : this->type->equipment_layout->slots)
		{
			if (s.bounds.p0 == e->equippedPosition && s.type == type)
			{
				return e;
			}
		}
	}
	return nullptr;
}

StateRef<BattleUnitImagePack> Agent::getImagePack(AgentType::BodyPart bodyPart) const
{
	AgentEquipmentLayout::EquipmentSlotType slotType = AgentType::getArmorSlotType(bodyPart);

	auto e = getFirstItemInSlot(slotType);
	if (e)
		return e->type->body_image_pack;
	auto it = type->image_packs[appearance].find(bodyPart);
	if (it != type->image_packs[appearance].end())
		return it->second;
	return nullptr;
}
} // namespace OpenApoc
