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

	bool isEmpty() const;

	bool operator==(const VoxelSlice &other) const;
	bool operator!=(const VoxelSlice &other) const;

	VoxelSlice(Vec2<int> size);
	VoxelSlice() = default;
};

class VoxelMap
{
  private:
	Vec3<int> centre = {0, 0, 0};
	bool centreChanged = true;

  public:
	Vec3<int> size = {0, 0, 0};
	std::vector<sp<VoxelSlice>> slices;

	const Vec3<int> &getCentre();

	bool getBit(Vec3<int> pos) const;
	void setSlice(int z, sp<VoxelSlice> slice);
	void calculateCentre();

	const Vec3<int> &getSize() const { return this->size; }

	bool operator==(const VoxelMap &other) const;
	bool operator!=(const VoxelMap &other) const;

	VoxelMap(Vec3<int> size);
	VoxelMap() = default;
};
} // namespace OpenApoc
