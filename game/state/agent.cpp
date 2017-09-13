#include "game/state/agent.h"
#include "game/state/aequipment.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"
#include "game/state/rules/aequipment_type.h"
#include "library/strings_format.h"

namespace OpenApoc
{

static const unsigned TICKS_PER_PHYSICAL_TRAINING = 4 * TICKS_PER_HOUR;
static const unsigned TICKS_PER_PSI_TRAINING = 4 * TICKS_PER_HOUR;

sp<AgentType> AgentType::get(const GameState &state, const UString &id)
{
	auto it = state.agent_types.find(id);
	if (it == state.agent_types.end())
	{
		LogError("No agent_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &AgentType::getPrefix()
{
	static UString prefix = "AGENTTYPE_";
	return prefix;
}
const UString &AgentType::getTypeName()
{
	static UString name = "AgentType";
	return name;
}

const UString &AgentType::getId(const GameState &state, const sp<AgentType> ptr)
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

sp<AgentBodyType> AgentBodyType::get(const GameState &state, const UString &id)
{
	auto it = state.agent_body_types.find(id);
	if (it == state.agent_body_types.end())
	{
		LogError("No agent_body_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &AgentBodyType::getPrefix()
{
	static UString prefix = "AGENTBODYTYPE_";
	return prefix;
}
const UString &AgentBodyType::getTypeName()
{
	static UString name = "AgentBodyType";
	return name;
}

const UString &AgentBodyType::getId(const GameState &state, const sp<AgentBodyType> ptr)
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

sp<AgentEquipmentLayout> AgentEquipmentLayout::get(const GameState &state, const UString &id)
{
	auto it = state.agent_equipment_layouts.find(id);
	if (it == state.agent_equipment_layouts.end())
	{
		LogError("No agent_body_type matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &AgentEquipmentLayout::getPrefix()
{
	static UString prefix = "AGENTEQUIPMENTLAYOUT_";
	return prefix;
}
const UString &AgentEquipmentLayout::getTypeName()
{
	static UString name = "AgentEquipmentLayout";
	return name;
}

const UString &AgentEquipmentLayout::getId(const GameState &state,
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

sp<Agent> Agent::get(const GameState &state, const UString &id)
{
	auto it = state.agents.find(id);
	if (it == state.agents.end())
	{
		LogError("No agent matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &Agent::getPrefix()
{
	static UString prefix = "AGENT_";
	return prefix;
}
const UString &Agent::getTypeName()
{
	static UString name = "Agent";
	return name;
}
const UString &Agent::getId(const GameState &state, const sp<Agent> ptr)
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

AgentType::AgentType() : aiType(AIType::None) {}

EquipmentSlotType AgentType::getArmorSlotType(BodyPart bodyPart)
{
	switch (bodyPart)
	{
		case BodyPart::Body:
			return EquipmentSlotType::ArmorBody;
			break;
		case BodyPart::Legs:
			return EquipmentSlotType::ArmorLegs;
			break;
		case BodyPart::Helmet:
			return EquipmentSlotType::ArmorHelmet;
			break;
		case BodyPart::LeftArm:
			return EquipmentSlotType::ArmorLeftHand;
			break;
		case BodyPart::RightArm:
			return EquipmentSlotType::ArmorRightHand;
			break;
	}
	LogError("Unknown body part?");
	return EquipmentSlotType::General;
}

EquipmentLayoutSlot *AgentType::getFirstSlot(EquipmentSlotType type)
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
	UString ID = Agent::generateObjectID(state);

	auto agent = mksp<Agent>();

	agent->owner = org;
	agent->type = type;
	agent->gender = probabilityMapRandomizer(state.rng, type->gender_chance);

	if (type->playable)
	{

		auto firstNameList = this->first_names.find(agent->gender);
		if (firstNameList == this->first_names.end())
		{
			LogError("No first name list for gender");
			return nullptr;
		}

		auto firstName = listRandomiser(state.rng, firstNameList->second);
		auto secondName = listRandomiser(state.rng, this->second_names);
		agent->name = format("%s %s", firstName, secondName);
	}
	else
	{
		agent->name = type->name;
	}
	// FIXME: When rng is fixed we can remove this unnesecary kludge
	// RNG is bad at generating small numbers, so we generate more and divide
	agent->appearance = randBoundsExclusive(state.rng, 0, type->appearance_count * 10) / 10;

	agent->portrait =
	    randBoundsInclusive(state.rng, 0, (int)type->portraits[agent->gender].size() - 1);

	AgentStats s;
	s.health = randBoundsInclusive(state.rng, type->min_stats.health, type->max_stats.health);
	s.accuracy = randBoundsInclusive(state.rng, type->min_stats.accuracy, type->max_stats.accuracy);
	s.reactions =
	    randBoundsInclusive(state.rng, type->min_stats.reactions, type->max_stats.reactions);
	s.setSpeed(randBoundsInclusive(state.rng, type->min_stats.speed, type->max_stats.speed));
	s.stamina =
	    randBoundsInclusive(state.rng, type->min_stats.stamina, type->max_stats.stamina) * 10;
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
			int playerScore = state.score;

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
	for (auto &t : initialEquipment)
	{
		if (!t)
			continue;
		agent->addEquipmentByType(state, {&state, t->id}, type->inventory);
	}

	agent->updateSpeed();
	agent->modified_stats.restoreTU();

	return {&state, ID};
}

bool Agent::isBodyStateAllowed(BodyState bodyState) const
{
	static const std::list<EquipmentSlotType> armorslots = {
	    EquipmentSlotType::ArmorBody, EquipmentSlotType::ArmorHelmet,
	    EquipmentSlotType::ArmorLeftHand, EquipmentSlotType::ArmorLegs,
	    EquipmentSlotType::ArmorRightHand};

	if (type->bodyType->allowed_body_states.find(bodyState) !=
	    type->bodyType->allowed_body_states.end())
	{
		return true;
	}
	if (bodyState == BodyState::Flying)
	{
		for (auto &t : armorslots)
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

bool Agent::isMovementStateAllowed(MovementState movementState) const
{
	return type->bodyType->allowed_movement_states.find(movementState) !=
	       type->bodyType->allowed_movement_states.end();
}

bool Agent::isFireDuringMovementStateAllowed(MovementState movementState) const
{
	return type->bodyType->allowed_fire_movement_states.find(movementState) !=
	       type->bodyType->allowed_fire_movement_states.end();
}

bool Agent::isFacingAllowed(Vec2<int> facing) const
{
	return type->bodyType->allowed_facing.empty() ||
	       type->bodyType->allowed_facing[appearance].find(facing) !=
	           type->bodyType->allowed_facing[appearance].end();
}

const std::set<Vec2<int>> *Agent::getAllowedFacings() const
{
	static const std::set<Vec2<int>> allFacings = {{0, -1}, {1, -1}, {1, 0},  {1, 1},
	                                               {0, 1},  {-1, 1}, {-1, 0}, {-1, -1}};

	if (type->bodyType->allowed_facing.empty())
	{
		return &allFacings;
	}
	else
	{
		return &type->bodyType->allowed_facing[appearance];
	}
}

int Agent::getReactionValue() const
{
	return modified_stats.reactions * modified_stats.time_units / current_stats.time_units;
}

int Agent::getTULimit(int reactionValue) const
{
	if (reactionValue == 0)
	{
		return 0;
	}
	else if (reactionValue >= getReactionValue())
	{
		return modified_stats.time_units;
	}
	else
	{
		return current_stats.time_units * reactionValue / modified_stats.reactions;
	}
}

UString Agent::getRankName() const
{
	switch (rank)
	{
		case Rank::Rookie:
			return tr("Rookie");
		case Rank::Squaddie:
			return tr("Squaddie");
		case Rank::SquadLeader:
			return tr("Squad leader");
		case Rank::Sergeant:
			return tr("Sergeant");
		case Rank::Captain:
			return tr("Captain");
		case Rank::Colonel:
			return tr("Colonel");
		case Rank::Commander:
			return tr("Commander");
	}
	LogError("Unknown rank %d", (int)rank);
	return "";
}

sp<AEquipment> Agent::getArmor(BodyPart bodyPart) const
{
	auto a = getFirstItemInSlot(AgentType::getArmorSlotType(bodyPart));
	if (a)
	{
		return a;
	}
	return nullptr;
}

bool Agent::canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> type) const
{
	EquipmentSlotType slotType = EquipmentSlotType::General;
	return canAddEquipment(pos, type, slotType);
}

bool Agent::canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> type,
                            EquipmentSlotType &slotType) const
{
	Vec2<int> slotOrigin;
	bool slotFound = false;
	bool slotIsArmor = false;
	// Check the slot this occupies hasn't already got something there
	for (auto &slot : this->type->equipment_layout->slots)
	{
		if (slot.bounds.within(pos))
		{
			// If we are equipping into an armor slot, ensure the item being equipped is correct
			// armor
			if (isArmorEquipmentSlot(slot.type))
			{
				if (type->type != AEquipmentType::Type::Armor)
				{
					break;
				}
				if (AgentType::getArmorSlotType(type->body_part) != slot.type)
				{
					break;
				}
				slotIsArmor = true;
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
		return false;
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
		            if (slot.bounds.within(slotPos)
		                && (isArmorEquipmentSlot(slot.type)))
		            {
		                validSlot = true;
		                break;
		            }
		        }
		        if (!validSlot)
		        {
		            LogInfo("Equipping \"%s\" on \"%s\" at {%d,%d} failed: Armor intersecting both
		armor and non-armor slot",
		                type->name, this->name, pos.x, pos.y);
		            return false;
		        }
		    }
		}
		*/
	}
	else
	{
		pos = slotOrigin;
		// Check that the equipment doesn't overlap with any other
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
				return false;
			}
		}
		// Check that this doesn't go outside a slot of the correct type
		for (int y = 0; y < type->equipscreen_size.y; y++)
		{
			for (int x = 0; x < type->equipscreen_size.x; x++)
			{
				Vec2<int> slotPos = {x, y};
				slotPos += pos;
				bool validSlot = false;
				for (auto &slot : this->type->equipment_layout->slots)
				{
					if (slot.bounds.within(slotPos) && slot.type == slotType)
					{
						validSlot = true;
						break;
					}
				}
				if (!validSlot)
				{
					return false;
				}
			}
		}
	}

	return true;
}

// If type is null we look for any slot, if type is not null we look for slot that can fit the type
Vec2<int> Agent::findFirstSlotByType(EquipmentSlotType slotType, StateRef<AEquipmentType> type)
{
	Vec2<int> pos = {-1, 0};
	for (auto &slot : this->type->equipment_layout->slots)
	{
		if (slot.type == slotType && (!type || canAddEquipment(slot.bounds.p0, type)))
		{
			pos = slot.bounds.p0;
			break;
		}
	}
	return pos;
}

sp<AEquipment> Agent::addEquipmentByType(GameState &state, StateRef<AEquipmentType> type,
                                         bool allowFailure)
{
	Vec2<int> pos;
	bool slotFound = false;
	EquipmentSlotType prefSlotType = EquipmentSlotType::General;
	bool prefSlot = false;
	if (type->type == AEquipmentType::Type::Ammo)
	{
		auto wpn = addEquipmentAsAmmoByType(state, type);
		if (wpn)
		{
			return wpn;
		}
		prefSlotType = EquipmentSlotType::General;
		prefSlot = true;
	}
	else if (type->type == AEquipmentType::Type::Armor)
	{
		switch (type->body_part)
		{
			case BodyPart::Body:
				prefSlotType = EquipmentSlotType::ArmorBody;
				break;
			case BodyPart::Legs:
				prefSlotType = EquipmentSlotType::ArmorLegs;
				break;
			case BodyPart::Helmet:
				prefSlotType = EquipmentSlotType::ArmorHelmet;
				break;
			case BodyPart::LeftArm:
				prefSlotType = EquipmentSlotType::ArmorLeftHand;
				break;
			case BodyPart::RightArm:
				prefSlotType = EquipmentSlotType::ArmorRightHand;
				break;
		}
		prefSlot = true;
	}
	if (prefSlot)
	{
		for (auto &slot : this->type->equipment_layout->slots)
		{
			if (slot.type != prefSlotType)
			{
				continue;
			}
			if (canAddEquipment(slot.bounds.p0, type))
			{
				pos = slot.bounds.p0;
				slotFound = true;
				break;
			}
		}
	}
	if (!slotFound)
	{
		for (auto &slot : this->type->equipment_layout->slots)
		{
			if (canAddEquipment(slot.bounds.p0, type))
			{
				pos = slot.bounds.p0;
				slotFound = true;
				break;
			}
		}
	}
	if (!slotFound)
	{
		if (!allowFailure)
		{
			LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found", type.id,
			         this->name);
		}
		return nullptr;
	}

	return addEquipmentByType(state, pos, type);
}

sp<AEquipment> Agent::addEquipmentByType(GameState &state, StateRef<AEquipmentType> type,
                                         EquipmentSlotType slotType, bool allowFailure)
{
	if (type->type == AEquipmentType::Type::Ammo)
	{
		auto wpn = addEquipmentAsAmmoByType(state, type);
		if (wpn)
		{
			return wpn;
		}
	}
	Vec2<int> pos = findFirstSlotByType(slotType, type);
	if (pos.x == -1)
	{
		if (!allowFailure)
		{
			LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found", type.id,
			         this->name);
		}
		return nullptr;
	}

	return addEquipmentByType(state, pos, type);
}

sp<AEquipment> Agent::addEquipmentByType(GameState &state, Vec2<int> pos,
                                         StateRef<AEquipmentType> type)
{
	auto equipment = mksp<AEquipment>();
	equipment->type = type;
	equipment->armor = type->armor;
	if (type->ammo_types.size() == 0)
	{
		equipment->ammo = type->max_ammo;
	}
	this->addEquipment(state, pos, equipment);
	if (type->ammo_types.size() > 0)
	{
		equipment->loadAmmo(state);
	}
	return equipment;
}

sp<AEquipment> Agent::addEquipmentAsAmmoByType(GameState &state, StateRef<AEquipmentType> type)
{
	for (auto &e : equipment)
	{
		if (e->type->type == AEquipmentType::Type::Weapon && e->ammo == 0 &&
		    std::find(e->type->ammo_types.begin(), e->type->ammo_types.end(), type) !=
		        e->type->ammo_types.end())
		{
			e->payloadType = type;
			e->ammo = type->max_ammo;
			return e;
		}
	}
	return nullptr;
}

void Agent::addEquipment(GameState &state, sp<AEquipment> object, EquipmentSlotType slotType)
{
	Vec2<int> pos = findFirstSlotByType(slotType, object->type);
	if (pos.x == -1)
	{
		LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found", type.id,
		         this->name);
		return;
	}

	this->addEquipment(state, pos, object);
}

void Agent::addEquipment(GameState &state, Vec2<int> pos, sp<AEquipment> object)
{
	EquipmentSlotType slotType;
	if (!canAddEquipment(pos, object->type, slotType))
	{
		LogError("Trying to add \"%s\" at %s on agent  \"%s\" failed", object->type.id, pos,
		         this->name);
	}

	LogInfo("Equipped \"%s\" with equipment \"%s\"", this->name, object->type->name);
	// Proper position
	for (auto &slot : this->type->equipment_layout->slots)
	{
		if (slot.bounds.within(pos))
		{
			pos = slot.bounds.p0;
			break;
		}
	}
	object->equippedPosition = pos;
	object->ownerAgent = StateRef<Agent>(&state, shared_from_this());
	object->ownerUnit.clear();
	object->equippedSlotType = slotType;
	if (slotType == EquipmentSlotType::RightHand)
	{
		rightHandItem = object;
	}
	if (slotType == EquipmentSlotType::LeftHand)
	{
		leftHandItem = object;
	}
	this->equipment.emplace_back(object);
	updateSpeed();
	if (unit)
	{
		unit->updateDisplayedItem();
	}
}

void Agent::removeEquipment(GameState &state, sp<AEquipment> object)
{
	this->equipment.remove(object);
	if (object->equippedSlotType == EquipmentSlotType::RightHand)
	{
		rightHandItem = nullptr;
	}
	if (object->equippedSlotType == EquipmentSlotType::LeftHand)
	{
		leftHandItem = nullptr;
	}
	if (unit)
	{
		// Stop flying if jetpack lost
		if (object->type->provides_flight && unit->target_body_state == BodyState::Flying &&
		    !isBodyStateAllowed(BodyState::Flying))
		{
			unit->setBodyState(state, BodyState::Standing);
		}
		unit->updateDisplayedItem();
	}
	object->ownerAgent.clear();
	updateSpeed();
}

void Agent::updateSpeed()
{
	int encumbrance = 0;
	for (auto &item : equipment)
	{
		encumbrance += item->getWeight();
	}
	overEncumbred = current_stats.strength * 4 < encumbrance;
	encumbrance *= encumbrance;

	// Expecting str to never be 0
	int strength = current_stats.strength;
	strength *= strength * 16;

	// Ensure actual speed is at least "1"
	modified_stats.speed =
	    std::max(8,
	             ((strength + encumbrance) / 2 + current_stats.speed * (strength - encumbrance)) /
	                 (strength + encumbrance));
}

void Agent::updateModifiedStats()
{
	int health = modified_stats.health;
	modified_stats = current_stats;
	modified_stats.health = health;
	updateSpeed();
}

void Agent::trainPhysical(GameState &state, unsigned ticks)
{
	if (!type->can_improve)
	{
		return;
	}
	trainingPhysicalTicksAccumulated += ticks;
	while (trainingPhysicalTicksAccumulated >= TICKS_PER_PHYSICAL_TRAINING)
	{
		trainingPhysicalTicksAccumulated -= TICKS_PER_PHYSICAL_TRAINING;

		if (randBoundsExclusive(state.rng, 0, 100) >= current_stats.health)
		{
			current_stats.health++;
			modified_stats.health++;
		}
		if (randBoundsExclusive(state.rng, 0, 100) >= current_stats.accuracy)
		{
			current_stats.accuracy++;
		}
		if (randBoundsExclusive(state.rng, 0, 100) >= current_stats.reactions)
		{
			current_stats.reactions++;
		}
		if (randBoundsExclusive(state.rng, 0, 100) >= current_stats.speed)
		{
			current_stats.speed++;
			current_stats.restoreTU();
		}
		if (randBoundsExclusive(state.rng, 0, 2000) >= current_stats.stamina)
		{
			current_stats.stamina += 20;
		}
		if (randBoundsExclusive(state.rng, 0, 100) >= current_stats.strength)
		{
			current_stats.strength++;
		}
		updateModifiedStats();
	}
}

void Agent::trainPsi(GameState &state, unsigned ticks)
{
	if (!type->can_improve)
	{
		return;
	}
	trainingPsiTicksAccumulated += ticks;
	while (trainingPsiTicksAccumulated >= TICKS_PER_PSI_TRAINING)
	{
		trainingPsiTicksAccumulated -= TICKS_PER_PSI_TRAINING;

		// FIXME: Ensure correct
		// Roger Wong gives this info:
		// - Improve up to 3x base value
		// - Chance is 100 - (3 x current - initial)
		// - Hybrids have much higher chance to improve and humans hardly ever improve
		// This seems very wong (lol)!
		//	 For example, if initial is 50, no improvement ever possible because 100 - (150-50) = 0
		// already)
		//   Or, for initial 10, even at 30 the formula would be 100 - (90-10) = 20% improve chance
		//   In this formula the bigger is the initial stat, the harder it is to improve
		// Therefore, we'll use a formula that makes senes and follows what he said.
		// Properties of our formula:
		// - properly gives 0 chance when current = 3x initial
		// - gives higher chance with higher initial values

		if (randBoundsExclusive(state.rng, 0, 100) <
		    3 * initial_stats.psi_attack - current_stats.psi_attack)
		{
			current_stats.psi_attack += current_stats.psi_energy / 20;
		}
		if (randBoundsExclusive(state.rng, 0, 100) <
		    3 * initial_stats.psi_defence - current_stats.psi_defence)
		{
			current_stats.psi_defence += current_stats.psi_energy / 20;
		}
		if (randBoundsExclusive(state.rng, 0, 100) <
		    3 * initial_stats.psi_energy - current_stats.psi_energy)
		{
			current_stats.psi_energy++;
		}

		updateModifiedStats();
	}
}

StateRef<BattleUnitAnimationPack> Agent::getAnimationPack() const
{
	return type->animation_packs[appearance];
}

StateRef<AEquipmentType> Agent::getDominantItemInHands(StateRef<AEquipmentType> itemLastFired) const
{
	sp<AEquipment> e1 = getFirstItemInSlot(EquipmentSlotType::RightHand);
	sp<AEquipment> e2 = getFirstItemInSlot(EquipmentSlotType::LeftHand);
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

sp<AEquipment> Agent::getFirstItemInSlot(EquipmentSlotType type, bool lazy) const
{
	if (lazy)
	{
		if (type == EquipmentSlotType::RightHand)
			return rightHandItem;
		if (type == EquipmentSlotType::LeftHand)
			return leftHandItem;
	}
	for (auto &e : equipment)
	{
		for (auto &s : this->type->equipment_layout->slots)
		{
			if (s.bounds.p0 == e->equippedPosition && s.type == type)
			{
				return e;
			}
		}
	}
	return nullptr;
}

sp<AEquipment> Agent::getFirstItemByType(StateRef<AEquipmentType> type) const
{
	for (auto &e : equipment)
	{
		if (e->type == type)
		{
			return e;
		}
	}
	return nullptr;
}

sp<AEquipment> Agent::getFirstItemByType(AEquipmentType::Type type) const
{
	for (auto &e : equipment)
	{
		if (e->type->type == type)
		{
			return e;
		}
	}
	return nullptr;
}

sp<AEquipment> Agent::getFirstShield() const
{
	for (auto &e : equipment)
	{
		if (e->type->type == AEquipmentType::Type::DisruptorShield)
		{
			return e;
		}
	}
	return nullptr;
}

StateRef<BattleUnitImagePack> Agent::getImagePack(BodyPart bodyPart) const
{
	EquipmentSlotType slotType = AgentType::getArmorSlotType(bodyPart);

	auto e = getFirstItemInSlot(slotType);
	if (e)
		return e->type->body_image_pack;
	auto it = type->image_packs[appearance].find(bodyPart);
	if (it != type->image_packs[appearance].end())
		return it->second;
	return nullptr;
}

sp<Equipment> Agent::getEquipmentAt(const Vec2<int> &position) const
{
	Vec2<int> slotPosition = {0, 0};
	for (auto &slot : this->type->equipment_layout->slots)
	{
		if (slot.bounds.within(position))
		{
			slotPosition = slot.bounds.p0;
		}
	}
	// Find whatever equipped in the slot itself
	for (auto &eq : this->equipment)
	{
		if (eq->equippedPosition == slotPosition)
		{
			return eq;
		}
	}
	// Find whatever occupies this space
	for (auto &eq : this->equipment)
	{
		Rect<int> eqBounds{eq->equippedPosition, eq->equippedPosition + eq->type->equipscreen_size};
		if (eqBounds.within(slotPosition))
		{
			return eq;
		}
	}
	return nullptr;
}

const std::list<EquipmentLayoutSlot> &Agent::getSlots() const
{
	return this->type->equipment_layout->slots;
}

std::list<std::pair<Vec2<int>, sp<Equipment>>> Agent::getEquipment() const
{
	std::list<std::pair<Vec2<int>, sp<Equipment>>> equipmentList;

	for (auto &equipmentObject : this->equipment)
	{
		equipmentList.emplace_back(
		    std::make_pair(equipmentObject->equippedPosition, equipmentObject));
	}

	return equipmentList;
}

int Agent::getMaxHealth() const { return current_stats.health; }

int Agent::getHealth() const { return modified_stats.health; }

int Agent::getMaxShield() const
{
	int maxShield = 0;

	for (auto &e : equipment)
	{
		if (e->type->type != AEquipmentType::Type::DisruptorShield)
			continue;
		maxShield += e->type->max_ammo;
	}

	return maxShield;
}

int Agent::getShield() const
{
	int curShield = 0;

	for (auto &e : equipment)
	{
		if (e->type->type != AEquipmentType::Type::DisruptorShield)
			continue;
		curShield += e->ammo;
	}

	return curShield;
}

void Agent::destroy()
{
	leftHandItem = nullptr;
	rightHandItem = nullptr;
	while (!equipment.empty())
	{
		GameState state;
		this->removeEquipment(state, equipment.front());
	}
}

BodyState AgentBodyType::getFirstAllowedState()
{
	auto bodyState = BodyState::Standing;
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		bodyState = BodyState::Flying;
	}
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		bodyState = BodyState::Kneeling;
	}
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		bodyState = BodyState::Prone;
	}
	if (allowed_body_states.find(bodyState) == allowed_body_states.end())
	{
		LogError("Body type cannot Stand, Fly, Kneel or go Prone! WTF!?");
	}
	return bodyState;
}

} // namespace OpenApoc
