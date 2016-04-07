#pragma once
#include "game/state/stateobject.h"
#include "library/colour.h"
#include "library/sp.h"
#include "library/vec.h"
#include <memory>

namespace OpenApoc
{
class Image;
class VoxelMap;
class SceneryTileType : public StateObject<SceneryTileType>
{
  public:
	SceneryTileType() : isLandingPad(false) {}
	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<Image> overlaySprite;
	sp<VoxelMap> voxelMap;
	// FIXME: If the damaged tile links form a loop this will leak?
	StateRef<SceneryTileType> damagedTile;
	Vec2<float> imageOffset;
	bool isLandingPad;
	Colour minimap_colour;
};
}; // namespace OpenApoc
