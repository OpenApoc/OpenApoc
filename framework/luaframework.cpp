
#include "luaframework.h"
#include "configfile.h"
#include "framework.h"

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
} // namespace

void getFromLua(lua_State *L, int argNum, bool &v) { v = luaL_checkinteger(L, argNum); }
void getFromLua(lua_State *L, int argNum, float &v) { v = luaL_checknumber(L, argNum); }
void getFromLua(lua_State *L, int argNum, double &v) { v = luaL_checknumber(L, argNum); }
void getFromLua(lua_State *L, int argNum, UString &v)
{
	size_t len;
	const char *buf = luaL_checklstring(L, argNum, &len);
	v = UString(buf, len);
}
// note: constructing a new sp from the underlying object type is UB
// so we get these resources from lua through a string
void getFromLua(lua_State *L, int argNum, sp<Image> &v)
{
	UString path;
	getFromLua(L, argNum, path);
	v = fw().data->loadImage(path);
}
void getFromLua(lua_State *L, int argNum, sp<Sample> &v)
{
	UString path;
	getFromLua(L, argNum, path);
	v = fw().data->loadSample(path);
}

void pushToLua(lua_State *L, const UString &v) { lua_pushlstring(L, v.c_str(), v.length()); }
void pushToLua(lua_State *L, const char *v) { lua_pushstring(L, v); }
void pushToLua(lua_State *L, bool v) { lua_pushboolean(L, v); }
void pushToLua(lua_State *L, int v) { lua_pushinteger(L, v); }
void pushToLua(lua_State *L, unsigned int v) { lua_pushinteger(L, v); }
void pushToLua(lua_State *L, uint64_t v) { lua_pushinteger(L, v); }
void pushToLua(lua_State *L, float v) { lua_pushnumber(L, v); }
void pushToLua(lua_State *L, double v) { lua_pushnumber(L, v); }
void pushToLua(lua_State *L, sp<Image> &v)
{
	if (v)
		pushToLua(L, v->path);
	else
		lua_pushnil(L);
}
void pushToLua(lua_State *L, sp<LazyImage> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, sp<VoxelSlice> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, sp<Sample> &v)
{
	if (v)
		pushToLua(L, v->path);
	else
		lua_pushnil(L);
}
void pushToLua(lua_State *L, sp<VoxelMap> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
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

void pushToLua(lua_State *L, const sp<Image> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, const sp<LazyImage> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, const sp<VoxelSlice> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, const sp<Sample> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
void pushToLua(lua_State *L, const sp<VoxelMap> &v [[maybe_unused]])
{
	lua_pushnil(L);
	LogError("Unimplemented Lua function");
}
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

void pushToLua(lua_State *L, Xorshift128Plus<uint32_t> &v)
{
	Xorshift128Plus<uint32_t> **udata =
	    (Xorshift128Plus<uint32_t> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 0);
	lua_pushcfunction(L, [](lua_State *L) {
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
	lua_setmetatable(L, -2);
}

void pushToLua(lua_State *L, const Xorshift128Plus<uint32_t> &v)
{
	const Xorshift128Plus<uint32_t> **udata =
	    (const Xorshift128Plus<uint32_t> **)lua_newuserdata(L, sizeof(&v));
	*udata = &v;
	lua_createtable(L, 0, 0);
	lua_pushcfunction(L, [](lua_State *L) {
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

template <> lua_CFunction getLuaObjectMethods<Xorshift128Plus<uint32_t>>(const std::string &key)
{
	if (key == "seed")
		return [](lua_State *L) {
			Xorshift128Plus<uint32_t> **xorshift =
			    (Xorshift128Plus<uint32_t> **)lua_touserdata(L, 1);
			uint64_t buf = static_cast<uint64_t>(luaL_checkinteger(L, 2));
			lua_settop(L, 0);
			(*xorshift)->seed(buf);
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

void pushToLua(lua_State *L [[maybe_unused]]) { LogError("Unimplemented Lua function"); }

void pushLuaDebugTraceback(lua_State *L)
{
	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	lua_remove(L, -2); // remove debug table
}

void handleLuaError(lua_State *L, LogLevel level)
{
	size_t count;
	const char *buf = luaL_checklstring(L, -1, &count);
	UString str{buf, count};
	// TODO: unwind stack and show previously called c function?
	Log(level, LOGGER_PREFIX, "Lua: " + str);
	lua_pop(L, 1); // pop error object we just printed
}

void pushLuaFramework(lua_State *L)
{
	lua_createtable(L, 0, 2);
	// push logging functions. note we only accept one string as argument
	// because you can just do all the formatting and concatenation in lua
	lua_pushcfunction(L, [](lua_State *L) {
		lua_settop(L, 1);
		handleLuaError(L, LogLevel::Info);
		return 0;
	});
	lua_setfield(L, -2, "LogInfo");
	lua_pushcfunction(L, [](lua_State *L) {
		lua_settop(L, 1);
		handleLuaError(L, LogLevel::Warning);
		return 0;
	});
	lua_setfield(L, -2, "LogWarning");
	lua_pushcfunction(L, [](lua_State *L) {
		lua_settop(L, 1);
		handleLuaError(L, LogLevel::Error);
		return 0;
	});
	lua_setfield(L, -2, "LogError");

	// config table
	lua_createtable(L, 0, 2);

	lua_pushcfunction(L, [](lua_State *L) {
		UString key;
		getFromLua(L, 1, key);
		lua_settop(L, 0);
		pushToLua(L, config().getString(key));
		return 1;
	});
	lua_setfield(L, -2, "getString");

	lua_pushcfunction(L, [](lua_State *L) {
		UString key;
		getFromLua(L, 1, key);
		lua_settop(L, 0);
		pushToLua(L, config().getInt(key));
		return 1;
	});
	lua_setfield(L, -2, "getInt");

	lua_pushcfunction(L, [](lua_State *L) {
		UString key;
		getFromLua(L, 1, key);
		lua_settop(L, 0);
		pushToLua(L, config().getBool(key));
		return 1;
	});
	lua_setfield(L, -2, "getBool");

	lua_pushcfunction(L, [](lua_State *L) {
		UString key;
		getFromLua(L, 1, key);
		lua_settop(L, 0);
		pushToLua(L, config().getFloat(key));
		return 1;
	});
	lua_setfield(L, -2, "getFloat");

	lua_pushcfunction(L, [](lua_State *L) {
		UString section;
		UString key;
		getFromLua(L, 1, section);
		getFromLua(L, 2, key);
		lua_settop(L, 0);
		pushToLua(L, config().describe(section, key));
		return 1;
	});
	lua_setfield(L, -2, "describe");

	lua_setfield(L, -2, "Config");
}

} // namespace OpenApoc
