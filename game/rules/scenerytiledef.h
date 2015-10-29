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
	sp<VoxelMap> voxelMap;
	UString damagedTileID;
	Vec2<float> imageOffset;

	bool isLandingPad;
	SceneryTileDef *damagedTile;

	friend class RulesLoader;
	friend class Rules;

  public:
	sp<Image> getSprite() { return this->sprite; }
	sp<Image> getStrategySprite() { return this->strategySprite; }
	sp<VoxelMap> getVoxelMap() { return this->voxelMap; }
	Vec2<float> getImageOffset() { return this->imageOffset; }
	bool getIsLandingPad() const { return this->isLandingPad; }
	SceneryTileDef *getDamagedTile() { return this->damagedTile; }
};
}; // namespace OpenApoc
