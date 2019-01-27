#pragma once

extern "C" {
#include "dependencies/lua/lua.h"
}
#include "framework/luaframework.h"
#include <string>

namespace OpenApoc
{

class GameState;

class LuaGameState
{
  private:
	// pushes OpenApoc.hook[hookName] to the top of the stack and returns true if callable. leaves
	// stack unchanged otherwise
	bool pushHook(const char *hookName);
	bool initOk = false;

  public:
	LuaGameState();
	~LuaGameState();

	void init(GameState &game);
	lua_State *L = nullptr;

	operator bool() const;

	template <typename... Args> int callHook(const UString &hookName, int nresults, Args &&... args)
	{
		pushLuaDebugTraceback(L);
		if (pushHook(hookName.str().c_str()))
		{
			pushToLua(L, args...);
			// the traceback function is under the function and its arguments
			const int msgh = -2 - static_cast<int>(sizeof...(args));
			if (lua_pcall(L, static_cast<int>(sizeof...(args)), nresults, msgh))
			{
				// this handles and removes the error object at the top of the stack
				handleLuaError(L);
			}
		}
		// remove traceback function from stack
		lua_remove(L, 1);
		// return number of results in the stack
		return lua_gettop(L);
	}
};
}
