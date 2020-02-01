extern "C"
{
#include "dependencies/lua/lauxlib.h"
#include "dependencies/lua/lua.h"
#include "dependencies/lua/lualib.h"
}
#include "game/state/luagamestate_support.h"
#include "game/state/luagamestate_support_generated.h"

namespace OpenApoc
{

GameState *getGameStateFromLua(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "OpenApoc.GameState");
	GameState *r = (GameState *)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return r;
}
const GameState *getConstGameStateFromLua(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "OpenApoc.GameState");
	const GameState *r = (const GameState *)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return r;
}

void pushToLua(lua_State *L, sp<TacticalAI> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, UnitAI &v) { pushToLua(L, v.type); }
void pushToLua(lua_State *L, const sp<TacticalAI> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, const UnitAI &v) { pushToLua(L, v.type); }

template <> lua_CFunction getLuaObjectMethods<GameState>(const std::string &key)
{
	if (key == "loadGame")
		return [](lua_State *L) {
			GameState **obj = (GameState **)lua_touserdata(L, 1);
			UString path;
			getFromLua(L, 2, path);
			lua_settop(L, 0);
			(*obj)->loadGame(path);
			return 0;
		};
	else if (key == "appendGameState")
		return [](lua_State *L) {
			GameState **obj = (GameState **)lua_touserdata(L, 1);
			UString path;
			getFromLua(L, 2, path);
			lua_settop(L, 0);
			pushToLua(L, (*obj)->appendGameState(path));
			return 1;
		};
	return getLuaObjectConstMethods<GameState>(key);
}

template <> lua_CFunction getLuaObjectMethods<Agent>(const std::string &key)
{
	if (key == "enterBuilding")
		return [](lua_State *L) {
			Agent **obj = (Agent **)lua_touserdata(L, 1);
			StateRef<Building> building;
			getFromLua(L, 2, building);
			GameState *gameState = getGameStateFromLua(L);
			lua_settop(L, 0);
			(*obj)->enterBuilding(*gameState, building);
			return 0;
		};
	return getLuaObjectConstMethods<Agent>(key);
}

template <> lua_CFunction getLuaObjectMethods<City>(const std::string &key)
{
	if (key == "placeVehicle")
		return [](lua_State *L) {
			City **obj = (City **)lua_touserdata(L, 1);
			StateRef<VehicleType> vehicleType;
			getFromLua(L, 2, vehicleType);
			StateRef<Organisation> owner;
			getFromLua(L, 3, owner);
			lua_settop(L, 0);
			pushToLua(L, (*obj)->placeVehicle(*getGameStateFromLua(L), vehicleType, owner));
			return 1;
		};
	else if (key == "placeVehicleInBuilding")
		return [](lua_State *L) {
			City **obj = (City **)lua_touserdata(L, 1);
			StateRef<VehicleType> vehicleType;
			getFromLua(L, 2, vehicleType);
			StateRef<Organisation> owner;
			getFromLua(L, 3, owner);
			StateRef<Building> building;
			getFromLua(L, 4, building);
			lua_settop(L, 0);
			pushToLua(L,
			          (*obj)->placeVehicle(*getGameStateFromLua(L), vehicleType, owner, building));
			return 1;
		};
	else if (key == "placeVehicleAtPosition")
		return [](lua_State *L) {
			City **obj = (City **)lua_touserdata(L, 1);
			StateRef<VehicleType> vehicleType;
			getFromLua(L, 2, vehicleType);
			StateRef<Organisation> owner;
			getFromLua(L, 3, owner);
			Vec3<float> position;
			getFromLua(L, 4, position);
			float facing = static_cast<float>(luaL_optnumber(L, 5, 0.0));
			lua_settop(L, 0);
			pushToLua(L, (*obj)->placeVehicle(*getGameStateFromLua(L), vehicleType, owner, position,
			                                  facing));
			return 1;
		};
	return getLuaObjectConstMethods<City>(key);
}

template <> lua_CFunction getLuaObjectConstMethods<AgentGenerator>(const std::string &key)
{
	if (key == "createAgent")
		return [](lua_State *L) {
			const AgentGenerator **obj = (const AgentGenerator **)lua_touserdata(L, 1);
			StateRef<Organisation> organisation;
			getFromLua(L, 2, organisation);
			StateRef<AgentType> agentType;
			getFromLua(L, 3, agentType);
			GameState *gameState = getGameStateFromLua(L);
			lua_settop(L, 0);
			// put the result into a local variable so we can push with the non-const overload
			auto ref = (*obj)->createAgent(*gameState, organisation, agentType);
			pushToLua(L, ref);
			return 1;
		};
	return nullptr;
}

template <> lua_CFunction getLuaObjectConstMethods<GameTime>(const std::string &key)
{
	if (key == "getHours")
		return [](lua_State *L) {
			const GameTime **obj = (const GameTime **)lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_pushinteger(L, (*obj)->getHours());
			return 1;
		};
	else if (key == "getMinutes")
		return [](lua_State *L) {
			const GameTime **obj = (const GameTime **)lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_pushinteger(L, (*obj)->getMinutes());
			return 1;
		};
	else if (key == "getDay")
		return [](lua_State *L) {
			const GameTime **obj = (const GameTime **)lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_pushinteger(L, (*obj)->getDay());
			return 1;
		};
	else if (key == "getWeek")
		return [](lua_State *L) {
			const GameTime **obj = (const GameTime **)lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_pushinteger(L, (*obj)->getWeek());
			return 1;
		};
	return nullptr;
}

} // namespace OpenApoc
