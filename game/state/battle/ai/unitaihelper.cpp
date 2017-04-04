#include "game/state/battle/ai/unitaihelper.h"
#include "game/state/aequipment.h"
#include "game/state/battle/ai/aidecision.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

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
}