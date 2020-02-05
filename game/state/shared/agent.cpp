#include "game/state/shared/agent.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

static const unsigned TICKS_PER_PHYSICAL_TRAINING = 4 * TICKS_PER_HOUR;
static const unsigned TICKS_PER_PSI_TRAINING = 4 * TICKS_PER_HOUR;

template <> sp<Agent> StateObject<Agent>::get(const GameState &state, const UString &id)
{
	auto it = state.agents.find(id);
	if (it == state.agents.end())
	{
		LogError("No agent matching ID \"%s\"", id);
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

StateRef<Agent> AgentGenerator::createAgent(GameState &state, StateRef<Organisation> org,
                                            AgentType::Role role) const
{
	std::list<sp<AgentType>> types;
	for (auto &t : state.agent_types)
		if (t.second->role == role && t.second->playable)
			types.insert(types.begin(), t.second);
	auto type = pickRandom(state.rng, types);

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

		auto firstName = pickRandom(state.rng, firstNameList->second);
		auto secondName = pickRandom(state.rng, this->second_names);
		agent->name = format("%s %s", firstName, secondName);
	}
	else
	{
		agent->name = type->name;
	}
	// FIXME: When rng is fixed we can remove this unnecessary kludge
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
	std::list<const AEquipmentType *> initialEquipment;
	if (type->inventory)
	{
		// Player gets no default equipment
		if (org == state.getPlayer())
		{
		}
		// Aliens get equipment based on player score
		else if (org == state.getAliens())
		{
			initialEquipment = EquipmentSet::getByScore(state, state.totalScore.tacticalMissions)
			                       ->generateEquipmentList(state);
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
		initialEquipment.push_back(type->built_in_weapon_right.get());
		initialEquipment.push_back(type->built_in_weapon_left.get());
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

/**
 * Get relevant skill.
 */
int Agent::getSkill() const
{
	int skill = 0;
	switch (type->role)
	{
		case AgentType::Role::Physicist:
			skill = current_stats.physics_skill;
			break;
		case AgentType::Role::BioChemist:
			skill = current_stats.biochem_skill;
			break;
		case AgentType::Role::Engineer:
			skill = current_stats.engineering_skill;
			break;
	}

	return skill;
}

void Agent::leaveBuilding(GameState &state, Vec3<float> initialPosition)
{
	if (currentBuilding)
	{
		currentBuilding->currentAgents.erase({&state, shared_from_this()});
		currentBuilding = nullptr;
	}
	if (currentVehicle)
	{
		currentVehicle->currentAgents.erase({&state, shared_from_this()});
		currentVehicle = nullptr;
	}
	position = initialPosition;
	goalPosition = initialPosition;
}

void Agent::enterBuilding(GameState &state, StateRef<Building> b)
{
	if (currentVehicle)
	{
		currentVehicle->currentAgents.erase({&state, shared_from_this()});
		currentVehicle = nullptr;
	}
	currentBuilding = b;
	currentBuilding->currentAgents.insert({&state, shared_from_this()});
	position = (Vec3<float>)b->crewQuarters + Vec3<float>{0.5f, 0.5f, 0.5f};
	goalPosition = position;
}

void Agent::enterVehicle(GameState &state, StateRef<Vehicle> v)
{
	if (v->getPassengers() >= v->getMaxPassengers())
	{
		return;
	}
	if (currentBuilding)
	{
		currentBuilding->currentAgents.erase({&state, shared_from_this()});
		currentBuilding = nullptr;
	}
	if (currentVehicle)
	{
		currentVehicle->currentAgents.erase({&state, shared_from_this()});
		currentVehicle = nullptr;
	}
	currentVehicle = v;
	currentVehicle->currentAgents.insert({&state, shared_from_this()});
}

bool Agent::canTeleport() const
{
	return teleportTicksAccumulated >= TELEPORT_TICKS_REQUIRED_AGENT;
}

bool Agent::hasTeleporter() const
{
	for (auto &e : this->equipment)
	{
		if (e->type->type != AEquipmentType::Type::Teleporter)
			continue;
		return true;
	}

	return false;
}

void Agent::assignTraining(TrainingAssignment assignment)
{
	if (type->role == AgentType::Role::Soldier && type->canTrain)
	{
		trainingAssignment = assignment;
	}
}

void Agent::hire(GameState &state, StateRef<Building> newHome)
{
	owner = newHome->owner;
	homeBuilding = newHome;
	recentlyHired = true;
	setMission(state, AgentMission::gotoBuilding(state, *this, newHome, false, true));
}

void Agent::transfer(GameState &state, StateRef<Building> newHome)
{
	homeBuilding = newHome;
	recentlyHired = false;
	recentryTransferred = true;
	assigned_to_lab = false;
	setMission(state, AgentMission::gotoBuilding(state, *this, newHome, false, true));
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

bool Agent::canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> equipmentType) const
{
	EquipmentSlotType slotType = EquipmentSlotType::General;
	return canAddEquipment(pos, equipmentType, slotType);
}

bool Agent::canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> equipmentType,
                            EquipmentSlotType &slotType) const
{
	Vec2<int> slotOrigin;
	bool slotFound = false;
	bool slotIsArmor = false;
	// Check the slot this occupies hasn't already got something there
	for (auto &slot : type->equipment_layout->slots)
	{
		if (slot.bounds.within(pos))
		{
			// If we are equipping into an armor slot, ensure the item being equipped is correct
			// armor
			if (isArmorEquipmentSlot(slot.type))
			{
				if (equipmentType->type != AEquipmentType::Type::Armor)
				{
					break;
				}
				if (AgentType::getArmorSlotType(equipmentType->body_part) != slot.type)
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
		// We still have to check that slot itself isn't full
		for (auto &otherEquipment : this->equipment)
		{
			// Something is already in that slot, fail
			if (otherEquipment->equippedPosition == slotOrigin)
			{
				return false;
			}
		}

		// This could would check that every slot is of appropriate type,
		// But for units armor this is unnecessary
		/*
		for (int y = 0; y < type->equipscreen_size.y; y++)
		{
		    for (int x = 0; x < type->equipscreen_size.x; x++)
		    {
		        Vec2<int> slotPos = { x, y };
		        slotPos += pos;
		        bool validSlot = false;
		        for (auto &slot : type->equipment_layout->slots)
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
		Rect<int> bounds{pos, pos + equipmentType->equipscreen_size};
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
		for (int y = 0; y < equipmentType->equipscreen_size.y; y++)
		{
			for (int x = 0; x < equipmentType->equipscreen_size.x; x++)
			{
				Vec2<int> slotPos = {x, y};
				slotPos += pos;
				bool validSlot = false;
				for (auto &slot : type->equipment_layout->slots)
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
Vec2<int> Agent::findFirstSlotByType(EquipmentSlotType slotType,
                                     StateRef<AEquipmentType> equipmentType)
{
	Vec2<int> pos = {-1, 0};
	for (auto &slot : type->equipment_layout->slots)
	{
		if (slot.type == slotType &&
		    (!equipmentType || canAddEquipment(slot.bounds.p0, equipmentType)))
		{
			pos = slot.bounds.p0;
			break;
		}
	}
	return pos;
}

Vec2<int> Agent::findFirstSlot(StateRef<AEquipmentType> equipmentType)
{
	Vec2<int> pos = {-1, 0};
	for (auto &slot : type->equipment_layout->slots)
	{
		if (canAddEquipment(slot.bounds.p0, equipmentType))
		{
			pos = slot.bounds.p0;
			break;
		}
	}
	return pos;
}

sp<AEquipment> Agent::addEquipmentByType(GameState &state, StateRef<AEquipmentType> equipmentType,
                                         bool allowFailure)
{
	Vec2<int> pos;
	bool slotFound = false;
	EquipmentSlotType prefSlotType = EquipmentSlotType::General;
	bool prefSlot = false;
	if (equipmentType->type == AEquipmentType::Type::Ammo)
	{
		auto wpn = addEquipmentAsAmmoByType(equipmentType);
		if (wpn)
		{
			return wpn;
		}
		prefSlotType = EquipmentSlotType::General;
		prefSlot = true;
	}
	else if (equipmentType->type == AEquipmentType::Type::Armor)
	{
		switch (equipmentType->body_part)
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
		for (auto &slot : type->equipment_layout->slots)
		{
			if (slot.type != prefSlotType)
			{
				continue;
			}
			if (canAddEquipment(slot.bounds.p0, equipmentType))
			{
				pos = slot.bounds.p0;
				slotFound = true;
				break;
			}
		}
	}
	if (!slotFound)
	{
		for (auto &slot : type->equipment_layout->slots)
		{
			if (canAddEquipment(slot.bounds.p0, equipmentType))
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
			LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found",
			         equipmentType.id, this->name);
		}
		return nullptr;
	}

	return addEquipmentByType(state, pos, equipmentType);
}

sp<AEquipment> Agent::addEquipmentByType(GameState &state, StateRef<AEquipmentType> equipmentType,
                                         EquipmentSlotType slotType, bool allowFailure)
{
	if (equipmentType->type == AEquipmentType::Type::Ammo)
	{
		auto wpn = addEquipmentAsAmmoByType(equipmentType);
		if (wpn)
		{
			return wpn;
		}
	}
	Vec2<int> pos = findFirstSlotByType(slotType, equipmentType);
	if (pos.x == -1)
	{
		if (!allowFailure)
		{
			LogError("Trying to add \"%s\" on agent \"%s\" failed: no valid slot found",
			         equipmentType.id, this->name);
		}
		return nullptr;
	}

	return addEquipmentByType(state, pos, equipmentType);
}

sp<AEquipment> Agent::addEquipmentByType(GameState &state, Vec2<int> pos,
                                         StateRef<AEquipmentType> equipmentType)
{
	auto equipment = mksp<AEquipment>();
	equipment->type = equipmentType;
	equipment->armor = equipmentType->armor;
	if (equipmentType->ammo_types.empty())
	{
		equipment->ammo = equipmentType->max_ammo;
	}
	this->addEquipment(state, pos, equipment);
	if (equipmentType->ammo_types.size() > 0)
	{
		equipment->loadAmmo(state);
	}
	return equipment;
}

sp<AEquipment> Agent::addEquipmentAsAmmoByType(StateRef<AEquipmentType> equipmentType)
{
	for (auto &e : equipment)
	{
		if (e->type->type == AEquipmentType::Type::Weapon && e->ammo == 0 &&
		    std::find(e->type->ammo_types.begin(), e->type->ammo_types.end(), equipmentType) !=
		        e->type->ammo_types.end())
		{
			e->payloadType = equipmentType;
			e->ammo = equipmentType->max_ammo;
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
	for (auto &slot : type->equipment_layout->slots)
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
	updateIsBrainsucker();
	if (unit)
	{
		unit->updateDisplayedItem(state);
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
		unit->updateDisplayedItem(state);
	}
	object->ownerAgent.clear();
	updateSpeed();
	updateIsBrainsucker();
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
	modified_stats.speed = std::max(
	    8, ((strength + encumbrance) / 2 + current_stats.speed * (strength - encumbrance)) /
	           (strength + encumbrance));
}

void Agent::updateModifiedStats()
{
	int health = modified_stats.health;
	modified_stats = current_stats;
	modified_stats.health = health;
	updateSpeed();
}

void Agent::updateIsBrainsucker()
{
	isBrainsucker = false;
	for (auto &e : equipment)
	{
		if (e->type->type == AEquipmentType::Type::Brainsucker)
		{
			isBrainsucker = true;
			return;
		}
	}
}

bool Agent::addMission(GameState &state, AgentMission *mission, bool toBack)
{
	if (missions.empty() || !toBack)
	{
		missions.emplace_front(mission);
		missions.front()->start(state, *this);
	}
	else
	{
		missions.emplace_back(mission);
	}
	return true;
}

bool Agent::setMission(GameState &state, AgentMission *mission)
{
	for (auto &m : this->missions)
	{
		// if we're removing an InvestigateBuilding mission
		// decrease the investigate count so the other investigating vehicles won't dangle
		if (m->type == AgentMission::MissionType::InvestigateBuilding)
		{
			if (!m->isFinished(state, *this))
			{
				m->targetBuilding->decreasePendingInvestigatorCount(state);
			}
		}
	}
	missions.clear();
	missions.emplace_front(mission);
	missions.front()->start(state, *this);
	return true;
}

bool Agent::popFinishedMissions(GameState &state)
{
	bool popped = false;
	while (missions.size() > 0 && missions.front()->isFinished(state, *this))
	{
		LogWarning("Agent %s mission \"%s\" finished", name, missions.front()->getName());
		missions.pop_front();
		popped = true;
		if (!missions.empty())
		{
			LogWarning("Agent %s mission \"%s\" starting", name, missions.front()->getName());
			missions.front()->start(state, *this);
			continue;
		}
		else
		{
			LogWarning("No next agent mission, going idle");
			break;
		}
	}
	return popped;
}

bool Agent::getNewGoal(GameState &state)
{
	bool popped = false;
	bool acquired = false;
	Vec3<float> nextGoal;
	// Pop finished missions if present
	popped = popFinishedMissions(state);
	do
	{
		// Try to get new destination
		if (!missions.empty())
		{
			acquired = missions.front()->getNextDestination(state, *this, goalPosition);
		}
		// Pop finished missions if present
		popped = popFinishedMissions(state);
	} while (popped && !acquired);
	return acquired;
}

void Agent::die(GameState &state, bool silent)
{
	auto thisRef = StateRef<Agent>{&state, shared_from_this()};
	// Actually die
	modified_stats.health = 0;
	// Remove from lab
	if (assigned_to_lab)
	{
		for (auto &fac : homeBuilding->base->facilities)
		{
			if (!fac->lab)
			{
				continue;
			}
			auto it = std::find(fac->lab->assigned_agents.begin(), fac->lab->assigned_agents.end(),
			                    thisRef);
			if (it != fac->lab->assigned_agents.end())
			{
				fac->lab->assigned_agents.erase(it);
				assigned_to_lab = false;
				break;
			}
		}
	}
	// Remove from building
	if (currentBuilding)
	{
		currentBuilding->currentAgents.erase(thisRef);
	}
	// Remove from vehicle
	if (currentVehicle)
	{
		currentVehicle->currentAgents.erase(thisRef);
	}
	// In city we remove agent
	if (!state.current_battle)
	{
		// In city (if not died in a vehicle) we make an event
		if (!silent && owner == state.getPlayer())
		{
			fw().pushEvent(
			    new GameSomethingDiedEvent(GameEventType::AgentDiedCity, name, "", position));
		}
		state.agents.erase(getId(state, shared_from_this()));
	}
}

bool Agent::isDead() const { return getHealth() <= 0; }

void Agent::update(GameState &state, unsigned ticks)
{
	if (isDead() || !city)
	{
		return;
	}

	if (teleportTicksAccumulated < TELEPORT_TICKS_REQUIRED_VEHICLE)
	{
		teleportTicksAccumulated += ticks;
	}
	if (!hasTeleporter())
	{
		teleportTicksAccumulated = 0;
	}

	// Agents in vehicles don't update missions and don't move
	if (!currentVehicle)
	{
		if (!this->missions.empty())
		{
			this->missions.front()->update(state, *this, ticks);
		}
		popFinishedMissions(state);
		updateMovement(state, ticks);
	}
}

void Agent::updateEachSecond(GameState &state)
{
	if (type->role != AgentType::Role::Soldier && currentBuilding != homeBuilding &&
	    missions.empty())
	{
		setMission(state, AgentMission::gotoBuilding(state, *this));
	}
}

void Agent::updateDaily(GameState &state [[maybe_unused]]) { recentlyFought = false; }

void Agent::updateHourly(GameState &state)
{
	StateRef<Base> base;
	if (currentBuilding == homeBuilding)
	{
		// agent is in home building
		base = currentBuilding->base;
	}
	else if (currentVehicle && currentVehicle->currentBuilding == homeBuilding)
	{
		// agent is in a vehicle stationed in home building
		base = currentVehicle->currentBuilding->base;
	}

	if (!base)
	{
		// not in a base
		return;
	}

	// Heal
	if (modified_stats.health < current_stats.health && !recentlyFought)
	{
		int usage = base->getUsage(state, FacilityType::Capacity::Medical);
		if (usage < 999)
		{
			usage = std::max(100, usage);
			// As per Roger Wong's guide, healing is 0.8 points an hour
			healingProgress += 80.0f / (float)usage;
			if (healingProgress > 1.0f)
			{
				healingProgress -= 1.0f;
				modified_stats.health++;
			}
		}
	}
	// Train
	if (trainingAssignment != TrainingAssignment::None)
	{
		int usage = base->getUsage(state, trainingAssignment == TrainingAssignment::Physical
		                                      ? FacilityType::Capacity::Training
		                                      : FacilityType::Capacity::Psi);
		if (usage < 999)
		{
			usage = std::max(100, usage);
			// As per Roger Wong's guide
			float mult = config().getFloat("OpenApoc.Cheat.StatGrowthMultiplier");
			if (trainingAssignment == TrainingAssignment::Physical)
			{
				trainPhysical(state, TICKS_PER_HOUR * 100 / usage * mult);
			}
			else
			{
				trainPsi(state, TICKS_PER_HOUR * 100 / usage * mult);
			}
		}
	}
}

void Agent::updateMovement(GameState &state, unsigned ticks)
{
	auto ticksToMove = ticks;
	unsigned lastTicksToMove = 0;

	// See that we're not in the air
	if (!city->map->getTile(position)->presentScenery)
	{
		die(state);
		return;
	}

	// Move until we become idle or run out of ticks
	while (ticksToMove != lastTicksToMove)
	{
		lastTicksToMove = ticksToMove;

		// Advance agent position to goal
		if (ticksToMove > 0 && position != goalPosition)
		{
			Vec3<float> vectorToGoal = goalPosition - position;
			int distanceToGoal = (unsigned)ceilf(glm::length(
			    vectorToGoal * VELOCITY_SCALE_CITY * (float)TICKS_PER_UNIT_TRAVELLED_AGENT));

			// Cannot reach in one go
			if (distanceToGoal > ticksToMove)
			{
				auto dir = glm::normalize(vectorToGoal);
				Vec3<float> newPosition = (float)(ticksToMove)*dir;
				newPosition /= VELOCITY_SCALE_BATTLE;
				newPosition /= (float)TICKS_PER_UNIT_TRAVELLED_AGENT;
				newPosition += position;
				position = newPosition;
				ticksToMove = 0;
			}
			// Can reach in one go
			else
			{
				ticksToMove -= distanceToGoal;
				position = goalPosition;
			}
		}

		// Request new goal
		if (position == goalPosition)
		{
			if (!getNewGoal(state))
			{
				break;
			}
		}
	}
}

void Agent::trainPhysical(GameState &state, unsigned ticks)
{
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
		// Therefore, we'll use a formula that makes sense and follows what he said.
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

StateRef<AEquipmentType> Agent::getDominantItemInHands(GameState &state,
                                                       StateRef<AEquipmentType> itemLastFired) const
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
	        : (e1->canFire(state) ? 4
	                              : (e1->type->two_handed
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
	        : (e2->canFire(state) ? 4
	                              : (e2->type->two_handed
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

sp<AEquipment> Agent::getFirstItemInSlot(EquipmentSlotType equipmentSlotType, bool lazy) const
{
	if (lazy)
	{
		if (equipmentSlotType == EquipmentSlotType::RightHand)
			return rightHandItem;
		if (equipmentSlotType == EquipmentSlotType::LeftHand)
			return leftHandItem;
	}
	for (auto &e : equipment)
	{
		for (auto &s : type->equipment_layout->slots)
		{
			if (s.bounds.p0 == e->equippedPosition && s.type == equipmentSlotType)
			{
				return e;
			}
		}
	}
	return nullptr;
}

sp<AEquipment> Agent::getFirstItemByType(StateRef<AEquipmentType> equipmentType) const
{
	for (auto &e : equipment)
	{
		if (e->type == equipmentType)
		{
			return e;
		}
	}
	return nullptr;
}

sp<AEquipment> Agent::getFirstItemByType(AEquipmentType::Type itemType) const
{
	for (auto &e : equipment)
	{
		if (e->type->type == itemType)
		{
			return e;
		}
	}
	return nullptr;
}

sp<AEquipment> Agent::getFirstShield(GameState &state) const
{
	for (auto &e : equipment)
	{
		if (e->type->type == AEquipmentType::Type::DisruptorShield && e->canBeUsed(state))
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
	for (auto &slot : type->equipment_layout->slots)
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
	return type->equipment_layout->slots;
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

int Agent::getMaxShield(GameState &state) const
{
	int maxShield = 0;

	for (auto &e : equipment)
	{
		if (e->type->type != AEquipmentType::Type::DisruptorShield || !e->canBeUsed(state))
			continue;
		maxShield += e->type->max_ammo;
	}

	return maxShield;
}

int Agent::getShield(GameState &state) const
{
	int curShield = 0;

	for (auto &e : equipment)
	{
		if (e->type->type != AEquipmentType::Type::DisruptorShield || !e->canBeUsed(state))
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

} // namespace OpenApoc
