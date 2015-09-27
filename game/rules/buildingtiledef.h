#pragma once

#include <memory>

namespace OpenApoc
{
class BuildingTileDef
{
  private:
	BuildingTileDef() : isLandingPad(false) {}
	std::shared_ptr<Image> sprite;
	std::shared_ptr<Image> strategySprite;
	std::shared_ptr<VoxelMap> voxelMap;
	bool isLandingPad;
	friend class RulesLoader;

  public:
	std::shared_ptr<Image> getSprite() { return this->sprite; }
	std::shared_ptr<Image> getStrategySprite() { return this->strategySprite; }
	std::shared_ptr<VoxelMap> getVoxelMap() { return this->voxelMap; }
	bool getIsLandingPad() const { return this->isLandingPad; }
};
}; // namespace OpenApoc
