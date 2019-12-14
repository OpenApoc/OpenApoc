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
	this->centreChanged = true;
	if (z < 0 || static_cast<unsigned>(z) >= this->slices.size())
	{
		return;
	}
	// Slices greater than map size are fine, we will just ignore extra pixels
	if (slice->getSize().x < this->size.x || slice->getSize().y < this->size.y)
	{
		return;
	}
	this->slices[z] = slice;
}

void VoxelMap::calculateCentre()
{
	this->centreChanged = false;

	// This calculates the 'centre' of the voxel map by finding
	// the average position of all 'filled' voxels
	// An 'int' should be more than enough to keep the sum
	// of even the largest completely filled voxel map
	Vec3<int> sum = {0, 0, 0};
	int numFilled = 0;

	for (int z = 0; z < this->size.z; z++)
	{
		// The following would figure out whether a pixel is filled or not
		// However, vanilla always aims at map's centre
		// This provides for small vehicles to "dodge shots", which actually
		// are just misaligned voxelmaps assigned to them
		// Therefore, we should not check this to recreate vanilla behavior
		/*
		for (int y = 0; y < this->size.y; y++)
		{
		    for (int x = 0; x < this->size.x; x++)
		    {

		        if (this->getBit({x, y, z}))
		        {
		            sum += Vec3<int>{x, y, z};
		            numFilled++;
		        }
		    }
		}
		*/
		// Instead, we consider layer filled if one bit is
		if (this->slices[z] && !this->slices[z]->isEmpty())
		{
			sum += Vec3<int>{this->size.x / 2, this->size.y / 2, z};
			numFilled++;
		}
	}
	if (numFilled == 0)
	{
		// Special case 'empty' voxel maps to be the middle of the bounds?
		this->centre = this->size / 2;
	}
	else
	{
		this->centre = sum / numFilled;
	}
}

bool VoxelMap::operator==(const VoxelMap &other) const
{
	if (this->size != other.size)
	{
		return false;
	}
	for (unsigned i = 0; i < this->slices.size(); i++)
	{
		if (this->slices[i] == other.slices[i])
		{
			continue;
		}
		if (!this->slices[i] || !other.slices[i])
		{
			return false;
		}
		if (*this->slices[i] != *other.slices[i])
		{
			return false;
		}
	}
	return true;
}

bool VoxelMap::operator!=(const VoxelMap &other) const { return !(*this == other); }

const Vec3<int> &VoxelMap::getCentre()
{
	if (this->centreChanged)
	{
		this->calculateCentre();
	}
	return this->centre;
}

bool VoxelSlice::operator==(const VoxelSlice &other) const
{
	if (this->size != other.size)
	{
		return false;
	}
	if (this->bits != other.bits)
	{
		return false;
	}
	return true;
}

bool VoxelSlice::operator!=(const VoxelSlice &other) const { return !(*this == other); }

bool VoxelSlice::isEmpty() const
{
	for (const auto &bit : this->bits)
	{
		if (bit)
			return false;
	}
	return true;
}

} // namespace OpenApoc
