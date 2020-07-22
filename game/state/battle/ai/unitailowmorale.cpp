#include "game/state/battle/ai/unitailowmorale.h"
#include "game/state/battle/ai/aidecision.h"
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/shared/aequipment.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

namespace
{
static const std::tuple<AIDecision, bool> NULLTUPLE2 = std::make_tuple(AIDecision(), false);
}

static const uint64_t LOWMORALE_AI_INTERVAL = TICKS_PER_SECOND * 2;

UnitAILowMorale::UnitAILowMorale() { type = Type::LowMorale; }

void UnitAILowMorale::reset(GameState &, BattleUnit &) { ticksActionAvailable = 0; }

std::tuple<AIDecision, bool> UnitAILowMorale::think(GameState &state, BattleUnit &u, bool interrupt)
{
	std::ignore = interrupt;
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

	if (u.agent->modified_stats.time_units == 0)
	{
		return std::make_tuple(AIDecision(), true);
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
						auto targetLB = pickRandom(state.rng, adjacentBlocks);
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
				auto e1 = u.agent->getFirstItemInSlot(EquipmentSlotType::LeftHand);
				auto e2 = u.agent->getFirstItemInSlot(EquipmentSlotType::RightHand);
				auto canFire = ((e1 && e1->canFire(state)) || (e2 && e2->canFire(state)));
				// Roll for what kind of action we take with berserk
				int roll = randBoundsExclusive(state.rng, 0, 100);
				// 20% chance to attack a friendly, 40% chance to attack an enemy, 40% chance to
				// attack random tile
				int shootType = roll < 20 ? 1 : (roll < 60 ? 2 : 3);
				// Intentional fall-through in case we cannot find a valid target
				switch (shootType)
				{
					case 1:
					{
						// Pick a random friendly within 5 tiles of us
						std::list<sp<BattleUnit>> victims;
						for (auto &unit : state.current_battle->units)
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
							auto victim = pickRandom(state.rng, victims);
							if (!canFire)
							{
								decision.movement = mksp<AIMovement>();
								decision.movement->type = AIMovement::Type::Turn;
								decision.movement->targetLocation = victim->position;
								break;
							}
							else
							{
								if (u.canAttackUnit(state, victim) != WeaponStatus::NotFiring)
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
							auto target = pickRandom(state.rng, u.visibleEnemies);
							if (!canFire)
							{
								decision.movement = mksp<AIMovement>();
								decision.movement->type = AIMovement::Type::Turn;
								decision.movement->targetLocation = target->position;
								break;
							}
							else
							{
								if (u.canAttackUnit(state, target) != WeaponStatus::NotFiring)
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
} // namespace OpenApoc