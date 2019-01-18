#pragma once

extern "C" {
#include "dependencies/lua/lauxlib.h"
#include "dependencies/lua/lua.h"
#include "dependencies/lua/lualib.h"
}
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
	void pushDebugTraceback();
	void handleLuaError();
	bool initOk = false;

  public:
	LuaGameState();
	~LuaGameState();

	void init(GameState &game);
	lua_State *L = nullptr;

	void updateEconomyHook();
	void newGameHook();
	void newGamePostInitHook();
	operator bool() const;
};
}
