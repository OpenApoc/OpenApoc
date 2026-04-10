#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

template <>
sp<SceneryTileType> StateObject<SceneryTileType>::get(const GameState &state, const UString &id)
{
	auto it = state.scenery_tile_types.find(id);
	if (it == state.scenery_tile_types.end())
	{
		LogError("No scenery tile type matching ID \"{0}\"", id);
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<SceneryTileType>::getPrefix()
{
	static UString prefix = "CITYTILE_";
	return prefix;
}
template <> const UString &StateObject<SceneryTileType>::getTypeName()
{
	static UString name = "SceneryTileType";
	return name;
}

SceneryTileType::WalkMode SceneryTileType::getATVMode() const
{
	if (walk_mode == WalkMode::None && tile_type == TileType::Road)
	{
		return WalkMode::Into;
	}
	return walk_mode;
}

} // namespace OpenApoc
