#include "library/sp.h"
#include "game/apocresources/loftemps.h"
#include "framework/logger.h"
#include "framework/data.h"

namespace OpenApoc
{

LOFTemps::LOFTemps(IFile &datFile, IFile &tabFile)
{
	if (!tabFile)
	{
		LogError("Invalid TAB file");
		return;
	}

	if (!datFile)
	{
		LogError("Invalid DAT file");
		return;
	}

	uint32_t offset;
	while (tabFile.readule32(offset))
	{
		datFile.seekg(offset * 4, std::ios::beg);
		if (!datFile)
		{
			LogError("Seeking beyond end of file reading offset %u", offset * 4);
			return;
		}

		uint32_t width;
		if (!datFile.readule32(width))
		{
			LogError("Failed to read width");
			return;
		}
		uint32_t height;
		if (!datFile.readule32(height))
		{
			LogError("Failed to read height");
			return;
		}

		if (width % 8)
		{
			LogError("Non-8-bit-aligned width: %u", width);
			return;
		}

		auto slice = std::make_shared<VoxelSlice>(Vec2<int>{width, height});

		for (unsigned int y = 0; y < height; y++)
		{
			// Bitmasks are packed into a 32-bit word, so all strides will
			// be 4-byte aligned
			for (unsigned int x = 0; x < width; x += 32)
			{
				uint32_t bitmask;
				if (!datFile.readule32(bitmask))
				{
					LogError("Failed to read bitmask at {%u,%u}", x, y);
					return;
				}
				for (unsigned int bit = 0; bit < 32; bit++)
				{
					if (x >= width)
						break;
					bool b;
					if (bitmask & 0x80000000)
						b = true;
					else
						b = false;
					bitmask <<= 1;
					slice->setBit(Vec2<int>{x + bit, y}, b);
				}
			}
		}
		LogInfo("Read voxel slice of size {%u,%u}", width, height);
		this->slices.push_back(slice);
	}
}

sp<VoxelSlice> LOFTemps::getSlice(unsigned int idx)
{
	if (idx >= this->slices.size())
	{
		LogError("Requested slice %d - only %u in file", idx, this->slices.size());
		return nullptr;
	}
	return this->slices[idx];
}

}; // namespace OpenApoc
