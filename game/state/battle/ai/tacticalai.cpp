#include "game/state/battle/ai/tacticalai.h"
#include "game/state/battle/ai/tacticalaivanilla.h"
#include "game/state/battle/battle.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

static const uint64_t TACTICAL_AI_THINK_INTERVAL = TICKS_PER_SECOND / 4;

const UString TacticalAI::getName()
{
	switch (type)
	{
		case Type::Vanilla:
			return "TacticalAIVanilla";
	}
	LogError("Unimplemented getName for Tactical AI Type %d", (int)type);
	return "";
}

void AIBlockTactical::init(GameState &state)
{
	for (auto &o : state.current_battle->participants)
	{
		if (o == state.getPlayer() || (state.current_battle->hotseat && o != state.getCivilian()))
		{
			continue;
		}
		aiList.emplace(o, mksp<TacticalAIVanilla>());
		aiList[o]->reset(state, o);
	}
	reset(state);
}

void AIBlockTactical::reset(GameState &state)
{
	ticksLastThink = state.gameTime.getTicks();
	ticksUntilReThink = randBoundsExclusive(state.rng, (uint64_t)0, TACTICAL_AI_THINK_INTERVAL * 4);
}

std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
AIBlockTactical::think(GameState &state)
{
	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilReThink > curTicks)
	{
		return {};
	}

	if (state.current_battle->mode == Battle::Mode::TurnBased &&
	    (!state.current_battle->interruptUnits.empty() ||
	     !state.current_battle->interruptQueue.empty()))
	{
		return {};
	}

	ticksLastThink = curTicks;
	ticksUntilReThink = TACTICAL_AI_THINK_INTERVAL;

	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> result;
	for (auto &o : this->aiList)
	{
		if (state.current_battle->mode == Battle::Mode::TurnBased &&
		    o.first != state.current_battle->currentActiveOrganisation)
		{
			continue;
		}
		auto decisions = o.second->think(state, o.first);
		result.insert(result.end(), decisions.begin(), decisions.end());
	}
	return result;
}

void AIBlockTactical::beginTurnRoutine(GameState &state, StateRef<Organisation> org)
{
	if (aiList.find(org) != aiList.end())
	{
		aiList[org]->beginTurnRoutine(state, org);
	}
}
} // namespace OpenApoc