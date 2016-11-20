#include "game/state/battle/ai.h"
#include "game/state/aequipment.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipment_type.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

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

// AI must also react to things like
// - getting shot at while moving

AIAction AI::getWeaponAction(GameState &state, BattleUnit &u, sp<AEquipment> e,
                             sp<BattleUnit> target)
{
	AIAction action;
	action.priority = FLT_MIN;

	return action;
}

// FIXME: IMPLEMENT PSI AI
AIAction AI::getPsiAction(GameState &state, BattleUnit &u, sp<AEquipment> e, sp<BattleUnit> target,
                          AIAction::Type type)
{
	AIAction action;
	action.priority = FLT_MIN;

	return action;
}

AIAction AI::getGrenadeAction(GameState &state, BattleUnit &u, sp<AEquipment> e,
                              sp<BattleUnit> target)
{
	AIAction action;
	action.priority = FLT_MIN;

	return action;
}

// FIXME: Implement AI pathing to something other than center
// FIXME: Implement AI checking wether target block can be stood in by this unit
AIAction AI::getMoveAction(GameState &state, std::list<StateRef<BattleUnit>> &units)
{
	AIAction action;

	auto &l = *state.current_battle->los_blocks.at(
	    vectorRandomizer(state.rng, state.current_battle->losBlockRandomizer));

	action.type = AIAction::Type::Move;
	action.targetLocation = {(l.start.x + l.end.x) / 2, (l.start.y + l.end.y) / 2,
	                         (l.start.z + l.end.z) / 2};
	action.units = units;
	// At least for 2 seconds keep moving and do not think again
	action.ticks = TICKS_PER_SECOND * 2;

	return action;
}

// FIXME: IMPLEMENT RETREAT
AIAction AI::getRetreatAction(GameState &state, std::list<StateRef<BattleUnit>> &units)
{
	AIAction action;

	// return action;
	return getMoveAction(state, units);
}

// Calculate AI's next action in case no enemies are seen
AIAction AI::thinkGreen(GameState &state, std::list<StateRef<BattleUnit>> &units)
{
	// Try move and retreat
	// Retreat chance depends on morale. Chance goes from 20% at 0 to 1% at 19, then 0% at 20+
	if (randBoundsExclusive(state.rng, 0, 100) - 80 >=
	    (*units.begin())->agent->modified_stats.morale)
	{
		// Retreat
		// Find a valid exit ground tile
		return getRetreatAction(state, units);
	}
	else
	{
		// Move
		// Find a los block we want to go into
		// Higher priority blocks have higher chance of being went into
		return getMoveAction(state, units);
	}
}

// Calculate AI's next action in case enemies are seen
AIAction AI::thinkRed(GameState &state, BattleUnit &u)
{
	AIAction action;
	action.priority = FLT_MIN;
	action.ticks = TICKS_PER_SECOND;

	// Try weapon/grenade/psi attack
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
					auto newAction = getWeaponAction(state, u, e, target);
					if (newAction.priority > action.priority)
					{
						action = newAction;
					}
					break;
				}
				case AEquipmentType::Type::Grenade:
				{
					auto newAction = getGrenadeAction(state, u, e, target);
					if (newAction.priority > action.priority)
					{
						action = newAction;
					}
					break;
				}
				case AEquipmentType::Type::MindBender:
				{
					auto newAction = getPsiAction(state, u, e, target, AIAction::Type::AttackPsiMC);
					if (newAction.priority > action.priority)
					{
						action = newAction;
					}
					newAction = getPsiAction(state, u, e, target, AIAction::Type::AttackPsiPanic);
					if (newAction.priority > action.priority)
					{
						action = newAction;
					}
					newAction = getPsiAction(state, u, e, target, AIAction::Type::AttackPsiStun);
					if (newAction.priority > action.priority)
					{
						action = newAction;
					}
					break;
				}
				default: // don't cry travis
					// Nothing to do
					break;
			}
		}
	}
	return action;
}

AIAction AI::think(GameState &state, BattleUnit &u)
{
	static float SQUAD_RANGE = 5.0f;

	if (u.agent->type->aiType == AIType::None)
	{
		AIAction action;
		action.ticks = TICKS_PER_HOUR;
		return action;
	}

	routine(state, u);

	std::list<StateRef<BattleUnit>> units;

	// Figure out wether we want to act as a group
	if (u.agent->type->aiType == AIType::Group && u.agent->type->allowsDirectControl)
	{
		// If no enemies visible - try to move as group
		if (u.visibleEnemies.empty())
		{
			auto sft = u.shared_from_this();
			for (auto &unit : state.current_battle->forces[u.owner].squads[u.squadNumber].units)
			{
				if (unit != sft && unit->isConscious() &&
				    unit->agent->type->aiType == AIType::Group &&
				    unit->agent->type->allowsDirectControl && unit->visibleEnemies.empty())
				{
					units.emplace_back(&state, unit->id);
				}
			}
			if (!units.empty())
			{
				// Check if at least half of the group is ready
				int numUnitsReady = 0;
				for (auto &unit : units)
				{
					if (!unit->isBusy())
					{
						numUnitsReady++;
					}
				}
				if (numUnitsReady + 1 < (units.size() + 1) / 2)
				{
					// Group move failed, just process this unit
					units.clear();
				}
			}
		}
		else // If enemies are visible - stop other units nearby from moving
		{
			for (auto unit : state.current_battle->forces[u.owner].squads[u.squadNumber].units)
			{
				if (!unit->missions.empty() &&
				    unit->missions.front()->type == BattleUnitMission::Type::GotoLocation &&
				    glm::distance(u.position, unit->position) < SQUAD_RANGE)
				{
					unit->cancelMissions(state);
				}
			}
		}
	}

	// If no enemies seen - patrol
	if (u.visibleEnemies.empty())
	{
		units.emplace_back(&state, u.id);
		return thinkGreen(state, units);
	}
	// If enemies seen - attack
	else
	{
		return thinkRed(state, u);
	}
}

void AI::routine(GameState &state, BattleUnit &u)
{
	// Operate inventory, reload weapons etc.
}

UString AIAction::getName()
{
	switch (type)
	{
		case AIAction::Type::Idle:
			return format("Idle for %d", ticks);
		case AIAction::Type::Move:
			return format("Move to %d,%d,%d in a group of %d", targetLocation.x, targetLocation.y,
			              targetLocation.z, (int)units.size());
		case AIAction::Type::Retreat:
			return format("Retreat to %d,%d,%d in a group of %d", targetLocation.x,
			              targetLocation.y, targetLocation.z, (int)units.size());
		case AIAction::Type::AttackGrenade:
			return format("Attack %s with grenade %s ", target->id, item->type->id);
		case AIAction::Type::AttackWeapon:
			return format("Attack %s with weapon %s ", target->id, item->type->id);
		case AIAction::Type::AttackPsiMC:
			return format("Attack %s with psi MC using %s ", target->id, item->type->id);
		case AIAction::Type::AttackPsiStun:
			return format("Attack %s with psi stun using %s ", target->id, item->type->id);
		case AIAction::Type::AttackPsiPanic:
			return format("Attack %s with psi panic using %s ", target->id, item->type->id);
	}
}
}