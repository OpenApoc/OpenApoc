#include "game/state/battle/ai/unitai.h"
#include "game/state/battle/ai/unitaibehavior.h"
#include "game/state/battle/ai/unitaidefault.h"
#include "game/state/battle/ai/unitaihardcore.h"
#include "game/state/battle/ai/unitailowmorale.h"
#include "game/state/battle/ai/unitaivanilla.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"

extern "C"
{
#include "dependencies/lua/lua.h"
#include "dependencies/lua/lualib.h"
#include "dependencies/lua/lauxlib.h"
}

namespace OpenApoc
{

static const uint64_t UNIT_AI_THINK_INTERVAL = TICKS_PER_SECOND / 8;

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

std::tuple<AIDecision, bool> UnitAI::luaThink(GameState &state, BattleUnit &u, bool interrupt)
{
	lua_createtable(luaEngine, 0, 4);

	lua_pushstring(luaEngine, u.id.c_str());
	lua_setfield(luaEngine, -2, "id");
	lua_pushstring(luaEngine, u.agent.get()->name.c_str());
	lua_setfield(luaEngine, -2, "agent_name");

	lua_setglobal(luaEngine, "battle_unit");

	lua_getglobal(luaEngine, this->getName().c_str());
	lua_pushnumber(luaEngine, 0);

	lua_call(luaEngine, 1, 1);
	lua_pop(luaEngine, 1); /* pop returned value */

	return {};
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
	ticksLastOutOfOrderThink = 0;
	ticksUntilReThink = randBoundsExclusive(state.rng, 0, (int)TICKS_PER_SECOND);

	for (auto &ai : aiList)
	{
		ai->reset(state, u);
	}
}

// void AIBlockUnit::reportExecuted(AIAction &action)
//{
//	for (auto &ai : aiList)
//	{
//		ai->reportExecuted(action);
//	}
//}

void AIBlockUnit::reportExecuted(AIMovement &movement)
{
	for (auto &ai : aiList)
	{
		ai->reportExecuted(movement);
	}
}

void AIBlockUnit::beginTurnRoutine(GameState &state, BattleUnit &u)
{
	for (auto &ai : aiList)
	{
		ai->routine(state, u);
	}
}

AIBlockUnit::AIBlockUnit()
{
	luaEngine = luaL_newstate();
	luaL_openlibs(luaEngine);

	if (luaL_dofile(luaEngine, "data/scripts/unitai.lua") == LUA_OK) {
		lua_pop(luaEngine, lua_gettop(luaEngine));
	}
}

AIBlockUnit::~AIBlockUnit()
{
	lua_close(luaEngine);
}

AIDecision AIBlockUnit::think(GameState &state, BattleUnit &u, bool forceInterrupt)
{
	auto curTicks = state.gameTime.getTicks();
	if (ticksLastThink + ticksUntilReThink > curTicks)
	{
		if (u.visibleUnits.empty() || u.isMoving() || u.isAttacking() ||
		    u.psiStatus != PsiStatus::NotEngaged ||
		    ticksLastOutOfOrderThink + ticksUntilReThink > curTicks)
		{
			return {};
		}
		ticksLastOutOfOrderThink = curTicks;
	}

	bool interrupt = forceInterrupt;
	if (!interrupt && state.current_battle->mode == Battle::Mode::TurnBased)
	{
		if (!state.current_battle->interruptQueue.empty())
		{
			return {};
		}
		if (u.owner != state.current_battle->currentActiveOrganisation ||
		    !state.current_battle->interruptUnits.empty())
		{
			auto it = state.current_battle->interruptUnits.find({&state, u.id});
			interrupt = it != state.current_battle->interruptUnits.end() &&
			            u.agent->modified_stats.time_units > it->second;
			if (!interrupt)
			{
				return {};
			}
		}
	}

	ticksLastThink = curTicks;
	ticksUntilReThink = UNIT_AI_THINK_INTERVAL;

	AIDecision decision = {};

	for (auto &ai : aiList)
	{
		if (forceInterrupt)
		{
			ai->reset(state, u);
		}
		ai->setLuaEngine(luaEngine);
		ai->luaThink(state, u, interrupt);
		auto result = ai->think(state, u, interrupt);
		auto newDecision = std::get<0>(result);
		auto halt = std::get<1>(result);
		if (!newDecision.isEmpty())
		{
			// We can keep last decision's movement if this one is action only,
			// because that would mean this decision may be possible to be done on the move
			// We cannot take last decision's action because it might override
			// our move or malfunction otherwise
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
} // namespace OpenApoc