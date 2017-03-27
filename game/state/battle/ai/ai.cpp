#include "game/state/battle/ai/ai.h"
#include "game/state/aequipment.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

namespace
{
static const std::set<TileObject::Type> mapPartSet = {
    TileObject::Type::Ground, TileObject::Type::LeftWall, TileObject::Type::RightWall,
    TileObject::Type::Feature};
static const std::set<TileObject::Type> unitSet = {TileObject::Type::Unit};
static const std::tuple<AIDecision, bool> NULLTUPLE2 = std::make_tuple(AIDecision(), false);
static const std::tuple<AIDecision, float, unsigned> NULLTUPLE3 =
    std::make_tuple(AIDecision(), FLT_MIN, 0);
}

/* AI LOGIC: attack()

options (checked for every visible target):
- stand still and shoot at target with weapon
  (advance to target's location while shoooting at it)
- run forward to get in range for firing a weapon
- throw a grenade
- run forward to get in range for a greande throw
- use psi attack

- calculate priority for each action
- pick action with highest priority
- if "shoot at target" action wins then:
  - advance if CTH is low, stand still if high, with some added random variance

*/

// AI Logic for attacking:
//
// We go through every possibile tool:
// - first mindbender encountered if we have it
// - every weapon with loaded ammo
// - every grenade
//
// We then consider for every visible enemy:
//
// 1) Power, with resistance taken into consideration
// - For typical attacks, this will calculate avg damage done (using spread, armor, resist)
//   (Body will be used when body part hit is unknown)
// - This is then divided by unit's HP, resulting in percentage of HP taken by attack
// - For panic/mc, resultant value is hardcoded 0.5/1.0o
// - For psi stun, resultant value is just the avg stun damage done divided by HP
//
// 2) Collateral damage, for explosives we consider units in a radius of 4,
//   and simply subtract depletion rate multiplied by distane to them,
//   and repeat calculations above and add (hostile) or subtract (friendly)
//
// 3) Chance to hit, which will be determined in some simplified approximated way
// - For guided this will be 100%
// - For grenades this will be a hardcoded 80% since it's complicated to calculate all that
//   (considering a grenade can miss but still deal splash damage, and usually does so)
// - For psi this is the attack's success chance
//
// 4) Payload delivery delay, this is sum of:
// - getting in range at run speed delay
// - firing snap shot delay
// - possibly some fixed value for psionic attack
// - (RT only) projectile speed delay (or item speed, whichever)
//     Item has XY velocity of 2.0f, which is multiplied by 24, thus it's XY speed is 48
//     Projectile speed is already in tile scale, and has a multiplier of 8
//     Therefore, item can be considered to have a projectile speed of 48/8 = 6
//	   Then just divide distance by velocity (scaled) to get ticks
// - every second delay divides value gotten above by 2

// Delay before unit will turn automatically again after doing it once
static const unsigned AUTO_TURN_COOLDOWN = TICKS_PER_TURN;
// Delay before unit will target an enemy automatically again after failing to do so once
static const unsigned AUTO_TARGET_COOLDOWN = TICKS_PER_TURN / 4;

AIAction::AIAction() : weaponStatus(WeaponStatus::NotFiring) {}

AIMovement::AIMovement() : movementMode(MovementMode::Walking), kneelingMode(KneelingMode::None) {}

AIDecision::AIDecision(sp<AIAction> action, sp<AIMovement> movement)
    : action(action), movement(movement)
{
}

LowMoraleUnitAI::LowMoraleUnitAI() { type = Type::LowMorale; }
DefaultUnitAI::DefaultUnitAI() { type = Type::Default; }
BehaviorUnitAI::BehaviorUnitAI() { type = Type::Behavior; }
VanillaUnitAI::VanillaUnitAI() { type = Type::Vanilla; }
HardcoreUnitAI::HardcoreUnitAI() { type = Type::Hardcore; }
VanillaTacticalAI::VanillaTacticalAI() { type = Type::Vanilla; }

const UString TacticalAI::getName()
{
	switch (type)
	{
		case Type::Vanilla:
			return "VanillaTacticalAI";
	}
	LogError("Unimplemented getName for Tactical AI Type %d", (int)type);
	return "";
}
const UString UnitAI::getName()
{
	switch (type)
	{
		case Type::LowMorale:
			return "LowMoraleUnitAI";
		case Type::Default:
			return "DefaultUnitAI";
		case Type::Behavior:
			return "BehaviorUnitAI";
		case Type::Vanilla:
			return "VanillaUnitAI";
		case Type::Hardcore:
			return "HardcoreUnitAI";
	}
	LogError("Unimplemented getName for Unit AI Type %d", (int)type);
	return "";
}

bool AIDecision::isEmpty() { return !action && !movement; }

void UnitAIList::init(GameState &state, BattleUnit &u)
{
	// FIXME: Actually read this option
	bool USER_OPTION_USE_HARDCORE_AI = false;

	aiList.clear();
	aiList.push_back(mksp<LowMoraleUnitAI>());
	aiList.push_back(mksp<DefaultUnitAI>());
	aiList.push_back(mksp<BehaviorUnitAI>());
	// Even player units have AI because when they're taken control of they will need that
	if (USER_OPTION_USE_HARDCORE_AI)
	{
		aiList.push_back(mksp<HardcoreUnitAI>());
	}
	else
	{
		aiList.push_back(mksp<VanillaUnitAI>());
	}

	reset(state, u);
}

void UnitAIList::reset(GameState &state, BattleUnit &u)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = randBoundsExclusive(state.rng, 0, (int)TICKS_PER_SECOND);

	for (auto &ai : aiList)
	{
		ai->reset(state, u);
	}
}

AIDecision UnitAIList::think(GameState &state, BattleUnit &u)
{
	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilReThink > curTicks)
	{
		return {};
	}

	ticksLastThink = curTicks;
	ticksUntilReThink = AI_THINK_INTERVAL;

	AIDecision decision = {};
	for (auto ai : aiList)
	{
		auto result = ai->think(state, u);
		auto newDecision = std::get<0>(result);
		auto halt = std::get<1>(result);
		if (!newDecision.isEmpty())
		{
			// We can keep last decision's movement if this one is action only,
			// because that would mean this decision may be possible to be done on the move
			// We cannot take last decision's action because it might override
			// our move or maulfunction otherwise
			if (!newDecision.movement && decision.movement)
			{
				newDecision.movement = decision.movement;
			}
			decision = newDecision;
			decision.ai = ai->getName();
		}
		if (halt)
		{
			break;
		}
	}
	return decision;
}

void UnitAIList::notifyUnderFire(Vec3<int> position)
{
	for (auto &ai : aiList)
	{
		if (!ai->active)
		{
			continue;
		}
		ai->notifyUnderFire(position);
	}
}

void UnitAIList::notifyHit(Vec3<int> position)
{
	for (auto &ai : aiList)
	{
		if (!ai->active)
		{
			continue;
		}
		ai->notifyHit(position);
	}
}

void UnitAIList::notifyEnemySpotted(Vec3<int> position)
{
	for (auto &ai : aiList)
	{
		if (!ai->active)
		{
			continue;
		}
		ai->notifyEnemySpotted(position);
	}
}

// FIXME: Allow for patrol to a point other than block's center
std::tuple<std::list<StateRef<BattleUnit>>, sp<AIMovement>>
VanillaTacticalAI::getPatrolMovement(GameState &state, BattleUnit &u)
{
	static float SQUAD_RANGE = 10.0f;

	auto result = mksp<AIMovement>();
	std::list<StateRef<BattleUnit>> units = {};
	units.emplace_back(&state, u.id);

	// If we have group AI - collect other units within range
	if (u.getAIType() == AIType::Group && u.agent->type->allowsDirectControl)
	{
		// Collect all units within squad range
		auto sft = u.shared_from_this();
		for (auto &unit : state.current_battle->forces[u.owner].squads[u.squadNumber].units)
		{
			if (unit != sft && unit->isConscious() && unit->getAIType() == AIType::Group &&
			    unit->agent->type->allowsDirectControl && unit->canMove() &&
			    unit->visibleEnemies.empty() &&
			    glm::distance(unit->position, u.position) < SQUAD_RANGE)
			{
				// If unit is busy and moving to a point within range - wait for him
				if (unit->isBusy())
				{
					if (!unit->missions.empty())
					{
						auto &m = unit->missions.front();
						if (m->type == BattleUnitMission::Type::GotoLocation &&
						    glm::distance((Vec3<float>)m->targetLocation, u.position) < SQUAD_RANGE)
						{
							units.clear();
							return std::make_tuple(units, result);
						}
					}
				}
				else
				{
					units.emplace_back(&state, unit->id);
				}
			}
		}
	}

	int maxIterations = 50;
	int iterationCount = 0;

	while (iterationCount++ < maxIterations)
	{
		auto lbID = vectorRandomizer(state.rng, state.current_battle->losBlockRandomizer);

		// Make sure every unit can go there
		bool unavailable = false;
		for (auto &unit : units)
		{
			if (!state.current_battle->blockAvailable[unit->getType()][lbID])
			{
				unavailable = true;
				break;
			}
		}
		if (unavailable)
		{
			continue;
		}

		// Go there actually
		// auto &lb = *state.current_battle->losBlocks.at(lbID); // <-- not needed yet?
		result->type = AIMovement::Type::Patrol;
		result->targetLocation = state.current_battle->blockCenterPos[u.getType()][lbID];
		bool canRun = true;
		for (auto unit : units)
		{
			if (!unit->agent->isMovementStateAllowed(MovementState::Running))
			{
				canRun = false;
				break;
			}
		}
		result->movementMode = canRun ? MovementMode::Running : MovementMode::Walking;
		break;
	}

	return std::make_tuple(units, result);
}

sp<AIMovement> UnitAIHelper::getRetreatMovement(GameState &state, BattleUnit &u, bool forced)
{
	// Chance to take retreat is 1% per morale point below 20
	if (!forced &&
	    randBoundsExclusive(state.rng, 0, 100) >= std::max(0, 20 - u.agent->modified_stats.morale))
	{
		return nullptr;
	}

	LogWarning("Implement retreat (for now kneeling instead)");

	if (!u.agent->isBodyStateAllowed(BodyState::Kneeling))
	{
		return nullptr;
	}

	auto result = mksp<AIMovement>();
	result->type = AIMovement::Type::Stop;
	result->kneelingMode = KneelingMode::Kneeling;
	result->movementMode = MovementMode::Walking;

	return result;
}

sp<AIMovement> UnitAIHelper::getTakeCoverMovement(GameState &state, BattleUnit &u, bool forced)
{
	// Chance to take cover is 33% * sqrt(num_enemies_seen), if no one is seen then assume 3
	if (!forced)
	{
		if (randBoundsExclusive(state.rng, 0, 100) >=
		    33.0f * sqrtf(u.visibleEnemies.empty() ? 3 : (int)u.visibleEnemies.size()))
		{
			return nullptr;
		}
		// If already prone - ignore (for now, until we implement proper take cover)
		if (u.movement_mode == MovementMode::Prone)
		{
			return nullptr;
		}
	}

	LogWarning("Implement take cover (for now proning instead)");

	if (!u.agent->isBodyStateAllowed(BodyState::Prone))
	{
		return nullptr;
	}

	auto result = mksp<AIMovement>();
	result->type = AIMovement::Type::ChangeStance;
	result->movementMode = MovementMode::Prone;
	result->kneelingMode = KneelingMode::None;

	return result;
}

sp<AIMovement> UnitAIHelper::getKneelMovement(GameState &state, BattleUnit &u, bool forced)
{
	if (!u.agent->isBodyStateAllowed(BodyState::Kneeling))
	{
		return nullptr;
	}

	if (!forced)
	{
		// Chance to kneel is 33% * sqrt(num_enemies_seen), if no one is seen then assume 3
		if (randBoundsExclusive(state.rng, 0, 100) >=
		    33.0f * sqrtf(u.visibleEnemies.empty() ? 3 : (int)u.visibleEnemies.size()))
		{
			return nullptr;
		}
		// If already kneeling or prone - ignore
		if (u.movement_mode == MovementMode::Prone || u.kneeling_mode == KneelingMode::Kneeling)
		{
			return nullptr;
		}
	}

	auto result = mksp<AIMovement>();
	result->type = AIMovement::Type::ChangeStance;
	result->kneelingMode = KneelingMode::Kneeling;
	result->movementMode = MovementMode::Walking;

	return result;
}

sp<AIMovement> UnitAIHelper::getTurnMovement(GameState &, BattleUnit &, Vec3<int> target)
{
	auto result = mksp<AIMovement>();

	result->type = AIMovement::Type::Turn;
	result->targetLocation = target;

	return result;
}

void UnitAIHelper::ensureItemInSlot(GameState &state, sp<AEquipment> item, AEquipmentSlotType slot)
{
	auto u = item->ownerAgent->unit;
	auto curItem = u->agent->getFirstItemInSlot(slot);
	if (curItem != item)
	{
		// Remove item in the desired slot
		if (curItem)
		{
			u->agent->removeEquipment(curItem);
		}

		// Remove item we will use and equip it in the desired slot
		u->agent->removeEquipment(item);
		u->agent->addEquipment(state, item, slot);

		// Equip back the item removed earlier
		if (curItem)
		{
			for (auto s : u->agent->type->equipment_layout->slots)
			{
				if (u->agent->canAddEquipment(s.bounds.p0, curItem->type))
				{
					u->agent->addEquipment(state, s.bounds.p0, curItem);
					curItem = nullptr;
					break;
				}
			}
		}

		// Drop the item if we couldn't equip it
		if (curItem)
		{
			u->addMission(state, BattleUnitMission::dropItem(*u, curItem));
		}
	}
}

sp<AIMovement> UnitAIHelper::getPursueMovement(GameState &state, BattleUnit &u, Vec3<int> target,
                                               bool forced)
{
	// Chance to pursuit is 1% per morale point above 20
	if (!forced &&
	    randBoundsExclusive(state.rng, 0, 100) >= std::max(0, u.agent->modified_stats.morale - 20))
	{
		return nullptr;
	}

	auto result = mksp<AIMovement>();

	result->type = AIMovement::Type::Pursue;
	result->targetLocation = target;

	return result;
}

// FIXME: IMPLEMENT PROPER WEAPON AI
std::tuple<AIDecision, float, unsigned>
VanillaUnitAI::getWeaponDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                                 StateRef<BattleUnit> target)
{
	if (u.canAttackUnit(target, e) != WeaponStatus::NotFiring)
	{
		auto action = mksp<AIAction>();
		action->item = e;
		action->targetUnit = target;
		action->type = AIAction::Type::AttackWeaponUnit;

		auto movement = mksp<AIMovement>();
		movement->type = AIMovement::Type::Stop;
		// Chance to advance is 50%
		if (randBoundsExclusive(state.rng, 0, 100) >= 50)
		{
			movement->type = AIMovement::Type::Advance;
			movement->targetLocation = target->position;
			movement->kneelingMode = KneelingMode::None;
			movement->movementMode = MovementMode::Walking;
		}

		unsigned reThinkDelay = TICKS_PER_TURN;
		float priority = e->getPayloadType()->damage;

		return std::make_tuple(AIDecision(action, movement), priority, reThinkDelay);
	}

	return NULLTUPLE3;
}

// FIXME: IMPLEMENT PSI AI
std::tuple<AIDecision, float, unsigned>
VanillaUnitAI::getPsiDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                              StateRef<BattleUnit> target, AIAction::Type type)
{
	std::ignore = state;
	std::ignore = u;
	std::ignore = e;
	std::ignore = target;
	std::ignore = type;

	return NULLTUPLE3;
}

// FIXME: IMPLEMENT GRENADE AI
std::tuple<AIDecision, float, unsigned>
VanillaUnitAI::getGrenadeDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                                  StateRef<BattleUnit> target)
{
	// Temporarily disable grenades
	return NULLTUPLE3;

	auto action = mksp<AIAction>();
	action->item = e;
	action->targetUnit = target;
	action->type = AIAction::Type::AttackGrenade;

	auto movement = mksp<AIMovement>();
	movement->type = AIMovement::Type::Stop;

	// Properly get rethingdelay based on how far we must run to reach throwable plint
	unsigned reThinkDelay = TICKS_PER_TURN * 2;
	// For now, auto-throw if in range at closest enemy
	float priority = FLT_MAX - glm::length(u.position - target->position);

	if (!e->getCanThrow(u, target->position))
	{
		// For now, just 30% chance to advance and throw anyway
		if (randBoundsExclusive(state.rng, 0, 100) >= 30)
		{
			priority = FLT_MAX / 2 - glm::length(u.position - target->position);
			movement->type = AIMovement::Type::GetInRange;
			movement->movementMode = MovementMode::Running;
			movement->targetLocation = target->position;
		}
	}

	return std::make_tuple(AIDecision(action, movement), priority, reThinkDelay);
}

std::tuple<AIDecision, float, unsigned> VanillaUnitAI::getAttackDecision(GameState &state,
                                                                         BattleUnit &u)
{
	std::tuple<AIDecision, float, unsigned> decision = NULLTUPLE3;

	// Try each item for a weapon/grenade/psi attack
	// If out of range - advance
	// If cannot attack at all - take cover!

	bool mindBenderEncountered = false;
	for (auto &e : u.agent->equipment)
	{
		switch (e->type->type)
		{
			case AEquipmentType::Type::Weapon:
				if (!e->canFire())
				{
					continue;
				}
				break;
			case AEquipmentType::Type::Grenade:
				break;
			case AEquipmentType::Type::MindBender:
				// Limit PSI attack calculations to one instance
				if (mindBenderEncountered)
				{
					continue;
				}
				else
				{
					mindBenderEncountered = true;
				}
				break;
			default:
				continue;
		}

		for (auto &target : u.visibleEnemies)
		{
			switch (e->type->type)
			{
				case AEquipmentType::Type::Weapon:
				{
					auto newDecision = getWeaponDecision(state, u, e, target);
					if (std::get<1>(newDecision) > std::get<1>(decision))
					{
						decision = newDecision;
					}
					break;
				}
				case AEquipmentType::Type::Grenade:
				{
					auto newDecision = getGrenadeDecision(state, u, e, target);
					if (std::get<1>(newDecision) > std::get<1>(decision))
					{
						decision = newDecision;
					}
					break;
				}
				case AEquipmentType::Type::MindBender:
				{
					auto newDecision =
					    getPsiDecision(state, u, e, target, AIAction::Type::AttackPsiMC);
					if (std::get<1>(newDecision) > std::get<1>(decision))
					{
						decision = newDecision;
					}
					newDecision =
					    getPsiDecision(state, u, e, target, AIAction::Type::AttackPsiPanic);
					if (std::get<1>(newDecision) > std::get<1>(decision))
					{
						decision = newDecision;
					}
					newDecision =
					    getPsiDecision(state, u, e, target, AIAction::Type::AttackPsiStun);
					if (std::get<1>(newDecision) > std::get<1>(decision))
					{
						decision = newDecision;
					}
					break;
				}
				default: // don't cry travis
					     // Nothing to do
					break;
			}
		}
	}
	// If cannot attack - take cover!
	if (std::get<0>(decision).isEmpty())
	{
		decision = std::make_tuple(
		    AIDecision(nullptr, UnitAIHelper::getTakeCoverMovement(state, u, true)), 0.0f, 0);
	}

	return decision;
}

// Calculate AI's next action in case the unit is not attacking
std::tuple<AIDecision, float, unsigned> VanillaUnitAI::thinkGreen(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	bool isMoving = lastDecision.movement && u.isMoving();
	bool isUnderAttack = attackerPosition != NONE;
	bool wasEnemyVisible = enemySpotted && lastSeenEnemyPosition != NONE;

	if (isMoving)
	{
		// If unit is moving into cover -> carry on
		if (lastDecision.movement->type == AIMovement::Type::TakeCover)
		{
			return NULLTUPLE3;
		}
	}

	if (isUnderAttack)
	{
		auto takeCover = UnitAIHelper::getTakeCoverMovement(state, u);
		if (takeCover)
		{
			return std::make_tuple(AIDecision(nullptr, takeCover), 0.0f, 0);
		}
	}

	if (wasEnemyVisible)
	{
		auto pursue = UnitAIHelper::getPursueMovement(state, u, (Vec3<int>)u.position +
		                                                            lastSeenEnemyPosition);
		if (pursue)
		{
			return std::make_tuple(AIDecision(nullptr, pursue), 0.0f, 0);
		}

		if (isMoving)
		{
			return NULLTUPLE3;
		}
	}

	if (isUnderAttack)
	{
		if (u.missions.empty() || u.missions.front()->type != BattleUnitMission::Type::Turn)
		{
			auto turn =
			    UnitAIHelper::getTurnMovement(state, u, (Vec3<int>)u.position + attackerPosition);
			return std::make_tuple(AIDecision(nullptr, turn), 0.0f, 0);
		}
	}

	return NULLTUPLE3;
}

// Calculate AI's next action in case enemies are seen
std::tuple<AIDecision, float, unsigned> VanillaUnitAI::thinkRed(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	bool isUnderAttack = attackerPosition != NONE;
	bool isInterrupted =
	    ticksUntilReThink > 0 && ticksLastThink + ticksUntilReThink > state.gameTime.getTicks();

	if (isUnderAttack)
	{
		auto takeCover = UnitAIHelper::getTakeCoverMovement(state, u);
		if (takeCover)
		{
			return std::make_tuple(AIDecision(nullptr, takeCover), 0.0f, 0);
		}

		auto kneel = UnitAIHelper::getKneelMovement(state, u);
		if (kneel)
		{
			return std::make_tuple(AIDecision(nullptr, kneel), 0.0f, 0);
		}

		if (isInterrupted)
		{
			return NULLTUPLE3;
		}
	}

	return getAttackDecision(state, u);
}

AIDecision VanillaUnitAI::thinkInternal(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	switch (u.getAIType())
	{
		case AIType::None:
			return {};
		case AIType::Civilian:
			LogWarning("Implement Civilian Unit AI");
			return {};
		case AIType::PanicFreeze:
		case AIType::PanicRun:
		case AIType::Berserk:
			LogError("Calling VanillaUnitAI on panic/berserk unit!?");
			return {};
		case AIType::Loner:
		case AIType::Group:
			// Go on below
			break;
	}

	// Clear actions that are outdated
	if (lastDecision.action && lastDecision.action->targetUnit &&
	    !lastDecision.action->targetUnit->isConscious())
	{
		lastDecision.action = nullptr;
	}
	// Clear actions that are done
	if (lastDecision.action)
	{
		switch (lastDecision.action->type)
		{
			case AIAction::Type::AttackGrenade:
				if (!u.missions.empty() &&
				    u.missions.front()->type == BattleUnitMission::Type::ThrowItem)
				{
					lastDecision.action = nullptr;
				}
				break;
			case AIAction::Type::AttackPsiMC:
			case AIAction::Type::AttackPsiPanic:
			case AIAction::Type::AttackPsiStun:
				// FIXME: Introduce PSI condition
				if (false)
				{
					lastDecision.action = nullptr;
				}
				break;
			case AIAction::Type::AttackWeaponUnit:
			case AIAction::Type::AttackWeaponTile:
				if (u.isAttacking())
				{
					lastDecision.action = nullptr;
				}
				break;
		}
	}

	// Clear movement that is done
	if (lastDecision.movement)
	{
		switch (lastDecision.movement->type)
		{
			case AIMovement::Type::Stop:
				if (!u.isMoving())
				{
					lastDecision.movement = nullptr;
				}
				break;
			case AIMovement::Type::Turn:
				if (u.isDoing(BattleUnitMission::Type::Turn))
				{
					lastDecision.movement = nullptr;
				}
				break;
			case AIMovement::Type::ChangeStance:
				if (((u.target_body_state == BodyState::Kneeling ||
				      u.current_body_state == BodyState::Kneeling) &&
				     lastDecision.movement->kneelingMode == KneelingMode::Kneeling) ||
				    ((u.target_body_state == BodyState::Prone ||
				      u.current_body_state == BodyState::Prone) &&
				     lastDecision.movement->movementMode == MovementMode::Prone))
				{
					lastDecision.movement = nullptr;
				}
				break;
			default:
				break;
		}
	}

	//
	// See wether re-think is required
	//

	// Conditions that prevent re-thinking:

	// 1: Decision is never re-thought if currently throwing
	if (!u.missions.empty() && u.missions.front()->type == BattleUnitMission::Type::ThrowItem)
	{
		return {};
	}

	// 2: Anything else?
	//

	// Conditions that require re-thinking:

	bool reThink =
	    // Timer ran out
	    (ticksUntilReThink > 0 && ticksLastThink + ticksUntilReThink <= state.gameTime.getTicks())
	    // We just spotted an enemy and we have no timer
	    || (!u.visibleEnemies.empty() && !enemySpottedPrevious && ticksUntilReThink == 0)
	    // We just stopped seeing our last enemy and we are not on a get in range / pursuit move
	    ||
	    (enemySpotted && u.visibleEnemies.empty() &&
	     (!lastDecision.movement || (lastDecision.movement->type != AIMovement::Type::Pursue &&
	                                 lastDecision.movement->type != AIMovement::Type::GetInRange)))
	    // We have throw action but we stopped moving
	    || (lastDecision.action && lastDecision.action->type == AIAction::Type::AttackGrenade &&
	        u.missions.empty())
	    // We were attacked and we are not on a mission to get in range
	    || (attackerPosition != NONE &&
	        (!lastDecision.movement || lastDecision.movement->type != AIMovement::Type::GetInRange))
	    // We have enemies in sight, we are not attacking and we are not carrying out an action
	    || (!u.visibleEnemies.empty() && !u.isAttacking() &&
	        (!lastDecision.action || !u.missions.empty()));

	// Note: if no enemies are in sight, and we're idle, we will do nothing.
	// This state is handled by tactical AI

	if (!reThink)
	{
		return lastDecision;
	}

	auto result = u.visibleEnemies.empty() ? thinkGreen(state, u) : thinkRed(state, u);
	auto decision = std::get<0>(result);
	lastDecision = decision;

	if (decision.isEmpty())
	{
		return {};
	}

	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = std::get<2>(result);

	routine(state, u);

	return decision;
}

std::tuple<AIDecision, bool> VanillaUnitAI::think(GameState &state, BattleUnit &u)
{
	active = u.isAIControlled(state);

	if (!active)
	{
		return NULLTUPLE2;
	}

	auto decision = thinkInternal(state, u);

	if (!decision.isEmpty())
	{
		lastDecision = decision;
	}

	return std::make_tuple(decision, false);
}

void VanillaUnitAI::routine(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	// Reload all guns
	for (auto &e : u.agent->equipment)
	{
		if (e->needsReload())
		{
			e->loadAmmo(state);
		}
	}

	// Equip a cloaking field if we don't already have one in left hand
	auto cloaking = u.agent->getFirstItemByType(AEquipmentType::Type::CloakingField);
	if (cloaking)
	{
		UnitAIHelper::ensureItemInSlot(state, cloaking, AEquipmentSlotType::LeftHand);
	}

	// Equip best weapon in right hand if we don't have something we otherwise need in that slot
	auto rhItem = u.agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
	if (!lastDecision.action || rhItem != lastDecision.action->item)
	{
		// Right now just equip most long ranged weapon, if tied most damaging
		auto newItem = rhItem;
		int maxRange = 0;
		int maxDamage = 0;
		for (auto &e : u.agent->equipment)
		{
			if (!e->canFire())
			{
				continue;
			}
			if ((e->getPayloadType()->range > maxRange) ||
			    (e->getPayloadType()->range == maxRange && e->getPayloadType()->damage > maxDamage))
			{
				maxRange = e->getPayloadType()->range;
				maxDamage = e->getPayloadType()->damage;
			}
		}
		// Equip new item into slot
		if (newItem != rhItem)
		{
			UnitAIHelper::ensureItemInSlot(state, newItem, AEquipmentSlotType::RightHand);
		}
	}

	// Update aiState
	{
		attackerPosition = NONE;
		enemySpottedPrevious = enemySpotted;
		enemySpotted = false;
		lastSeenEnemyPosition = NONE;
	}
}

UString AIAction::getName()
{
	switch (type)
	{
		case AIAction::Type::AttackGrenade:
			return format("Attack %s with grenade %s ", targetUnit->id, item->type->id);
		case AIAction::Type::AttackWeaponTile:
			if (item)
			{
				return format("Attack %s with weapon %s ", targetLocation, item->type->id);
			}
			else
			{
				return format("Attack %s with weapon(s)", targetLocation);
			}
		case AIAction::Type::AttackWeaponUnit:
			if (item)
			{
				return format("Attack %s with weapon %s ", targetUnit->id, item->type->id);
			}
			else
			{
				return format("Attack %s with weapon(s)", targetUnit->id);
			}
		case AIAction::Type::AttackPsiMC:
			return format("Attack %s with psi MC using %s ", targetUnit->id, item->type->id);
		case AIAction::Type::AttackPsiStun:
			return format("Attack %s with psi stun using %s ", targetUnit->id, item->type->id);
		case AIAction::Type::AttackPsiPanic:
			return format("Attack %s with psi panic using %s ", targetUnit->id, item->type->id);
	}
	LogError("Unimplemented getName for AIAction %d", (int)type);
	return "";
}

UString AIMovement::getName()
{
	switch (type)
	{
		case AIMovement::Type::Stop:
			return format("Stop");
		case AIMovement::Type::ChangeStance:
			return format("Change Stance");
		case AIMovement::Type::Patrol:
			return format("Move to %s", targetLocation);
		case AIMovement::Type::Advance:
			return format("Advance on target to %s", targetLocation);
		case AIMovement::Type::Pursue:
			return format("Pursue target to %s", targetLocation);
		case AIMovement::Type::GetInRange:
			return format("Get in range, moving to %s", targetLocation);
		case AIMovement::Type::Retreat:
			return format("Retreat to %s", targetLocation);
		case AIMovement::Type::TakeCover:
			return format("Taking cover, moving to %s", targetLocation);
		case AIMovement::Type::Turn:
			return format("Turn to %s", targetLocation);
	}
	LogError("Unimplemented getName for AIMovement %d", (int)type);
	return "";
}

UString AIDecision::getName()
{
	return format("Action: [%s] Movement: [%s]", action ? action->getName() : "NULL",
	              movement ? movement->getName() : "NULL");
}

void LowMoraleUnitAI::reset(GameState &, BattleUnit &) { ticksActionAvailable = 0; }

std::tuple<AIDecision, bool> LowMoraleUnitAI::think(GameState &state, BattleUnit &u)
{
	switch (u.getAIType())
	{
		case AIType::PanicFreeze:
		case AIType::PanicRun:
		case AIType::Berserk:
			active = true;
			break;
		case AIType::None:
		case AIType::Civilian:
		case AIType::Loner:
		case AIType::Group:
			active = false;
			break;
	}

	if (!active)
	{
		return NULLTUPLE2;
	}

	if (ticksActionAvailable <= state.gameTime.getTicks())
	{
		ticksActionAvailable = state.gameTime.getTicks() + LOWMORALE_AI_INTERVAL;
		auto decision = AIDecision();
		switch (u.getAIType())
		{
			case AIType::PanicFreeze:
				// Stay and do nothing
				return std::make_tuple(decision, true);
			case AIType::PanicRun:
			{
				// Do not overwrite if already running somewhere
				if (!u.isMoving())
				{
					// Move to adjacent LOS block's random tile
					// Find all adjacent LOS blocks
					auto &linkCost = state.current_battle->linkCost;
					auto type = u.getType();
					int lbCount = state.current_battle->losBlocks.size();
					auto curLB = state.current_battle->getLosBlockID(u.position.x, u.position.y,
					                                                 u.position.z);
					std::list<int> adjacentBlocks;
					for (int i = 0; i < lbCount; i++)
					{
						if (i != curLB && linkCost[type][curLB + i * lbCount] != -1)
						{
							adjacentBlocks.push_back(i);
						}
					}
					if (!adjacentBlocks.empty())
					{
						auto targetLB = listRandomiser(state.rng, adjacentBlocks);
						auto targetPos = state.current_battle->blockCenterPos[type][targetLB];
						// Try 10 times to pick a valid position in that block, otherwise run to
						// it's center
						auto lb = state.current_battle->losBlocks[curLB];
						auto &map = u.tileObject->map;
						auto helper = BattleUnitTileHelper(map, u);
						for (int i = 0; i < 10; i++)
						{
							auto randPos =
							    Vec3<int>{randBoundsExclusive(state.rng, lb->start.x, lb->end.x),
							              randBoundsExclusive(state.rng, lb->start.y, lb->end.y),
							              randBoundsExclusive(state.rng, lb->start.z, lb->end.z)};
							if (helper.canEnterTile(nullptr, map.getTile(randPos)))
							{
								targetPos = randPos;
								break;
							}
						}
						decision.movement = mksp<AIMovement>();
						decision.movement->type = AIMovement::Type::Patrol;
						decision.movement->targetLocation = targetPos;
						decision.movement->kneelingMode = u.kneeling_mode;
						// 33% chance to switch to run
						decision.movement->movementMode =
						    randBoundsExclusive(state.rng, 0, 100) < 33 ? MovementMode::Running
						                                                : u.movement_mode;
					}
				}
				return std::make_tuple(decision, true);
			}
			case AIType::Berserk:
			{
				// Determine if we have a weapon in hand
				auto e1 = u.agent->getFirstItemInSlot(AEquipmentSlotType::LeftHand);
				auto e2 = u.agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
				auto canFire = ((e1 && e1->canFire()) || (e2 && e2->canFire()));
				// Roll for what kind of action we take with berserk
				int roll = randBoundsExclusive(state.rng, 0, 100);
				// 20% chance to attack a friendly, 40% chance to attack an enemy, 40% chance to
				// attack random tile
				int shootType = roll < 20 ? 1 : (roll < 60 ? 2 : 3);
				// Intentional fall-through in case we cannot find a vaild target
				switch (shootType)
				{
					case 1:
					{
						// Pick a random friendly within 5 tiles of us
						std::list<sp<BattleUnit>> victims;
						for (auto unit : state.current_battle->units)
						{
							if (unit.second->owner != u.owner)
							{
								continue;
							}
							auto distance = glm::distance(unit.second->position, u.position);
							if (distance < 5.0f)
							{
								victims.push_back(unit.second);
							}
						}
						if (!victims.empty())
						{
							auto victim = listRandomiser(state.rng, victims);
							if (!canFire)
							{
								decision.movement = mksp<AIMovement>();
								decision.movement->type = AIMovement::Type::Turn;
								decision.movement->targetLocation = victim->position;
								break;
							}
							else
							{
								if (u.canAttackUnit(victim) != WeaponStatus::NotFiring)
								{
									decision.action = mksp<AIAction>();
									decision.action->type = AIAction::Type::AttackWeaponUnit;
									decision.action->targetUnit = {&state, victim};
									decision.action->weaponStatus = WeaponStatus::FiringBothHands;
									break;
								}
							}
						}
					}
					case 2:
					{
						// Pick a random visible enemy
						if (!u.visibleEnemies.empty())
						{
							auto target = setRandomiser(state.rng, u.visibleEnemies);
							if (!canFire)
							{
								decision.movement = mksp<AIMovement>();
								decision.movement->type = AIMovement::Type::Turn;
								decision.movement->targetLocation = target->position;
								break;
							}
							else
							{
								if (u.canAttackUnit(target) != WeaponStatus::NotFiring)
								{
									decision.action = mksp<AIAction>();
									decision.action->type = AIAction::Type::AttackWeaponUnit;
									decision.action->targetUnit = target;
									decision.action->weaponStatus = WeaponStatus::FiringBothHands;
									break;
								}
							}
						}
					}
					case 3:
					{
						int x = randBoundsInclusive(state.rng, -10, 10);
						int y = randBoundsInclusive(state.rng, -10, 10);
						int z = randBoundsInclusive(state.rng, -1, 1);

						auto targetPos = (Vec3<int>)u.position + Vec3<int>{x, y, z};
						auto &map = u.tileObject->map;

						if (targetPos.x < 0)
							targetPos.x = 0;
						if (targetPos.y < 0)
							targetPos.y = 0;
						if (targetPos.z < 0)
							targetPos.z = 0;
						if (targetPos.x >= map.size.x)
							targetPos.x = map.size.x - 1;
						if (targetPos.y >= map.size.y)
							targetPos.y = map.size.y - 1;
						if (targetPos.z >= map.size.z)
							targetPos.z = map.size.z - 1;

						decision.action = mksp<AIAction>();
						decision.action->type = AIAction::Type::AttackWeaponTile;
						decision.action->targetLocation = targetPos;
						decision.action->weaponStatus = WeaponStatus::FiringBothHands;
						break;
					}
				}
				return std::make_tuple(decision, true);
			}
			default:
				LogError("Unsupported LowMorale AI type %d", (int)u.getAIType());
				break;
		}
	}
	return std::make_tuple(AIDecision(), true);
}

void DefaultUnitAI::reset(GameState &, BattleUnit &)
{
	ticksAutoTargetAvailable = 0;
	ticksAutoTurnAvailable = 0;
	attackerPosition = {0, 0, 0};
}

void DefaultUnitAI::notifyUnderFire(Vec3<int> position) { attackerPosition = position; }

void DefaultUnitAI::notifyHit(Vec3<int> position) { attackerPosition = position; }

std::tuple<AIDecision, bool> DefaultUnitAI::think(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	active = true;

	if (!active)
	{
		return NULLTUPLE2;
	}

	sp<AIAction> action;
	sp<AIMovement> movement;

	// Turn to attacker in real time if we're idle
	if (attackerPosition != NONE && !u.isBusy() && u.isConscious() &&
	    ticksAutoTurnAvailable <= state.gameTime.getTicks())
	{
		movement = mksp<AIMovement>();
		movement->type = AIMovement::Type::Turn;
		movement->targetLocation = u.position + (Vec3<float>)attackerPosition;
		ticksAutoTurnAvailable = state.gameTime.getTicks() + AUTO_TURN_COOLDOWN;
	}

	// Attack or face enemy
	if (u.isConscious() && !u.isAttacking() &&
	    !state.current_battle->visibleEnemies[u.owner].empty() &&
	    (u.missions.empty() || u.missions.front()->type != BattleUnitMission::Type::Snooze))
	{
		auto &enemies = state.current_battle->visibleEnemies[u.owner];
		auto e1 = u.agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
		auto e2 = u.agent->getFirstItemInSlot(AEquipmentSlotType::LeftHand);
		// Cannot or forbidden to attack:	Turn to enemy
		if (u.fire_permission_mode == BattleUnit::FirePermissionMode::CeaseFire ||
		    ((!e1 || !e1->canFire()) && (!e2 || !e2->canFire())))
		{
			if (ticksAutoTurnAvailable <= state.gameTime.getTicks() && !u.isMoving())
			{
				// Look at focused unit or find closest enemy
				auto targetEnemy = u.focusUnit;
				auto backupEnemy = targetEnemy;
				if (!targetEnemy || !targetEnemy->isConscious() ||
				    enemies.find(targetEnemy) == enemies.end())
				{
					targetEnemy.clear();
					backupEnemy.clear();
					float minDistance = FLT_MAX;
					for (auto enemy : enemies)
					{
						// Do not auto-target harmless things
						if (!enemy->isConscious() || enemy->getAIType() == AIType::None)
						{
							continue;
						}
						if (!u.hasLineToUnit(enemy))
						{
							// Track an enemy we can see but can't fire at,
							// In case we can't fire at anybody
							if (u.visibleUnits.find(enemy) != u.visibleUnits.end())
							{
								backupEnemy = enemy;
							}
							continue;
						}
						auto distance = glm::distance(enemy->position, u.position);
						if (distance < minDistance)
						{
							minDistance = distance;
							targetEnemy = enemy;
						}
					}
				}
				if (!targetEnemy && backupEnemy)
				{
					targetEnemy = backupEnemy;
				}
				if (targetEnemy)
				{
					movement = mksp<AIMovement>();
					movement->type = AIMovement::Type::Turn;
					movement->targetLocation = targetEnemy->position;
					ticksAutoTurnAvailable = state.gameTime.getTicks() + AUTO_TURN_COOLDOWN;
				}
			}
		}
		// Can attack and allowed to:		Attack enemy
		else
		{
			if (ticksAutoTargetAvailable <= state.gameTime.getTicks())
			{
				// Find enemy we can attack amongst those visible
				auto targetEnemy = u.focusUnit;
				auto weaponStatus = WeaponStatus::NotFiring;
				// Ensure we can see and attack focus, if can't attack focus or have no focus - take
				// closest attackable
				if (!targetEnemy || !targetEnemy->isConscious() ||
				    enemies.find(targetEnemy) == enemies.end() ||
				    u.canAttackUnit(targetEnemy) == WeaponStatus::NotFiring)
				{
					targetEnemy.clear();
					// Make a list of enemies sorted by distance to them
					std::map<float, StateRef<BattleUnit>> enemiesByDistance;
					for (auto enemy : enemies)
					{
						// Do not auto-target harmless things
						if (!enemy->isConscious() || enemy->getAIType() == AIType::None)
						{
							continue;
						}
						// Ensure we add every unit
						auto distance = glm::distance(enemy->position, u.position);
						while (enemiesByDistance.find(distance) != enemiesByDistance.end())
						{
							distance += 0.01f;
						}
						enemiesByDistance[distance] = enemy;
					}
					// Pick enemy that is closest and can be attacked
					for (auto entry : enemiesByDistance)
					{
						weaponStatus = u.canAttackUnit(entry.second);
						if (weaponStatus != WeaponStatus::NotFiring)
						{
							targetEnemy = entry.second;
							break;
						}
					}
				}

				// Attack if we can
				if (targetEnemy)
				{
					action = mksp<AIAction>();
					action->type = AIAction::Type::AttackWeaponUnit;
					action->targetUnit = targetEnemy;
					action->weaponStatus = weaponStatus;
				}
				else
				{
					ticksAutoTargetAvailable = state.gameTime.getTicks() + AUTO_TARGET_COOLDOWN;
				}
			}
		}
	}

	// Enzyme
	if (u.enzymeDebuffIntensity > 0 && !u.isMoving() && u.canMove())
	{
		// Move to a random adjacent tile
		auto from = u.tileObject->getOwningTile();
		auto &map = u.tileObject->map;
		auto helper = BattleUnitTileHelper{map, u};
		std::list<Vec3<int>> possiblePositions;
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					if (x == 0 && y == 0 && z == 0)
					{
						continue;
					}
					Vec3<int> newPos = (Vec3<int>)u.position + Vec3<int>(x, y, z);
					if (map.tileIsValid(newPos) && helper.canEnterTile(from, map.getTile(newPos)))
					{
						possiblePositions.push_back(newPos);
					}
				}
			}
		}
		if (!possiblePositions.empty())
		{
			auto newPos = listRandomiser(state.rng, possiblePositions);
			movement = mksp<AIMovement>();
			movement->type = AIMovement::Type::Patrol;
			movement->targetLocation = newPos;
			movement->kneelingMode = u.kneeling_mode;
			movement->movementMode = u.agent->isMovementStateAllowed(MovementState::Running)
			                             ? MovementMode::Running
			                             : MovementMode::Walking;
		}
	}

	attackerPosition = NONE;

	return std::make_tuple(AIDecision(action, movement), action || movement);
}

void BehaviorUnitAI::reset(GameState &, BattleUnit &) {}

std::tuple<AIDecision, bool> BehaviorUnitAI::think(GameState &state, BattleUnit &u)
{
	std::ignore = state;
	std::ignore = u;
	active = false;

	if (!active)
	{
		return NULLTUPLE2;
	}

	LogError("Implement Behavior AI");
	return NULLTUPLE2;
}

void VanillaUnitAI::reset(GameState &state, BattleUnit &)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = 0;
}

void VanillaUnitAI::notifyUnderFire(Vec3<int> position) { attackerPosition = position; }

void VanillaUnitAI::notifyHit(Vec3<int> position) { attackerPosition = position; }

void VanillaUnitAI::notifyEnemySpotted(Vec3<int> position)
{
	enemySpotted = true;
	lastSeenEnemyPosition = position;
}

void VanillaTacticalAI::reset(GameState &state, StateRef<Organisation>)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = randBoundsExclusive(state.rng, (uint64_t)0, AI_THINK_INTERVAL * 4);
}

std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
VanillaTacticalAI::think(GameState &state, StateRef<Organisation> o)
{
	static const int VANILLA_TACTICAL_AI_THINK_INTERVAL = TICKS_PER_SECOND / 8;

	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilReThink > curTicks)
	{
		return {};
	}

	ticksLastThink = curTicks;
	ticksUntilReThink = VANILLA_TACTICAL_AI_THINK_INTERVAL;

	// Find an idle unit that needs orders
	for (auto u : state.current_battle->units)
	{
		// Skip: if wrong owner, unconscious, busy or immobile
		if (u.second->owner != o || !u.second->isConscious() || u.second->isBusy() ||
		    !u.second->canMove())
		{
			continue;
		}

		switch (u.second->getAIType())
		{
			case AIType::None:
				// Do nothing
				continue;
			case AIType::PanicFreeze:
			case AIType::PanicRun:
			case AIType::Berserk:
				// Do nothing
				continue;
			case AIType::Civilian:
			case AIType::Loner:
			case AIType::Group:
				// Go on below
				break;
		}

		// If unit found, try to get orders for him
		auto decisions = getPatrolMovement(state, *u.second);

		auto units = std::get<0>(decisions);
		AIDecision decision = {nullptr, std::get<1>(decisions)};

		// Randomize civ movement
		if (u.second->getAIType() == AIType::Civilian && decision.movement)
		{
			if (randBoundsExclusive(state.rng, 0, 100) < 50)
			{
				decision.movement->movementMode = MovementMode::Walking;
			}
			else
			{
				decision.movement->movementMode = MovementMode::Running;
			}
		}

		std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> result = {};
		result.emplace_back(units, decision);

		// For now, stop after giving orders to one group of units
		return result;
	}

	return {};
}

void TacticalAIBlock::init(GameState &state)
{
	for (auto o : state.current_battle->participants)
	{
		if (o == state.getPlayer())
		{
			continue;
		}
		aiList.emplace(o, mksp<VanillaTacticalAI>());
		aiList[o]->reset(state, o);
	}
	reset(state);
}

void TacticalAIBlock::reset(GameState &state)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = randBoundsExclusive(state.rng, (uint64_t)0, AI_THINK_INTERVAL * 4);
}

std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
TacticalAIBlock::think(GameState &state)
{
	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilReThink > curTicks)
	{
		return {};
	}

	ticksLastThink = curTicks;
	ticksUntilReThink = AI_THINK_INTERVAL;

	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> result;
	for (auto &o : this->aiList)
	{
		auto decisions = o.second->think(state, o.first);
		result.insert(result.end(), decisions.begin(), decisions.end());
	}
	return result;
}
}