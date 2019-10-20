#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include <map>

namespace OpenApoc
{

class BattleMapPartType;

class BattleMapTileset
{
  public:
	std::map<UString, sp<BattleMapPartType>> map_part_types;

	// high level api for loading tilesets
	bool loadTileset(GameState &state, const UString &path);

	// high level api for saving tilesets
	bool saveTileset(const UString &path, bool pack = true, bool pretty = false);

	static UString getTilesetPath();
};
} // namespace OpenApoc
