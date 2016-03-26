#pragma once
#include "game/tileview/voxel.h"
#include "library/sp.h"

namespace OpenApoc
{

class IFile;

class LOFTemps
{
  private:
	std::vector<sp<VoxelSlice>> slices;

  public:
	LOFTemps(IFile &datFile, IFile &tabFile);
	sp<VoxelSlice> getSlice(unsigned int idx);
};
}; // namespace OpenApoc
