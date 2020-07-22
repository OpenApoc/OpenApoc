#include "game/state/battle/ai/tacticalaivanilla.h"
#include "game/state/battle/ai/unitaihelper.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

TacticalAIVanilla::TacticalAIVanilla() { type = Type::Vanilla; }

void TacticalAIVanilla::beginTurnRoutine(GameState &state, StateRef<Organisation> o)
{
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner != o || !u.second->isConscious())
		{
			continue;
		}
		u.second->cancelMissions(state);
		u.second->aiList.beginTurnRoutine(state, *u.second);
	}
}

std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
TacticalAIVanilla::think(GameState &state, StateRef<Organisation> o)
{
	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> result = {};
	int countOrdersGiven = 0;

	// If turn based allow turn end when we're finished
	if (state.current_battle->mode == Battle::Mode::TurnBased &&
	    state.current_battle->ticksWithoutAction >= TICKS_END_TURN)
	{
		state.current_battle->turnEndAllowed = true;
	}

	int unitsTotal = 0;
	int unitsActive = 0;
	for (auto &u : state.current_battle->units)
	{
		if (u.second->owner != o)
		{
			continue;
		}
		unitsTotal++;
		if (u.second->isConscious())
		{
			unitsActive++;
		}
	}
	LogAssert(unitsTotal > 0);
	// Chance to retreat is [0 to 50]% as number of neutralised allies goes [50 to 100]%
	bool retreat =
	    randBoundsExclusive(state.rng, 0, 100) < (unitsActive - unitsTotal / 2) / unitsTotal;

	// Find an idle unit that needs orders
	for (auto &u : state.current_battle->units)
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
		auto decisions =
		    retreat ? std::make_tuple(
		                  std::list<StateRef<BattleUnit>>{StateRef<BattleUnit>(&state, u.first)},
		                  UnitAIHelper::getRetreatMovement(state, *u.second, true))
		            : getPatrolMovement(state, *u.second);

		auto units = std::get<0>(decisions);
		if (units.empty())
		{
			continue;
		}
		AIDecision decision = {nullptr, std::get<1>(decisions)};

		// Randomize civ movement kind
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

		result.emplace_back(units, decision);
		countOrdersGiven++;

		// For now, stop after giving orders to three group of units
		if (countOrdersGiven >= 3)
		{
			return result;
		}
	}

	return result;
}

// FIXME: Allow for patrol to a point other than block's center
std::tuple<std::list<StateRef<BattleUnit>>, sp<AIMovement>>
TacticalAIVanilla::getPatrolMovement(GameState &state, BattleUnit &u)
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
		auto lbID = pickRandom(state.rng, state.current_battle->losBlockRandomizer);

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
		for (auto &unit : units)
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
} // namespace OpenApoc
