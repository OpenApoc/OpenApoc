#pragma once
#include "library/sp.h"

#include <memory>

namespace OpenApoc
{
class VoxelMap;
class SceneryTileDef
{
  private:
	SceneryTileDef() : isLandingPad(false) {}
	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<VoxelMap> voxelMap;
	Vec2<float> imageOffset;
	bool isLandingPad;
	friend class RulesLoader;

  public:
	sp<Image> getSprite() { return this->sprite; }
	sp<Image> getStrategySprite() { return this->strategySprite; }
	sp<VoxelMap> getVoxelMap() { return this->voxelMap; }
	Vec2<float> getImageOffset() { return this->imageOffset; }
	bool getIsLandingPad() const { return this->isLandingPad; }
};
}; // namespace OpenApoc
