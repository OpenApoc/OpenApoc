#pragma once
#include "framework/fs.h"
#include "library/sp.h"
#include "library/voxel.h"
#include <vector>

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
