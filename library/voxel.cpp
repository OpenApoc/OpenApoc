#include "library/voxel.h"

namespace OpenApoc
{

VoxelSlice::VoxelSlice(Vec2<int> size) : size(size), bits(size.x * size.y) {}

bool VoxelSlice::getBit(Vec2<int> pos) const
{
	if (pos.x < 0 || pos.x >= this->size.x || pos.y < 0 || pos.y >= this->size.y)
	{
		return false;
	}

	return this->bits[pos.y * this->size.x + pos.x];
}

void VoxelSlice::setBit(Vec2<int> pos, bool b)
{
	if (pos.x < 0 || pos.x >= this->size.x || pos.y < 0 || pos.y >= this->size.y)
	{
		return;
	}
	this->bits[pos.y * this->size.x + pos.x] = b;
}

VoxelMap::VoxelMap(Vec3<int> size) : size(size) { slices.resize(size.z); }

bool VoxelMap::getBit(Vec3<int> pos) const
{
	if (pos.x < 0 || pos.x >= this->size.x || pos.y < 0 || pos.y >= this->size.y || pos.z < 0 ||
	    pos.z >= this->size.z)
	{
		return false;
	}

	if (slices.size() <= static_cast<unsigned>(pos.z))
		return false;
	if (!slices[pos.z])
		return false;

	return slices[pos.z]->getBit({pos.x, pos.y});
}

void VoxelMap::setSlice(int z, sp<VoxelSlice> slice)
{
	if (z < 0 || static_cast<unsigned>(z) >= this->slices.size())
	{
		return;
	}
	if (slice->getSize() != Vec2<int>{this->size.x, this->size.y})
	{
		return;
	}
	this->slices[z] = slice;
}

} // namesapce OpenApoc
