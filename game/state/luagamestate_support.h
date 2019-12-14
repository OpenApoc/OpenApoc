#pragma once

#include "framework/luaframework.h"

#include "game/state/battle/ai/tacticalaivanilla.h"
#include "game/state/battle/ai/unitai.h"
#include "game/state/battle/ai/unitaibehavior.h"
#include "game/state/battle/ai/unitaidefault.h"
#include "game/state/battle/ai/unitaihardcore.h"
#include "game/state/battle/ai/unitailowmorale.h"
#include "game/state/battle/ai/unitaivanilla.h"
#include "game/state/battle/battle.h"
#include "game/state/rules/battle/battlemaptileset.h"
#include "game/state/stateobject.h"
// required for luagamestate_support_generated
#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battleexplosion.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/economyinfo.h"
#include "game/state/city/facility.h"
#include "game/state/city/research.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/gametime.h"
#include "game/state/message.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/rules/battle/battlemaptileset.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/rules/city/baselayout.h"
#include "game/state/rules/city/citycommonimagelist.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/rules/city/facilitytype.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/ufogrowth.h"
#include "game/state/rules/city/ufoincursion.h"
#include "game/state/rules/city/ufomissionpreference.h"
#include "game/state/rules/city/ufopaedia.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/shared/aequipment.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/organisation.h"
#include "game/state/shared/projectile.h"
#include "game/state/stateobject.h"

namespace OpenApoc
{

// we can always get the GameState from a lua_State through the registry
GameState *getGameStateFromLua(lua_State *L);
const GameState *getConstGameStateFromLua(lua_State *L);

template <typename T> void getFromLua(lua_State *L, int argNum, StateRef<T> &v)
{
	// the luagamestate always has a pointer to the gamestate in OpenApoc.GameState
	// so we can construct a stateref just by having the id in the stack
	UString refid;
	getFromLua(L, argNum, refid);
	const GameState *gameState = getConstGameStateFromLua(L);
	v = {gameState, refid};
}

// overloaded functions for pushing objects into the lua stack
void pushToLua(lua_State *L, sp<TacticalAI> &v);
void pushToLua(lua_State *L, UnitAI &v);
void pushToLua(lua_State *L, const sp<TacticalAI> &v);
void pushToLua(lua_State *L, const UnitAI &v);

template <typename T> void pushToLua(lua_State *L, StateRef<T> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.id);
	lua_setfield(L, -2, "id");
	if (v)
	{
		pushToLua(L, *v);
		lua_setfield(L, -2, "object");
	}
}
template <typename T> void pushToLua(lua_State *L, const StateRef<T> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.id);
	lua_setfield(L, -2, "id");
	if (v)
	{
		pushToLua(L, *v);
		lua_setfield(L, -2, "object");
	}
}

template <> lua_CFunction getLuaObjectMethods<GameState>(const std::string &key);
template <> lua_CFunction getLuaObjectMethods<Agent>(const std::string &key);
template <> lua_CFunction getLuaObjectMethods<City>(const std::string &key);
template <> lua_CFunction getLuaObjectConstMethods<AgentGenerator>(const std::string &key);
template <> lua_CFunction getLuaObjectConstMethods<GameTime>(const std::string &key);

// implemented in luagamestate_support_generated.cpp file
void pushLuaGamestateEnums(lua_State *L);

} // namespace OpenApoc
