#pragma once
#include "game/tileview/voxel.h"

namespace OpenApoc
{

class IFile;

class LOFTemps
{
  private:
	std::vector<std::shared_ptr<VoxelSlice>> slices;

  public:
	LOFTemps(IFile &datFile, IFile &tabFile);
	std::shared_ptr<VoxelSlice> getSlice(unsigned int idx);
};
}; // namespace OpenApoc
