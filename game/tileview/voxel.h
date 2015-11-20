#pragma once
#include "library/sp.h"

#include "library/vec.h"
#include <vector>
#include <memory>

namespace OpenApoc
{

class TileObject;
class Projectile;

class Collision
{
  public:
	sp<TileObject> obj;
	sp<Projectile> projectile;
	Vec3<float> position;
	explicit operator bool() const { return obj != nullptr; }
};

class VoxelSlice
{
  private:
	Vec2<int> size;
	std::vector<bool> bits;

  public:
	bool getBit(Vec2<int> pos) const;
	void setBit(Vec2<int> pos, bool b);
	const Vec2<int> &getSize() const { return this->size; }

	VoxelSlice(Vec2<int> size);
};

class VoxelMap
{
  private:
	Vec3<int> size;
	std::vector<sp<VoxelSlice>> slices;

  public:
	bool getBit(Vec3<int> pos) const;
	void setSlice(int z, sp<VoxelSlice> slice);

	const Vec3<int> &getSize() const { return this->size; }

	VoxelMap(Vec3<int> size);
};

}; // namespace OpenApoc
