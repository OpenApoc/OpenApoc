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

	enum class RoadType
	{
		StraightBend,
		Junction,
		Terminal,
	};

	enum class WalkMode
	{
		None,
		Into,
		Onto
	};

	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<Image> overlaySprite;
	sp<VoxelMap> voxelMap;
	// FIXME: If the damaged tile links form a loop this will leak?
	StateRef<SceneryTileType> damagedTile;
	Vec2<float> imageOffset = {0, 0};
	bool isLandingPad = false;
	Colour minimap_colour;

	TileType tile_type = TileType::General;
	RoadType road_type = RoadType::StraightBend;
	WalkMode walk_mode = WalkMode::None;
	WalkMode getATVMode() const;

	// Road Connection NESW
	std::vector<bool> connection;
	// Road going up when moving NESW
	std::vector<bool> hill;
	// Tube connection NESWUD
	std::vector<bool> tube;

	int constitution = 0;
	int value = 0;
	int mass = 0;
	int strength = 0;
	// Max value 16
	int height = 0;
	// Max value 16
	int overlayHeight = 0;
	bool basement = false;
	// Does not receive attacks and is not considered hostile action if receives stray shots
	bool commonProperty = false;

	// instead of road_level_change; should be enough for now
	bool isHill = false;
};
}; // namespace OpenApoc
