#pragma once

extern "C"
{
#include "dependencies/lua/lua.h"
}
#include "library/strings.h"

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
	int callHook(const UString &hookName, int nargs, int nresults);
	bool runScript(const UString &scriptName);
};
} // namespace OpenApoc
