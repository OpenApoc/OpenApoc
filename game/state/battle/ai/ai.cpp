#include "game/state/battle/ai/ai.h"
#include "game/state/battle/battle.h"
#include "game/state/aequipment.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

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

AIMovement::AIMovement() : movementMode(MovementMode::Walking), kneelingMode(KneelingMode::None) {}

AIDecision::AIDecision(sp<AIAction> action, sp<AIMovement> movement)
    : action(action), movement(movement),
      priority(
          std::max(action ? action->priority : FLT_MIN, movement ? movement->priority : FLT_MIN)),
      ticksUntilThinkAgain(std::max(action ? action->ticksUntilThinkAgain : -1,
                                    movement ? movement->ticksUntilThinkAgain : -1))
{
}

bool AIDecision::isEmpty()
{
	return !action && !movement;
}

void UnitAIList::init(GameState &state, BattleUnit &u)
{
	// FIXME: Actually read this option
	bool USER_OPTION_USE_HARDCORE_AI = false;

	lastDecision.action = nullptr;
	lastDecision.movement = nullptr;

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

	for (int i = 0;i < aiList.size();i++)
	{
		aiMap[aiList[i]->getName()] = i;
	}

	reset(state, u);
}

void UnitAIList::reset(GameState &state, BattleUnit &u)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilThinkAgain = randBoundsExclusive(
		state.rng, 0, (int)TICKS_PER_SECOND);

	for (auto &ai : aiList)
	{
		ai->reset(state, u);
	}
}

AIDecision UnitAIList::think(GameState &state, BattleUnit &u)
{
	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilThinkAgain > curTicks)
	{
		return{};
	}

	ticksLastThink = curTicks;
	ticksUntilThinkAgain = AI_THINK_INTERVAL;

	AIDecision decision = {};
	for (auto ai : aiList)
	{
		auto newDecision = ai->think(state, u);
		if (newDecision.isEmpty())
		{
			continue;
		}
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
		if (newDecision.halt)
		{
			break;
		}
	}
	return decision;
}

void UnitAIList::registerNewDecision(AIDecision decision)
{
	if (lastDecision.action && !decision.action)
	{
		decision.action = lastDecision.action;
	}
	if (lastDecision.movement && !decision.movement)
	{
		decision.movement = lastDecision.movement;
	}
	lastDecision = decision;
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
sp<AIMovement> VanillaUnitAI::getPatrolMovement(GameState &state, BattleUnit &u)
{
	static float SQUAD_RANGE = 10.0f;

	auto result = mksp<AIMovement>();
	result->priority = 0.0f;

	// List of units that will go on patrol
	std::list<StateRef<BattleUnit>> units;
	units.emplace_back(&state, u.id);

	// If we have group AI - collect other units within range
	if (u.getAIType() == AIType::Group && u.agent->type->allowsDirectControl)
	{
		// Collect all units within squad range
		auto sft = u.shared_from_this();
		for (auto &unit : state.current_battle->forces[u.owner].squads[u.squadNumber].units)
		{
			if (unit != sft && unit->isConscious() && unit->getAIType() == AIType::Group &&
			    unit->agent->type->allowsDirectControl && unit->visibleEnemies.empty() &&
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
							result->ticksUntilThinkAgain = TICKS_PER_TURN;
							return result;
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
		// auto &lb = *state.current_battle->losBlocks.at(lbID); // <-- not needed?
		result->type = AIMovement::Type::Patrol;
		result->targetLocation = state.current_battle->blockCenterPos[u.getType()][lbID];
		result->units = units;
		break;
	}

	return result;
}

sp<AIMovement> VanillaUnitAI::getRetreatMovement(GameState &state, BattleUnit &u, bool forced)
{
	// Chance to take retreat is 1% per morale point below 20
	if (randBoundsExclusive(state.rng, 0, 100) >= std::max(0, 20 - u.agent->modified_stats.morale))
	{
		return nullptr;
	}

	LogWarning("Implement retreat (for now patrolling instead)");

	auto result = mksp<AIMovement>();

	return getPatrolMovement(state, u);
}

sp<AIMovement> VanillaUnitAI::getTakeCoverMovement(GameState &state, BattleUnit &u, bool forced)
{
	// Chance to take cover is 33% * sqrt(num_enemies_seen), if no one is seen then assume 3
	if (randBoundsExclusive(state.rng, 0, 100) >=
	    33.0f * sqrtf(u.visibleEnemies.empty() ? 3 : (int)u.visibleEnemies.size()))
	{
		return nullptr;
	}

	LogWarning("Implement take cover (for now kneeling instead)");

	auto result = mksp<AIMovement>();

	result->kneelingMode = KneelingMode::Kneeling;
	result->priority = 0.0f;
	result->type = AIMovement::Type::None;

	return result;
}

sp<AIMovement> VanillaUnitAI::getKneelMovement(GameState &state, BattleUnit &u, bool forced)
{
	// Chance to kneel is 33% * sqrt(num_enemies_seen), if no one is seen then assume 3
	if (randBoundsExclusive(state.rng, 0, 100) >=
	    33.0f * sqrtf(u.visibleEnemies.empty() ? 3 : (int)u.visibleEnemies.size()))
	{
		return nullptr;
	}

	auto result = mksp<AIMovement>();

	result->kneelingMode = KneelingMode::Kneeling;
	result->priority = 0.0f;
	result->type = AIMovement::Type::None;

	return result;
}

sp<AIMovement> VanillaUnitAI::getTurnMovement(GameState &state, BattleUnit &u, Vec3<int> target)
{
	auto result = mksp<AIMovement>();

	result->type = AIMovement::Type::Turn;
	result->priority = 0.0f;
	result->targetLocation = target;
	if (u.aiList.lastDecision.movement)
	{
		result->kneelingMode = u.aiList.lastDecision.movement->kneelingMode;
	}

	return result;
}

sp<AIMovement> VanillaUnitAI::getPursueMovement(GameState &state, BattleUnit &u, Vec3<int> target)
{
	// Chance to pursuit is 1% per morale point above 20
	if (randBoundsExclusive(state.rng, 0, 100) >= std::max(0, u.agent->modified_stats.morale - 20))
	{
		return nullptr;
	}

	auto result = mksp<AIMovement>();

	result->type = AIMovement::Type::Patrol;
	result->targetLocation = target;
	result->priority = 0.0f;

	return result;
}

// FIXME: IMPLEMENT PROPER WEAPON AI
AIDecision VanillaUnitAI::getWeaponDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                                 sp<BattleUnit> target)
{
	if (e->canFire(target->position))
	{
		auto action = mksp<AIAction>();
		action->item = e;
		action->targetUnit = target;
		action->type = AIAction::Type::AttackWeapon;
		action->ticksUntilThinkAgain = TICKS_PER_TURN;
		action->priority = e->getPayloadType()->damage;

		return {action, mksp<AIMovement>()};
	}

	return {};
}

// FIXME: IMPLEMENT PSI AI
AIDecision VanillaUnitAI::getPsiDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                              sp<BattleUnit> target, AIAction::Type type)
{
	return {};
}

// FIXME: IMPLEMENT GRENADE AI
AIDecision VanillaUnitAI::getGrenadeDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
                                  sp<BattleUnit> target)
{
	return {};
}

AIDecision VanillaUnitAI::getAttackDecision(GameState &state, BattleUnit &u)
{
	auto decision = AIDecision();
	decision.ticksUntilThinkAgain = TICKS_PER_SECOND;

	// Try each item for a weapon/grenade/psi attack
	// If out of range - advance
	// If you have no valid attack at all - retreat

	bool mindBenderFound = false;
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
				if (mindBenderFound)
				{
					continue;
				}
				else
				{
					mindBenderFound = true;
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
					if (newDecision.priority > decision.priority)
					{
						decision = newDecision;
					}
					break;
				}
				case AEquipmentType::Type::Grenade:
				{
					auto newDecision = getGrenadeDecision(state, u, e, target);
					if (newDecision.priority > decision.priority)
					{
						decision = newDecision;
					}
					break;
				}
				case AEquipmentType::Type::MindBender:
				{
					auto newDecision =
					    getPsiDecision(state, u, e, target, AIAction::Type::AttackPsiMC);
					if (newDecision.priority > decision.priority)
					{
						decision = newDecision;
					}
					newDecision =
					    getPsiDecision(state, u, e, target, AIAction::Type::AttackPsiPanic);
					if (newDecision.priority > decision.priority)
					{
						decision = newDecision;
					}
					newDecision =
					    getPsiDecision(state, u, e, target, AIAction::Type::AttackPsiStun);
					if (newDecision.priority > decision.priority)
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
	if (!decision.action && !decision.movement)
	{
		return {nullptr, getRetreatMovement(state, u, true)};
	}
	return decision;
}

// Calculate AI's next action in case the unit is not attacking
AIDecision VanillaUnitAI::thinkGreen(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	bool isMoving = u.aiList.lastDecision.movement && u.isMoving();
	bool isUnderAttack = attackerPosition != NONE;
	bool isInterrupted = u.aiList.lastDecision.ticksUntilThinkAgain > 0;
	bool isEnemyVisible = !u.visibleEnemies.empty();
	bool wasEnemyVisible = enemySpotted && lastSeenEnemyPosition != NONE;

	if (isMoving)
	{
		// If unit is moving into cover -> carry on
		if (u.aiList.lastDecision.movement->type == AIMovement::Type::TakeCover)
		{
			return {};
		}
		// If unit is moving to get in range and target is visible -> carry on
		if (u.aiList.lastDecision.movement->type == AIMovement::Type::GetInRange &&
		    std::find(u.visibleEnemies.begin(), u.visibleEnemies.end(),
		              u.aiList.lastDecision.action->targetUnit) != u.visibleEnemies.end())
		{
			return {};
		}
	}

	if (isEnemyVisible)
	{
		auto kneel = getKneelMovement(state, u);
		if (kneel)
		{
			return {nullptr, kneel};
		}

		if (isUnderAttack)
		{
			auto takeCover = getTakeCoverMovement(state, u);
			if (takeCover)
			{
				return {nullptr, takeCover};
			}

			if (isInterrupted)
			{
				return {};
			}
		}

		auto retreat = getRetreatMovement(state, u);
		if (retreat)
		{
			return {nullptr, retreat};
		}

		return getAttackDecision(state, u);
	}

	if (wasEnemyVisible)
	{
		auto retreat = getRetreatMovement(state, u);
		if (retreat)
		{
			return {nullptr, retreat};
		}

		if (isUnderAttack)
		{
			auto takeCover = getTakeCoverMovement(state, u);
			if (takeCover)
			{
				return {nullptr, takeCover};
			}
		}

		auto pursue =
		    getPursueMovement(state, u, (Vec3<int>)u.position + lastSeenEnemyPosition);
		if (pursue)
		{
			return {nullptr, pursue};
		}

		if (isMoving)
		{
			return {};
		}
	}

	if (isUnderAttack)
	{
		if (!wasEnemyVisible)
		{
			auto takeCover = getTakeCoverMovement(state, u);
			if (takeCover)
			{
				return {nullptr, takeCover};
			}
		}

		if (u.missions.empty() || u.missions.front()->type != BattleUnitMission::Type::Turn)
		{
			return {nullptr,
			        getTurnMovement(state, u, (Vec3<int>)u.position + attackerPosition)};
		}
	}

	if (isMoving)
	{
		return {};
	}

	if (!wasEnemyVisible)
	{
		auto retreat = getRetreatMovement(state, u);
		if (retreat)
		{
			return {nullptr, retreat};
		}
	}

	return {nullptr, getPatrolMovement(state, u)};
}

// Calculate AI's next action in case enemies are seen
AIDecision VanillaUnitAI::thinkRed(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = {0, 0, 0};

	bool isUnderAttack = attackerPosition != NONE;
	bool isInterrupted = u.aiList.lastDecision.ticksUntilThinkAgain > 0;

	if (isUnderAttack)
	{
		auto takeCover = getTakeCoverMovement(state, u);
		if (takeCover)
		{
			return {nullptr, takeCover};
		}

		auto kneel = getKneelMovement(state, u);
		if (kneel)
		{
			return {nullptr, kneel};
		}

		if (isInterrupted)
		{
			return {};
		}
	}

	auto retreat = getRetreatMovement(state, u);
	if (retreat)
	{
		return {nullptr, retreat};
	}

	return getAttackDecision(state, u);
}

AIDecision VanillaUnitAI::thinkInternal(GameState &state, BattleUnit &u)
{
	static const Vec3<int> NONE = { 0, 0, 0 };

	if (u.getAIType() == AIType::None)
	{
		auto result = AIDecision();
		result.ticksUntilThinkAgain = TICKS_PER_HOUR;
		return result;
	}

	// See wether re-think is required

	// Decision is never re-thought if: actionType == grenade && currently throwing
	if (u.aiList.lastDecision.action &&
		u.aiList.lastDecision.action->type == AIAction::Type::AttackGrenade && !u.missions.empty() &&
		u.missions.front()->type == BattleUnitMission::Type::ThrowItem)
	{
		return{};
	}

	// Decision wether we need to re-think our actions
	bool reThink =
		// Timer ran out
		u.aiList.lastDecision.ticksUntilThinkAgain == 0
		// We have no action, no movement and no timer set up
		|| ((!u.aiList.lastDecision.action ||
			u.aiList.lastDecision.action->type == AIAction::Type::None) &&
			(!u.aiList.lastDecision.movement ||
				u.aiList.lastDecision.movement->type == AIMovement::Type::None) &&
			u.aiList.lastDecision.ticksUntilThinkAgain == -1)
		// We just spotted an enemy and we have no timer
		|| (!u.visibleEnemies.empty() && !enemySpottedPrevious &&
			u.aiList.lastDecision.ticksUntilThinkAgain == -1)
		// We just stopped seeing an enemy
		|| (u.visibleEnemies.empty() && enemySpotted)
		// We were attacked
		|| (attackerPosition != NONE)
		// We stopped moving
		|| (u.aiList.lastDecision.movement &&
			u.aiList.lastDecision.movement->type != AIMovement::Type::None &&
			u.aiList.lastDecision.movement->type != AIMovement::Type::Turn && !u.isMoving())
		// We stopped throwing
		|| (u.aiList.lastDecision.action &&
			u.aiList.lastDecision.action->type == AIAction::Type::AttackGrenade &&
			(u.missions.empty() || u.missions.front()->type != BattleUnitMission::Type::ThrowItem))
		// We stopped attacking with PSI
		|| (u.aiList.lastDecision.action &&
		(u.aiList.lastDecision.action->type == AIAction::Type::AttackPsiMC ||
			u.aiList.lastDecision.action->type == AIAction::Type::AttackPsiPanic ||
			u.aiList.lastDecision.action->type == AIAction::Type::AttackPsiStun)
			// FIXME: Introduce PSI condition
			&& false)
		// We stopped attacking with a weapon
		|| (u.aiList.lastDecision.action &&
			u.aiList.lastDecision.action->type == AIAction::Type::AttackWeapon && !u.isAttacking());

	if (!reThink)
	{
		return{};
	}

	auto result = u.visibleEnemies.empty() ? thinkGreen(state, u) : thinkRed(state, u);

	routine(state, u, result);

	return result;
}

AIDecision VanillaUnitAI::think(GameState &state, BattleUnit &u)
{
	active = u.isAIControlled(state);

	if (!active)
	{
		return {};
	}
	
	auto decision = thinkInternal(state, u);

	if (decision.isEmpty())
	{
		if (decision.ticksUntilThinkAgain == -1)
		{
			// Ensure no freezing of AI
			// If no new decision is made and
			if (u.aiList.lastDecision.ticksUntilThinkAgain == -1 &&
				(!u.aiList.lastDecision.action ||
					u.aiList.lastDecision.action->type == AIAction::Type::None) &&
					(!u.aiList.lastDecision.movement ||
						u.aiList.lastDecision.movement->type == AIMovement::Type::None))
			{
				decision.ticksUntilThinkAgain = TICKS_PER_TURN;
			}
		}
	}

	return decision;
}

void VanillaUnitAI::routine(GameState &state, BattleUnit &u, const AIDecision &decision)
{
	static const Vec3<int> NONE = {0, 0, 0};

	// Do whatever needs to be done for the chosen action
	if (decision.action)
	{
		// Equip item we're going to use
		if (decision.action->item)
		{
			auto rhItem = u.agent->getFirstItemInSlot(AEquipmentSlotType::RightHand);
			if (rhItem != decision.action->item)
			{
				// Remove item in the right hand
				if (rhItem)
				{
					u.agent->removeEquipment(rhItem);
				}

				// Remove item we will use and equip it in the right hand
				u.agent->removeEquipment(decision.action->item);
				u.agent->addEquipment(state, decision.action->item, AEquipmentSlotType::RightHand);

				// Equip back the item removed earlier
				if (rhItem)
				{
					for (auto s : u.agent->type->equipment_layout->slots)
					{
						if (u.agent->canAddEquipment(s.bounds.p0, rhItem->type))
						{
							u.agent->addEquipment(state, s.bounds.p0, rhItem);
							rhItem = nullptr;
							break;
						}
					}
				}

				// Drop the item if we couldn't equip it
				if (rhItem)
				{
					u.addMission(state, BattleUnitMission::dropItem(u, rhItem));
				}
			}
		}
	}

	// Do whatever needs to be done for the chosen movement
	if (decision.movement)
	{
		u.kneeling_mode = decision.movement->kneelingMode;
		u.movement_mode = decision.movement->movementMode;
	}

	// Do miscellaneous stuff

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
		auto lhItem = u.agent->getFirstItemInSlot(AEquipmentSlotType::LeftHand);
		if (!lhItem || lhItem->type->type != AEquipmentType::Type::CloakingField)
		{
			// Remove item in the left hand
			if (lhItem)
			{
				u.agent->removeEquipment(lhItem);
			}

			// Remove item we will use and equip it in the right hand
			u.agent->removeEquipment(cloaking);
			u.agent->addEquipment(state, cloaking, AEquipmentSlotType::LeftHand);

			// Equip back the item removed earlier
			if (lhItem)
			{
				for (auto s : u.agent->type->equipment_layout->slots)
				{
					if (u.agent->canAddEquipment(s.bounds.p0, lhItem->type))
					{
						u.agent->addEquipment(state, s.bounds.p0, lhItem);
						lhItem = nullptr;
						break;
					}
				}
			}

			// Drop the item if we couldn't equip it
			if (lhItem)
			{
				u.addMission(state, BattleUnitMission::dropItem(u, lhItem));
			}
		}
	}

	// Update aiState
	{
		attackerPosition = NONE;
		enemySpottedPrevious = enemySpotted;
		enemySpotted = false;
		lastSeenEnemyPosition = NONE;
		// Set movement to none if complete
		if (u.aiList.lastDecision.movement)
		{
			switch (u.aiList.lastDecision.movement->type)
			{
				case AIMovement::Type::Patrol:
				case AIMovement::Type::Advance:
				case AIMovement::Type::GetInRange:
				case AIMovement::Type::TakeCover:
					if (!u.isMoving())
					{
						u.aiList.lastDecision.movement->type = AIMovement::Type::None;
					}
					break;
				case AIMovement::Type::Turn:
					if (u.goalFacing == u.facing)
					{
						u.aiList.lastDecision.movement->type = AIMovement::Type::None;
					}
					break;
				default:
					break;
			}
		}
	}
}

UString AIAction::getName()
{
	switch (type)
	{
		case AIAction::Type::None:
			return format("Nothing");
		case AIAction::Type::AttackGrenade:
			return format("Attack %s with grenade %s ", targetUnit->id, item->type->id);
		case AIAction::Type::AttackWeapon:
			return format("Attack %s with weapon %s ", targetUnit->id, item->type->id);
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
		case AIMovement::Type::None:
			return format("Nothing");
		case AIMovement::Type::Patrol:
			return format("Move to %s in a group of %d", targetLocation, (int)units.size());
		case AIMovement::Type::Advance:
			return format("Advance on target to %s", targetLocation);
		case AIMovement::Type::GetInRange:
			return format("Get in range, moving to %s", targetLocation);
		case AIMovement::Type::Retreat:
			return format("Retreat to %s in a group of %d", targetLocation, (int)units.size());
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
	return format("Action: [%s] Movement: [%s] Timer: %d", action ? action->getName() : "NULL",
	              movement ? movement->getName() : "NULL", ticksUntilThinkAgain);
}

void LowMoraleUnitAI::reset(GameState &state, BattleUnit &u)
{
}

AIDecision LowMoraleUnitAI::think(GameState &state, BattleUnit &u)
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
		return{};
	}

	LogError("Implement Low Morale AI");
	return{};
}

void DefaultUnitAI::reset(GameState &state, BattleUnit &u)
{
}

AIDecision DefaultUnitAI::think(GameState &state, BattleUnit &u)
{
	active = false;

	if (!active)
	{
		return{};
	}

	LogError("Implement Default AI");
	return {};
}

void BehaviorUnitAI::reset(GameState &state, BattleUnit &u)
{

}

AIDecision BehaviorUnitAI::think(GameState &state, BattleUnit &u)
{
	active = false;

	if (!active)
	{
		return{};
	}

	LogError("Implement Behavior AI");
	return{};
}

void VanillaUnitAI::reset(GameState &state, BattleUnit &u)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilThinkAgain = randBoundsExclusive(
		state.rng, 0, (int)TICKS_PER_SECOND);
}


void VanillaUnitAI::notifyUnderFire(Vec3<int> position)
{ 
	attackerPosition = position;
}

void VanillaUnitAI::notifyHit(Vec3<int> position)
{ 
	attackerPosition = position;
}

void VanillaUnitAI::notifyEnemySpotted(Vec3<int> position)
{ 
	enemySpotted = true;
	lastSeenEnemyPosition = position;
}

void VanillaTacticalAI::reset(GameState &state, const Organisation &o)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilThinkAgain = randBoundsExclusive(
		state.rng, 0, (int)TICKS_PER_SECOND);
}

AIDecision VanillaTacticalAI::think(GameState &state, const Organisation &o)
{
	static const int VANILLA_TACTICAL_AI_THINK_INTERVAL = TICKS_PER_SECOND / 8;

	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilThinkAgain > curTicks)
	{
		return{};
	}
	
	ticksLastThink = curTicks;
	ticksUntilThinkAgain = VANILLA_TACTICAL_AI_THINK_INTERVAL;
		
	LogError("Implement Tactical AI");
	ticksUntilThinkAgain = TICKS_PER_HOUR;

	return{};
}

void TacticalAIBlock::init(GameState &state)
{
	for (auto o : state.current_battle->participants)
	{
		aiList[o] = mksp<VanillaTacticalAI>();
		aiList[o]->reset(state, *o);
	}
	reset(state);
}

void TacticalAIBlock::reset(GameState &state)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilThinkAgain = randBoundsExclusive(
		state.rng, 0, (int)TICKS_PER_SECOND);
}

AIDecision TacticalAIBlock::think(GameState &state)
{
	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilThinkAgain > curTicks)
	{
		return{};
	}

	ticksLastThink = curTicks;
	ticksUntilThinkAgain = AI_THINK_INTERVAL;

	for (auto &o : this->aiList)
	{
		o.second->think(state, *o.first);
	}
}

}