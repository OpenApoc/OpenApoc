#pragma once
#include "library/sp.h"

#include <memory>

namespace OpenApoc
{
class VoxelMap;
class SceneryTileDef
{
  private:
	SceneryTileDef() : isLandingPad(false), damagedTile(nullptr) {}
	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<Image> overlaySprite;
	sp<VoxelMap> voxelMap;
	UString damagedTileID;
	Vec2<float> imageOffset;

	bool isLandingPad;
	SceneryTileDef *damagedTile;

	friend class RulesLoader;
	friend class Rules;

  public:
	sp<Image> getSprite() const { return this->sprite; }
	sp<Image> getStrategySprite() const { return this->strategySprite; }
	sp<Image> getOverlaySprite() const { return this->overlaySprite; }
	sp<VoxelMap> getVoxelMap() const { return this->voxelMap; }
	Vec2<float> getImageOffset() const { return this->imageOffset; }
	bool getIsLandingPad() const { return this->isLandingPad; }
	SceneryTileDef *getDamagedTile() const { return this->damagedTile; }
};
}; // namespace OpenApoc
