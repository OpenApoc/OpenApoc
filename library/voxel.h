#pragma once
#include "library/resource.h"
#include "library/sp.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class VoxelSlice : public ResObject
{
  public:
	Vec2<int> size;
	std::vector<bool> bits;

	bool getBit(Vec2<int> pos) const;
	void setBit(Vec2<int> pos, bool b);
	const Vec2<int> &getSize() const { return this->size; }

	VoxelSlice(Vec2<int> size);
	VoxelSlice() = default;
};

class VoxelMap
{
  public:
	Vec3<int> size;
	Vec3<int> centre;
	std::vector<sp<VoxelSlice>> slices;

	bool getBit(Vec3<int> pos) const;
	void setSlice(int z, sp<VoxelSlice> slice);
	void calculateCentre();

	const Vec3<int> &getSize() const { return this->size; }

	VoxelMap(Vec3<int> size);
	VoxelMap() = default;
};
} // namespace OpenApoc
