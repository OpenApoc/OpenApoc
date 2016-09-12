#include "game/state/battlemappart_type.h"
#include "framework/includes.h"
#include "game/state/gamestate.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/voxel.h"

namespace OpenApoc
{

template <>
sp<BattleMapPartType> StateObject<BattleMapPartType>::get(const GameState &state, const UString &id)
{
	unsigned id_length = 0;
	unsigned count_underscore = 0;
	for (auto c : id)
	{
		if (c == '_')
			count_underscore++;
		if (count_underscore == 2)
			break;
		id_length++;
	}
	if (id_length == id.length())
	{
		LogError(
		    "Wrong ID \"%s\" for a BattleMapPartType, expected it to contain a tileset name first",
		    id.cStr());
		return nullptr;
	}

	UString tileset_id = id.substr(0, id_length);
	if (state.battle_map_tilesets.find(tileset_id) == state.battle_map_tilesets.end())
	{
		LogError("No battle_map_tileset matching id \"%s\"", tileset_id.cStr());
		return nullptr;
	}

	auto it = state.battle_map_tilesets.at(tileset_id)->map_part_types.find(id);
	if (it != state.battle_map_tilesets.at(tileset_id)->map_part_types.end())
		return it->second;
	LogError("No battle map part type matching ID \"%s\"", id.cStr());

	return nullptr;
}

template <> const UString &StateObject<BattleMapPartType>::getPrefix()
{
	static UString prefix = "BATTLEMAPPART_";
	return prefix;
}
template <> const UString &StateObject<BattleMapPartType>::getTypeName()
{
	static UString name = "BattleMapPart";
	return name;
}
}
