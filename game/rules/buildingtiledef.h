#pragma once
#include "library/sp.h"

#include <memory>

namespace OpenApoc
{
class BuildingTileDef
{
  private:
	BuildingTileDef() : isLandingPad(false) {}
	sp<Image> sprite;
	sp<Image> strategySprite;
	sp<VoxelMap> voxelMap;
	bool isLandingPad;
	friend class RulesLoader;

  public:
	sp<Image> getSprite() { return this->sprite; }
	sp<Image> getStrategySprite() { return this->strategySprite; }
	sp<VoxelMap> getVoxelMap() { return this->voxelMap; }
	bool getIsLandingPad() const { return this->isLandingPad; }
};
}; // namespace OpenApoc
