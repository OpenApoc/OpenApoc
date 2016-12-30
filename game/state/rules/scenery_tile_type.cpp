#include "game/state/rules/scenery_tile_type.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

const std::map<SceneryTileType::TileType, UString> SceneryTileType::TileTypeMap = {
    {SceneryTileType::TileType::General, "general"},
    {SceneryTileType::TileType::Road, "road"},
    {SceneryTileType::TileType::PeopleTubeJunction, "people_tube_junction"},
    {SceneryTileType::TileType::PeopleTube, "people_tube"},
    {SceneryTileType::TileType::CityWall, "city_wall"}};

const std::map<SceneryTileType::RoadType, UString> SceneryTileType::RoadTypeMap = {
    {SceneryTileType::RoadType::StraightBend, "straight_bend"},
    {SceneryTileType::RoadType::Junction, "junction"},
    {SceneryTileType::RoadType::Terminal, "terminal"}};

const std::map<SceneryTileType::WalkMode, UString> SceneryTileType::WalkModeMap = {
    {SceneryTileType::WalkMode::None, "none"},
    {SceneryTileType::WalkMode::Into, "into"},
    {SceneryTileType::WalkMode::Onto, "onto"}};

sp<SceneryTileType> SceneryTileType::get(const GameState &state, const UString &id)
{
	for (auto &pair : state.cities)
	{
		auto it = pair.second->tile_types.find(id);
		if (it != pair.second->tile_types.end())
			return it->second;
	}
	LogError("No scenery tile type matching ID \"%s\"", id);
	return nullptr;
}

const UString &SceneryTileType::getPrefix()
{
	static UString prefix = "CITYTILE_";
	return prefix;
}
const UString &SceneryTileType::getTypeName()
{
	static UString name = "SceneryTileType";
	return name;
}

} // namespace OpenApoc
