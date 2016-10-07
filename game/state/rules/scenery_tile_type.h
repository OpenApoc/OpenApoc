#pragma once

#include "game/state/stateobject.h"
#include "library/colour.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{
class Image;
class VoxelMap;
class SceneryTileType : public StateObject<SceneryTileType>
{
  public:
	SceneryTileType() = default;

	enum class TileType
	{
		General,
		Road,
		PeopleTubeJunction,
		PeopleTube,
		CityWall,
	};
	static const std::map<TileType, UString> TileTypeMap;

	enum class RoadType
	{
		StraightBend,
		Junction,
		Terminal,
	};
	static const std::map<RoadType, UString> RoadTypeMap;

	enum class WalkMode
	{
		None,
		Into,
		Onto
	};
	static const std::map<WalkMode, UString> WalkModeMap;

	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<Image> overlaySprite;
	sp<VoxelMap> voxelMap;
	// FIXME: If the damaged tile links form a loop this will leak?
	StateRef<SceneryTileType> damagedTile;
	Vec2<float> imageOffset;
	bool isLandingPad = false;
	Colour minimap_colour;

	TileType tile_type = TileType::General;
	RoadType road_type = RoadType::StraightBend;
	WalkMode walk_mode = WalkMode::None;
	int constitution = 0;
	int value = 0;
	int mass = 0;
	int strength = 0;

	// instead of road_level_change; should be enough for now
	bool isHill = false;
};
}; // namespace OpenApoc
