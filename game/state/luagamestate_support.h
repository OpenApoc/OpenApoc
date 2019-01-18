#pragma once

#include "framework/data.h"
#include "framework/image.h"
#include "framework/sound.h"
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
#include "library/colour.h"
#include "library/strings.h"
#include "library/vec.h"
#include "library/voxel.h"
#include "library/xorshift.h"
// required for luagamestate_support_generated
#include "game/state/battle/battleunitmission.h"
#include "game/state/city/city.h"
#include "game/state/city/facility.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/doodadtype.h"

#include <unordered_map>

namespace OpenApoc
{

// we can always get the GameState from a lua_State through the registry
GameState *getGameStateFromLua(lua_State *L);
const GameState *getConstGameStateFromLua(lua_State *L);

// functions for getting objects from the lua stack
void getFromLua(lua_State *L, int argNum, bool &v);
template <typename Int>
typename std::enable_if<std::is_integral<Int>::value, void>::type getFromLua(lua_State *L,
                                                                             int argNum, Int &v)
{
	v = static_cast<Int>(luaL_checkinteger(L, argNum));
}
void getFromLua(lua_State *L, int argNum, float &v);
void getFromLua(lua_State *L, int argNum, double &v);
void getFromLua(lua_State *L, int argNum, UString &v);
void getFromLua(lua_State *L, int argNum, std::string &v);
template <typename T> void getFromLua(lua_State *L, int argNum, StateRef<T> &v)
{
	// the luagamestate always has a pointer to the gamestate in OpenApoc.GameState
	// so we can construct a stateref just by having the id in the stack
	UString refid;
	getFromLua(L, argNum, refid);
	const GameState *gameState = getConstGameStateFromLua(L);
	v = {gameState, refid};
}
template <typename T> void getFromLua(lua_State *L, int argNum, const T &v)
{
	luaL_error(L, "this member cannot be set directly");
}

// overloaded functions for pushing objects into the lua stack
void pushToLua(lua_State *L, const UString &v);
void pushToLua(lua_State *L, const char *v);
void pushToLua(lua_State *L, const std::string &v);
void pushToLua(lua_State *L, bool v);
void pushToLua(lua_State *L, int v);
void pushToLua(lua_State *L, uint64_t v);
void pushToLua(lua_State *L, unsigned int v);
void pushToLua(lua_State *L, float v);
void pushToLua(lua_State *L, double v);
void pushToLua(lua_State *L, sp<Image> &v);
void pushToLua(lua_State *L, sp<LazyImage> &v);
void pushToLua(lua_State *L, sp<VoxelSlice> &v);
void pushToLua(lua_State *L, sp<Sample> &v);
void pushToLua(lua_State *L, sp<VoxelMap> &v);
void pushToLua(lua_State *L, sp<TacticalAI> &v);
void pushToLua(lua_State *L, Colour &v);
void pushToLua(lua_State *L, const UnitAI &v);
void pushToLua(lua_State *L, Xorshift128Plus<uint32_t> &t);

void pushToLua(lua_State *L, const UString &v);
void pushToLua(lua_State *L, const char *v);
void pushToLua(lua_State *L, const std::string &v);
void pushToLua(lua_State *L, bool v);
void pushToLua(lua_State *L, int v);
void pushToLua(lua_State *L, uint64_t v);
void pushToLua(lua_State *L, unsigned int v);
void pushToLua(lua_State *L, float v);
void pushToLua(lua_State *L, double v);
void pushToLua(lua_State *L, const sp<Image> &v);
void pushToLua(lua_State *L, const sp<LazyImage> &v);
void pushToLua(lua_State *L, const sp<VoxelSlice> &v);
void pushToLua(lua_State *L, const sp<Sample> &v);
void pushToLua(lua_State *L, const sp<VoxelMap> &v);
void pushToLua(lua_State *L, const sp<TacticalAI> &v);
void pushToLua(lua_State *L, const Colour &v);
void pushToLua(lua_State *L, const UnitAI &v);
void pushToLua(lua_State *L, const Xorshift128Plus<uint32_t> &t);

template <typename T> void pushToLua(lua_State *L, Vec3<T> &v)
{
	lua_createtable(L, 0, 3);
	pushToLua(L, v.x);
	lua_setfield(L, -2, "x");
	pushToLua(L, v.y);
	lua_setfield(L, -2, "y");
	pushToLua(L, v.z);
	lua_setfield(L, -2, "z");
}
template <typename T> void pushToLua(lua_State *L, const Vec3<T> &v)
{
	lua_createtable(L, 0, 3);
	pushToLua(L, v.x);
	lua_setfield(L, -2, "x");
	pushToLua(L, v.y);
	lua_setfield(L, -2, "y");
	pushToLua(L, v.z);
	lua_setfield(L, -2, "z");
}

template <typename T> void pushToLua(lua_State *L, Vec2<T> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.x);
	lua_setfield(L, -2, "x");
	pushToLua(L, v.y);
	lua_setfield(L, -2, "y");
}
template <typename T> void pushToLua(lua_State *L, const Vec2<T> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.x);
	lua_setfield(L, -2, "x");
	pushToLua(L, v.y);
	lua_setfield(L, -2, "y");
}
template <typename T> void pushToLua(lua_State *L, Rect<T> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.p0);
	lua_setfield(L, -2, "p0");
	pushToLua(L, v.p1);
	lua_setfield(L, -2, "p1");
}
template <typename T> void pushToLua(lua_State *L, const Rect<T> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.p0);
	lua_setfield(L, -2, "p0");
	pushToLua(L, v.p1);
	lua_setfield(L, -2, "p1");
}

template <typename T> void pushToLua(lua_State *L, sp<T> &v)
{
	if (v)
		pushToLua(L, *v.get());
	else
		lua_pushnil(L);
}
template <typename T> void pushToLua(lua_State *L, const sp<T> &v)
{
	if (v)
		pushToLua(L, *v.get());
	else
		lua_pushnil(L);
}
template <typename T> void pushToLua(lua_State *L, up<T> &v)
{
	if (v)
		pushToLua(L, *v.get());
	else
		lua_pushnil(L);
}
template <typename T> void pushToLua(lua_State *L, const up<T> &v)
{
	if (v)
		pushToLua(L, *v.get());
	else
		lua_pushnil(L);
}
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

template <typename T, typename U> void pushToLua(lua_State *L, std::pair<T, U> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.first);
	lua_setfield(L, -2, "first");
	pushToLua(L, v.second);
	lua_setfield(L, -2, "second");
}
template <typename T, typename U> void pushToLua(lua_State *L, const std::pair<T, U> &v)
{
	lua_createtable(L, 0, 2);
	pushToLua(L, v.first);
	lua_setfield(L, -2, "first");
	pushToLua(L, v.second);
	lua_setfield(L, -2, "second");
}

// pushing containers to lua
template <typename C>
using LuaIteratorState = std::pair<typename C::iterator, typename C::iterator>;
template <typename C> int advanceIpairsIterator(lua_State *L);
template <typename C> int generateIpairsIterator(lua_State *L);
template <typename C> int advancePairsIterator(lua_State *L);
template <typename C> int generatePairsIterator(lua_State *L);
template <typename C> int containerIndexInteger(lua_State *L);
template <typename C> int containerIndexMap(lua_State *L);

template <typename T> void pushToLua(lua_State *L, std::list<T> &v);
template <typename T> void pushToLua(lua_State *L, const std::list<T> &v);
template <typename T> void pushToLua(lua_State *L, std::vector<T> &v);
template <typename T> void pushToLua(lua_State *L, const std::vector<T> &v);

template <typename K, typename V> void pushToLua(lua_State *L, std::map<K, V> &v);
template <typename K, typename V> void pushToLua(lua_State *L, const std::map<K, V> &v);

template <typename T> void pushToLua(lua_State *L, std::set<T> &v);
template <typename T> void pushToLua(lua_State *L, const std::set<T> &v);

template <typename C> int containerLength(lua_State *L)
{
	C **v = (C **)lua_touserdata(L, -1);
	lua_pop(L, 1);
	lua_pushinteger(L, (*v)->size());
	return 1;
}

template <typename T> void pushToLua(lua_State *L, std::list<T> &v)
{
	std::list<T> **udata = (std::list<T> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 2);
	lua_pushcfunction(L, generateIpairsIterator<std::list<T>>);
	lua_setfield(L, -2, "__ipairs");
	lua_pushcfunction(L, containerLength<std::list<T>>);
	lua_setfield(L, -2, "__len");
	lua_setmetatable(L, -2);
}
template <typename T> void pushToLua(lua_State *L, const std::list<T> &v)
{
	lua_createtable(L, v.size(), 0);
	int i = 0;
	for (const auto &item : v)
	{
		pushToLua(L, item);
		lua_seti(L, -2, i + 1);
		++i;
	}
}

template <typename T> void pushToLua(lua_State *L, std::vector<T> &v)
{
	std::vector<T> **udata = (std::vector<T> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 3);
	lua_pushcfunction(L, generateIpairsIterator<std::vector<T>>);
	lua_setfield(L, -2, "__ipairs");
	lua_pushcfunction(L, containerLength<std::vector<T>>);
	lua_setfield(L, -2, "__len");
	lua_pushcfunction(L, containerIndexInteger<std::vector<T>>);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);
}
template <typename T> void pushToLua(lua_State *L, const std::vector<T> &v)
{
	lua_createtable(L, v.size(), 0);
	for (size_t i = 0; i < v.size(); ++i)
	{
		pushToLua(L, v[i]);
		lua_seti(L, -2, i + 1);
	}
}

template <typename K, typename V> void pushToLua(lua_State *L, std::map<K, V> &v)
{
	std::map<K, V> **udata = (std::map<K, V> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 2);
	lua_pushcfunction(L, (generatePairsIterator<std::map<K, V>>));
	lua_setfield(L, -2, "__pairs");
	lua_pushcfunction(L, (containerLength<std::map<K, V>>));
	lua_setfield(L, -2, "__len");
	lua_pushcfunction(L, (containerIndexMap<std::map<K, V>>));
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);
}
template <typename K, typename V> void pushToLua(lua_State *L, const std::map<K, V> &v)
{
	lua_createtable(L, 0, 0);
	for (const auto &p : v)
	{
		pushToLua(L, p.first);
		pushToLua(L, p.second);
		lua_settable(L, -3);
	}
}

template <typename T> void pushToLua(lua_State *L, std::set<T> &v)
{
	std::set<T> **udata = (std::set<T> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 2);
	lua_pushcfunction(L, generateIpairsIterator<std::set<T>>);
	lua_setfield(L, -2, "__pairs");
	lua_pushcfunction(L, (containerLength<std::set<T>>));
	lua_setfield(L, -2, "__len");
	lua_setmetatable(L, -2);
}
template <typename T> void pushToLua(lua_State *L, const std::set<T> &v)
{
	lua_createtable(L, v.size(), 0);
	int i = 0;
	for (const auto &item : v)
	{
		pushToLua(L, v);
		lua_seti(L, -2, i + 1);
		++i;
	}
}

template <typename C> int advanceIpairsIterator(lua_State *L)
{
	LuaIteratorState<C> *it = (LuaIteratorState<C> *)lua_touserdata(L, 1);
	int i = luaL_checkinteger(L, 2);
	lua_settop(L, 0);
	if (it->first != it->second)
	{
		pushToLua(L, i + 1);
		pushToLua(L, *(it->first));
		++it->first;
		return 2;
	}
	return 0;
}
template <typename C> int generateIpairsIterator(lua_State *L)
{
	C **v = (C **)lua_touserdata(L, 1);
	lua_pop(L, 1);
	lua_pushcfunction(L, advanceIpairsIterator<C>);
	LuaIteratorState<C> *it =
	    (LuaIteratorState<C> *)lua_newuserdata(L, sizeof(LuaIteratorState<C>));
	it->first = (*v)->begin();
	it->second = (*v)->end();
	lua_pushinteger(L, 0);
	return 2;
}

template <typename C> int advancePairsIterator(lua_State *L)
{
	LuaIteratorState<C> *it = (LuaIteratorState<C> *)lua_touserdata(L, 1);
	lua_settop(L, 0);
	if (it->first != it->second)
	{
		pushToLua(L, it->first->first);
		pushToLua(L, it->first->second);
		++it->first;
		return 2;
	}
	return 0;
}
template <typename C> int generatePairsIterator(lua_State *L)
{
	C **v = (C **)lua_touserdata(L, 1);
	lua_settop(L, 0);
	lua_pushcfunction(L, advancePairsIterator<C>);
	LuaIteratorState<C> *it =
	    (LuaIteratorState<C> *)lua_newuserdata(L, sizeof(LuaIteratorState<C>));
	it->first = (*v)->begin();
	it->second = (*v)->end();
	return 2;
}

template <typename C> int containerIndexInteger(lua_State *L)
{
	C **v = (C **)lua_touserdata(L, 1);
	int i = luaL_checkinteger(L, 2);
	lua_settop(L, 0);
	--i; // lua indices start at 1
	if (i >= 0 && i < (*v)->size())
	{
		pushToLua(L, (*v)->operator[](i));
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}
template <typename C> int containerIndexMap(lua_State *L)
{
	C **v = (C **)lua_touserdata(L, 1);
	typename C::key_type k;
	getFromLua(L, 2, k);
	lua_settop(L, 0);
	auto it = (*v)->find(k);
	if (it != (*v)->end())
	{
		pushToLua(L, it->second);
	}
	else
	{
		lua_pushnil(L);
	}
	return 1;
}

// static const mapping between method name to a pointer-to-function
// of the actual implementation
template <typename T> lua_CFunction getLuaObjectConstMethods(const std::string &key)
{
	return nullptr;
}

template <typename T> lua_CFunction getLuaObjectMethods(const std::string &key)
{
	// if no non-const method is found, look in the const methods
	return getLuaObjectConstMethods<T>(key);
}
// actual overloads for each class
template <> lua_CFunction getLuaObjectMethods<Xorshift128Plus<uint32_t>>(const std::string &key);
template <>
lua_CFunction getLuaObjectConstMethods<Xorshift128Plus<uint32_t>>(const std::string &key);
template <> lua_CFunction getLuaObjectMethods<GameState>(const std::string &key);
template <> lua_CFunction getLuaObjectMethods<Agent>(const std::string &key);
template <> lua_CFunction getLuaObjectConstMethods<AgentGenerator>(const std::string &key);
template <> lua_CFunction getLuaObjectConstMethods<GameTime>(const std::string &key);

// implemeted in luagamestate_support_generated.cpp file
void pushLuaEnums(lua_State *L);

} // namespace OpenApoc
