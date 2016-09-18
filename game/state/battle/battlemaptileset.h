#pragma once
#include "game/state/battle/battlemappart_type.h"
#include "game/state/stateobject.h"

namespace OpenApoc
{

class BattleMapTileset
{
  public:
	std::map<UString, sp<BattleMapPartType>> map_part_types;

	// high level api for loading tilesets
	bool loadTileset(GameState &state, const UString &path);

	// high level api for saving tilesets
	bool saveTileset(const UString &path, bool pack = true);

	static const UString tilesetPath;
};
}
