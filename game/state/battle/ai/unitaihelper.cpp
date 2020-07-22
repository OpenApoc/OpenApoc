#include "game/state/battle/ai/unitaihelper.h"
#include "game/state/battle/ai/aidecision.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/shared/aequipment.h"
#include <float.h>
#include <glm/glm.hpp>

namespace OpenApoc
{

sp<AIMovement> UnitAIHelper::getFallbackMovement(GameState &state, BattleUnit &u, bool forced)
{
	StateRef<BattleUnit> closestEnemy;
	for (auto &unit : state.current_battle->visibleEnemies[u.owner])
	{
		if (!closestEnemy || glm::length(unit->position - u.position) <
		                         glm::length(closestEnemy->position - u.position))
		{
			closestEnemy = unit;
		}
	}

	// Chance to fall back is:
	// +1% per each morale missing
	// +1% per 1/100th of lost health
	// -20% per every tile enemy is closer to us than 6
	int chance =
	    100 - u.agent->modified_stats.morale +
	    (u.agent->current_stats.health - u.agent->modified_stats.health) * 100 /
	        u.agent->current_stats.health -
	    (closestEnemy ? 20 * std::min(0, 6 - (int)glm::length(closestEnemy->position - u.position))
	                  : 0);
	if (!forced && randBoundsExclusive(state.rng, 0, 100) < chance)
	{
		return nullptr;
	}

	// Rate allies based (crudely) on distance to unit pos and from closest enemy
	auto &map = *state.current_battle->map;
	std::list<Vec3<int>> allyPos;
	float closestDistance = FLT_MAX;
	for (auto &unit : state.current_battle->units)
	{
		if (unit.second->owner != u.owner || !unit.second->isConscious() || unit.first == u.id)
		{
			continue;
		}
		auto dist = BattleUnitTileHelper::getDistanceStatic(u.position, unit.second->position) -
		            (closestEnemy ? BattleUnitTileHelper::getDistanceStatic(closestEnemy->position,
		                                                                    unit.second->position)
		                          : 0.0f);
		if (dist < closestDistance)
		{
			closestDistance = dist;
			allyPos.push_front(unit.second->position);
		}
		else
		{
			allyPos.push_back(unit.second->position);
		}
	}
	for (auto &pos : allyPos)
	{
		if (state.current_battle->findShortestPath(u.position, pos, {map, u}).back() != pos)
		{
			continue;
		}
		auto result = mksp<AIMovement>();
		result->type = AIMovement::Type::Retreat;
		result->movementMode = MovementMode::Running;
		result->targetLocation = pos;
		return result;
	}

	return nullptr;
}

sp<AIMovement> UnitAIHelper::getRetreatMovement(GameState &state, BattleUnit &u, bool forced)
{
	// Chance to take retreat is 1% per each morale missing
	if (!forced && randBoundsExclusive(state.rng, 0, 100) >= u.agent->modified_stats.morale)
	{
		return nullptr;
	}

	StateRef<BattleUnit> closestEnemy;
	for (auto &unit : state.current_battle->visibleEnemies[u.owner])
	{
		if (!closestEnemy || glm::length(unit->position - u.position) <
		                         glm::length(closestEnemy->position - u.position))
		{
			closestEnemy = unit;
		}
	}

	// Rate exits based (crudely) on distance to unit pos and from closest enemy
	auto &map = *state.current_battle->map;
	std::set<Vec3<int>> badExits;
	std::list<Vec3<int>> goodExits;
	float closestDistance = FLT_MAX;
	for (auto &pos : state.current_battle->exits)
	{
		if (!map.getTile(pos)->hasExit)
		{
			badExits.insert(pos);
			continue;
		}
		auto dist =
		    BattleUnitTileHelper::getDistanceStatic(u.position, pos) -
		    (closestEnemy ? BattleUnitTileHelper::getDistanceStatic(closestEnemy->position, pos)
		                  : 0.0f);
		if (dist < closestDistance)
		{
			closestDistance = dist;
			goodExits.push_front(pos);
		}
		else
		{
			goodExits.push_back(pos);
		}
	}
	for (auto &pos : badExits)
	{
		state.current_battle->exits.erase(pos);
	}
	for (auto &pos : goodExits)
	{
		if (state.current_battle->findShortestPath(u.position, pos, {map, u}).back() != pos)
		{
			continue;
		}
		auto result = mksp<AIMovement>();
		result->type = AIMovement::Type::Retreat;
		result->movementMode = MovementMode::Running;
		result->targetLocation = pos;
		return result;
	}
	return nullptr;
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

void UnitAIHelper::ensureItemInSlot(GameState &state, sp<AEquipment> item, EquipmentSlotType slot)
{
	auto u = item->ownerAgent->unit;
	auto curItem = u->agent->getFirstItemInSlot(slot);
	if (curItem != item)
	{
		// Remove item in the desired slot
		if (curItem)
		{
			u->agent->removeEquipment(state, curItem);
		}

		// Remove item we will use and equip it in the desired slot
		u->agent->removeEquipment(state, item);
		u->agent->addEquipment(state, item, slot);

		// Equip back the item removed earlier
		if (curItem)
		{
			for (auto &s : u->agent->type->equipment_layout->slots)
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
} // namespace OpenApoc