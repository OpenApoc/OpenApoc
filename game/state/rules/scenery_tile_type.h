#pragma once
#include "game/state/stateobject.h"
#include "library/colour.h"
#include "library/sp.h"
#include "library/vec.h"
#include <map>
#include <memory>

namespace OpenApoc
{
class Image;
class VoxelMap;
class SceneryTileType : public StateObject<SceneryTileType>
{
  public:
	SceneryTileType();

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
	bool isLandingPad;
	Colour minimap_colour;

	TileType tile_type;
	RoadType road_type;
	WalkMode walk_mode;
	int constitution;
	int value;
	int mass;
	int strength;

	// instead of road_level_change; should be enough for now
	bool isHill;
};
}; // namespace OpenApoc
