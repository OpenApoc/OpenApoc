#pragma once

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

extern "C"
{
#include "dependencies/lua/lauxlib.h"
#include "dependencies/lua/lua.h"
#include "dependencies/lua/lualib.h"
}

#include "framework/data.h"
#include "framework/image.h"
#include "framework/sound.h"
#include "library/colour.h"
#include "library/strings.h"
#include "library/vec.h"
#include "library/voxel.h"
#include "library/xorshift.h"

namespace OpenApoc
{

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
template <typename T> void getFromLua(lua_State *L, int argNum, Vec3<T> &v)
{
	luaL_checktype(L, argNum, LUA_TTABLE);
	lua_getfield(L, argNum, "x");
	lua_getfield(L, argNum, "y");
	lua_getfield(L, argNum, "z");
	getFromLua(L, -3, v.x);
	getFromLua(L, -2, v.y);
	getFromLua(L, -1, v.z);
	lua_pop(L, 3);
}
template <typename T> void getFromLua(lua_State *L, int argNum, Vec2<T> &v)
{
	luaL_checktype(L, argNum, LUA_TTABLE);
	lua_getfield(L, argNum, "x");
	lua_getfield(L, argNum, "y");
	getFromLua(L, -2, v.x);
	getFromLua(L, -1, v.y);
	lua_pop(L, 2);
}
void getFromLua(lua_State *L, int argNum, sp<Image> &v);
void getFromLua(lua_State *L, int argNum, sp<Sample> &v);
template <typename T>
typename std::enable_if<std::is_enum<T>::value>::type getFromLua(lua_State *L, int argNum, T &v)
{
	typename std::underlying_type<T>::type integer;
	getFromLua(L, argNum, integer);
	v = static_cast<T>(integer);
}
// trying to get a value from the stack into a const reference (invalid)
template <typename T> void getFromLua(lua_State *L, int argNum, const T &v [[maybe_unused]])
{
	if (argNum < 0)
		argNum = lua_gettop(L) + argNum + 1;
	luaL_error(L, "this member (#%d) cannot be set directly", argNum);
	LogError("Invalid Lua function");
}

// functions for pushing objects to the lua stack
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
void pushToLua(lua_State *L, Colour &v);
void pushToLua(lua_State *L, Xorshift128Plus<uint32_t> &t);

void pushToLua(lua_State *L, const sp<Image> &v);
void pushToLua(lua_State *L, const sp<LazyImage> &v);
void pushToLua(lua_State *L, const sp<VoxelSlice> &v);
void pushToLua(lua_State *L, const sp<Sample> &v);
void pushToLua(lua_State *L, const sp<VoxelMap> &v);
void pushToLua(lua_State *L, const Colour &v);
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
		pushToLua(L, item);
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
	new (it) LuaIteratorState<C>((*v)->begin(), (*v)->end());
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
	new (it) LuaIteratorState<C>((*v)->begin(), (*v)->end());
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

// mapping between method name to a pointer-to-function
template <typename T>
lua_CFunction getLuaObjectConstMethods(const std::string &key [[maybe_unused]])
{
	LogError("Unimplemented Lua function");
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

void pushLuaDebugTraceback(lua_State *L);

// prints with lua prefix and pops the error object at the top of the stack
void handleLuaError(lua_State *L, LogLevel level = LogLevel::Warning);

// push a table with framework functions into the stack
void pushLuaFramework(lua_State *L);

} // namespace OpenApoc
