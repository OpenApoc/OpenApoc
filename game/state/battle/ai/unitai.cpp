#include "game/state/battle/ai/unitai.h"
#include "game/state/battle/ai/unitaidefault.h"
#include "game/state/battle/ai/unitailowmorale.h"
#include "game/state/battle/ai/unitaibehavior.h"
#include "game/state/battle/ai/unitaivanilla.h"
#include "game/state/battle/ai/unitaihardcore.h"
#include "game/state/gamestate.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"

namespace OpenApoc
{

static const uint64_t UNIT_AI_THINK_INTERVAL = TICKS_PER_SECOND / 4;

const UString UnitAI::getName()
{
	switch (type)
	{
		case Type::LowMorale:
			return "UnitAILowMorale";
		case Type::Default:
			return "UnitAIDefault";
		case Type::Behavior:
			return "UnitAIBehavior";
		case Type::Vanilla:
			return "UnitAIVanilla";
		case Type::Hardcore:
			return "UnitAIHardcore";
	}
	LogError("Unimplemented getName for Unit AI Type %d", (int)type);
	return "";
}


void AIBlockUnit::init(GameState &state, BattleUnit &u)
{
	// FIXME: Actually read this option
	bool USER_OPTION_USE_HARDCORE_AI = false;

	aiList.clear();
	aiList.push_back(mksp<UnitAILowMorale>());
	aiList.push_back(mksp<UnitAIDefault>());
	aiList.push_back(mksp<UnitAIBehavior>());
	// Even player units have AI because when they're taken control of they will need that
	if (USER_OPTION_USE_HARDCORE_AI)
	{
		aiList.push_back(mksp<UnitAIHardcore>());
	}
	else
	{
		aiList.push_back(mksp<UnitAIVanilla>());
	}

	reset(state, u);
}

void AIBlockUnit::reset(GameState &state, BattleUnit &u)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = randBoundsExclusive(state.rng, 0, (int)TICKS_PER_SECOND);

	for (auto &ai : aiList)
	{
		ai->reset(state, u);
	}
}

AIDecision AIBlockUnit::think(GameState &state, BattleUnit &u)
{
	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilReThink > curTicks)
	{
		return {};
	}

	bool interrupt = false;
	if (state.current_battle->mode == Battle::Mode::TurnBased)
	{
		if (!state.current_battle->interruptQueue.empty())
		{
			return{};
		}
		if (u.owner != state.current_battle->currentActiveOrganisation || !state.current_battle->interruptUnits.empty())
		{
			auto it = state.current_battle->interruptUnits.find({ &state, u.id });
			interrupt = it != state.current_battle->interruptUnits.end() && u.agent->modified_stats.time_units > it->second;
			if (!interrupt)
			{
				return{};
			}
		}
	}

	ticksLastThink = curTicks;
	ticksUntilReThink = UNIT_AI_THINK_INTERVAL;

	AIDecision decision = {};
	for (auto ai : aiList)
	{
		auto result = ai->think(state, u, interrupt);
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

void AIBlockUnit::notifyUnderFire(Vec3<int> position)
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

void AIBlockUnit::notifyHit(Vec3<int> position)
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

void AIBlockUnit::notifyEnemySpotted(Vec3<int> position)
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


}