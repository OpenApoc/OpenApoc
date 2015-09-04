#pragma once

#include <memory>

namespace OpenApoc
{
class BuildingTileDef
{
  private:
	BuildingTileDef(){};
	std::shared_ptr<Image> sprite;
	std::shared_ptr<VoxelMap> voxelMap;
	friend class RulesLoader;

  public:
	std::shared_ptr<Image> getSprite() { return this->sprite; }
	std::shared_ptr<VoxelMap> getVoxelMap() { return this->voxelMap; }
};
} // namespace OpenApoc
