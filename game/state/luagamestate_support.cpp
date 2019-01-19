extern "C" {
#include "dependencies/lua/lauxlib.h"
#include "dependencies/lua/lua.h"
#include "dependencies/lua/lualib.h"
}
#include "game/state/luagamestate_support.h"
#include "game/state/luagamestate_support_generated.h"

namespace OpenApoc
{

namespace
{
template <typename RngPrecision> int metamethodXorshift128PlusToString(lua_State *L)
{
	const Xorshift128Plus<RngPrecision> **obj =
	    (const Xorshift128Plus<RngPrecision> **)lua_touserdata(L, 1);
	lua_settop(L, 0);
	lua_pushfstring(L, "[Xorshift128Plus @ %p]", (const void *)(*obj));
	return 1;
}
}

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

template <> lua_CFunction getLuaObjectMethods<Xorshift128Plus<uint32_t>>(const std::string &key)
{
	if (key == "setState")
		return [](lua_State *L) {
			Xorshift128Plus<uint32_t> **xorshift =
			    (Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
			uint64_t buf[2] = {static_cast<uint64_t>(luaL_checkinteger(L, 2)),
			                   static_cast<uint64_t>(luaL_checkinteger(L, 3))};
			lua_settop(L, 0);
			(*xorshift)->setState(buf);
			return 0;
		};
	else if (key == "randBoundsInclusive")
		return [](lua_State *L) {
			Xorshift128Plus<uint32_t> **obj = (Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
			int minimum = luaL_checkinteger(L, 2);
			int maximum = luaL_checkinteger(L, 3);
			lua_settop(L, 0);
			lua_pushinteger(L, randBoundsInclusive(**obj, minimum, maximum));
			return 1;
		};
	else if (key == "randBoundsExclusive")
		return [](lua_State *L) {
			Xorshift128Plus<uint32_t> **obj = (Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
			int minimum = luaL_checkinteger(L, 2);
			int maximum = luaL_checkinteger(L, 3);
			lua_settop(L, 0);
			lua_pushinteger(L, randBoundsExclusive(**obj, minimum, maximum));
			return 1;
		};
	else if (key == "randBoundsReal")
		return [](lua_State *L) {
			Xorshift128Plus<uint32_t> **obj = (Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
			int minimum = luaL_checknumber(L, 2);
			int maximum = luaL_checknumber(L, 3);
			lua_settop(L, 0);
			std::uniform_real_distribution<double> dist(minimum, maximum);
			lua_pushnumber(L, dist(**obj));
			return 1;
		};
	else
		return getLuaObjectConstMethods<Xorshift128Plus<uint32_t>>(key);
}
template <>
lua_CFunction getLuaObjectConstMethods<Xorshift128Plus<uint32_t>>(const std::string &key)
{
	if (key == "getState")
		return [](lua_State *L) {
			const Xorshift128Plus<uint32_t> **xorshift =
			    (const Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
			lua_settop(L, 0);
			uint64_t buf[2];
			(*xorshift)->getState(buf);
			lua_pushinteger(L, static_cast<lua_Integer>(buf[0]));
			lua_pushinteger(L, static_cast<lua_Integer>(buf[1]));
			return 2;
		};
	return nullptr;
}

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

void pushToLua(lua_State *L, const UString &v) { lua_pushlstring(L, v.cStr(), v.cStrLength()); }
void pushToLua(lua_State *L, const char *v) { lua_pushstring(L, v); }
void pushToLua(lua_State *L, const std::string &v) { lua_pushlstring(L, v.c_str(), v.size()); }
void pushToLua(lua_State *L, bool v) { lua_pushboolean(L, v); }
void pushToLua(lua_State *L, int v) { lua_pushinteger(L, v); }
void pushToLua(lua_State *L, unsigned int v) { lua_pushinteger(L, v); }
void pushToLua(lua_State *L, uint64_t v) { lua_pushinteger(L, v); }
void pushToLua(lua_State *L, float v) { lua_pushnumber(L, v); }
void pushToLua(lua_State *L, double v) { lua_pushnumber(L, v); }
void pushToLua(lua_State *L, sp<Image> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, sp<LazyImage> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, sp<VoxelSlice> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, sp<Sample> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, sp<VoxelMap> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, Colour &v)
{
	lua_createtable(L, 0, 4);
	lua_pushinteger(L, v.r);
	lua_setfield(L, -2, "r");
	lua_pushinteger(L, v.g);
	lua_setfield(L, -2, "g");
	lua_pushinteger(L, v.b);
	lua_setfield(L, -2, "b");
	lua_pushinteger(L, v.a);
	lua_setfield(L, -2, "a");
}
void pushToLua(lua_State *L, const UnitAI &v) { pushToLua(L, v.type); }
void pushToLua(lua_State *L, sp<TacticalAI> &v) { lua_pushnil(L); }

void pushToLua(lua_State *L, Xorshift128Plus<uint32_t> &v)
{
	Xorshift128Plus<uint32_t> **udata =
	    (Xorshift128Plus<uint32_t> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 0);
	lua_pushcfunction(L, [](lua_State *L) {
		Xorshift128Plus<uint32_t> **xorshift = (Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
		std::string key = lua_tostring(L, 2);
		lua_settop(L, 0);
		if (auto method = getLuaObjectMethods<Xorshift128Plus<uint32_t>>(key))
			lua_pushcfunction(L, method);
		else
			lua_pushnil(L);
		return 1;
	});
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, metamethodXorshift128PlusToString<uint32_t>);
	lua_setfield(L, -2, "__tostring");
	lua_pushstring(L, "asd");
	lua_setfield(L, -2, "__newindex");
	lua_setmetatable(L, -2);
}

void pushToLua(lua_State *L, const sp<Image> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, const sp<LazyImage> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, const sp<VoxelSlice> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, const sp<Sample> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, const sp<VoxelMap> &v) { lua_pushnil(L); }
void pushToLua(lua_State *L, const Colour &v)
{
	lua_createtable(L, 0, 4);
	lua_pushinteger(L, v.r);
	lua_setfield(L, -2, "r");
	lua_pushinteger(L, v.g);
	lua_setfield(L, -2, "g");
	lua_pushinteger(L, v.b);
	lua_setfield(L, -2, "b");
	lua_pushinteger(L, v.a);
	lua_setfield(L, -2, "a");
}
void pushToLua(lua_State *L, const Xorshift128Plus<uint32_t> &v)
{
	const Xorshift128Plus<uint32_t> **udata =
	    (const Xorshift128Plus<uint32_t> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 0);
	lua_pushcfunction(L, [](lua_State *L) {
		Xorshift128Plus<uint32_t> **xorshift = (Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
		std::string key = lua_tostring(L, 2);
		lua_settop(L, 0);
		if (auto method = getLuaObjectConstMethods<Xorshift128Plus<uint32_t>>(key))
			lua_pushcfunction(L, method);
		else
			lua_pushnil(L);
		return 1;
	});
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, metamethodXorshift128PlusToString<uint32_t>);
	lua_setfield(L, -2, "__tostring");
	lua_setmetatable(L, -2);
}
void pushToLua(lua_State *L, const sp<TacticalAI> &v) { lua_pushnil(L); }

void getFromLua(lua_State *L, int argNum, bool &v) { v = luaL_checkinteger(L, argNum); }
void getFromLua(lua_State *L, int argNum, float &v) { v = luaL_checknumber(L, argNum); }
void getFromLua(lua_State *L, int argNum, double &v) { v = luaL_checknumber(L, argNum); }
void getFromLua(lua_State *L, int argNum, UString &v)
{
	size_t len;
	const char *buf = luaL_checklstring(L, argNum, &len);
	v = UString(buf, len);
}
void getFromLua(lua_State *L, int argNum, std::string &v)
{
	size_t len;
	const char *buf = luaL_checklstring(L, argNum, &len);
	v = std::string(buf, len);
}

} // namespace OpenApoc
