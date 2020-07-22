#include "framework/configfile.h"
#include "framework/logger.h"
#include "library/rect.h"
#include "library/voxel.h"

// Vanilla did not use voxel map centres properly, just used
// voxelmap centre without checking bits
// Therefore, we should not do it
//#define CHECK_VOXELMAP_CENTRE

using namespace OpenApoc;

static void check_voxel(Vec3<int> position, VoxelMap &v, bool expected)
{
	if (v.getBit(position) != expected)
	{
		LogError("Unexpected voxel at %s - expected %d", position, expected ? 1 : 0);
		exit(EXIT_FAILURE);
	}
}

static void check_slice(Vec2<int> position, VoxelSlice &s, bool expected)
{
	if (s.getBit(position) != expected)
	{
		LogError("Unexpected voxel at %s - expected %d", position, expected ? 1 : 0);
		exit(EXIT_FAILURE);
	}
}

static void test_voxel(Vec3<int> voxel_size)
{
	VoxelMap v{voxel_size};
	if (v.size != voxel_size)
	{
		LogError("Unexpected size %s", v.size);
		exit(EXIT_FAILURE);
	}
	// Ensure everything is '0' at init, and anything outside the bounds should be '0' too
	for (int z = -16; z < voxel_size.z + 32; z++)
	{
		for (int y = -64; y < voxel_size.y + 64; y++)
		{
			for (int x = -1; x < voxel_size.x + 99; x++)
			{
				check_voxel({x, y, z}, v, false);
			}
		}
	}

	// An empty map should have a center in the 'middle'
	v.calculateCentre();
	if (v.getCentre() != v.size / 2)
	{
		LogError("Unexpected centre %s for empty map", v.getCentre());
		exit(EXIT_FAILURE);
	}

	// Add a slice with a set voxel:
	auto slice = mksp<VoxelSlice>(Vec2<int>{voxel_size.x, voxel_size.y});

	if (slice->size != Vec2<int>{voxel_size.x, voxel_size.y})
	{
		LogError("Unexpected slice size %s", slice->size);
		exit(EXIT_FAILURE);
	}
	// Ensure everything is '0' at init, and anything outside the bounds should be '0' too
	for (int y = -64; y < voxel_size.y + 64; y++)
	{
		for (int x = -1; x < voxel_size.z + 99; x++)
		{
			check_slice({x, y}, *slice, false);
		}
	}

	// Set one bit to true and check that
	Vec2<int> bit_position = {2, 6};
	if (bit_position.x > voxel_size.x)
	{
		bit_position.x = voxel_size.x - 1;
		LogInfo("Clamping bit position x to %d", bit_position.x);
	}
	if (bit_position.y >= voxel_size.y)
	{
		bit_position.y = voxel_size.y - 1;
		LogInfo("Clamping bit position y to %d", bit_position.y);
	}

	slice->setBit(bit_position, true);
	for (int y = -64; y < voxel_size.y + 64; y++)
	{
		for (int x = -1; x < voxel_size.x + 99; x++)
		{
			if (x == bit_position.x && y == bit_position.y)
				check_slice({x, y}, *slice, true);
			else
				check_slice({x, y}, *slice, false);
		}
	}

	// Put that in the map and check that....
	Vec3<int> bit_voxel_position = {bit_position.x, bit_position.y, 14};
	if (bit_voxel_position.z >= voxel_size.z)
	{
		bit_voxel_position.z = voxel_size.z - 1;
		LogInfo("Clamping bit position z to %d", bit_voxel_position.z);
	}
	v.setSlice(bit_voxel_position.z, slice);
	for (int z = -16; z < voxel_size.z + 33; z++)
	{
		for (int y = -64; y < voxel_size.y + 66; y++)
		{
			for (int x = -1; x < voxel_size.x + 1; x++)
			{
				if (x == bit_voxel_position.x && y == bit_voxel_position.y &&
				    z == bit_voxel_position.z)
					check_voxel({x, y, z}, v, true);
				else
					check_voxel({x, y, z}, v, false);
			}
		}
	}

// The centre of a voxelmap with a single bit should be the same as that bit position
#ifdef CHECK_VOXELMAP_CENTRE
	v.calculateCentre();
	if (v.getCentre() != bit_voxel_position)
	{
		LogError("Unexpected centre %s for single-bit map, expected %s", v.getCentre(),
		         bit_voxel_position);
		exit(EXIT_FAILURE);
	}
#endif

	// Unset the bit and make sure it's empty again
	slice->setBit(bit_position, false);
	for (int z = -16; z < voxel_size.z + 32; z++)
	{
		for (int y = -64; y < voxel_size.y + 64; y++)
		{
			for (int x = -1; x < voxel_size.x + 99; x++)
			{
				check_voxel({x, y, z}, v, false);
			}
		}
	}

#ifdef CHECK_VOXELMAP_CENTRE
	v.calculateCentre();
	if (v.getCentre() != v.size / 2)
	{
		LogError("Unexpected centre %s for reset-to-empty map", v.getCentre());
		exit(EXIT_FAILURE);
	}
#endif
	// and set the bit again to get back to single-bit-set state
	slice->setBit(bit_position, true);

	// Add a second bit at (first_bit_pos - 2), the centre should then be at (first_bit_pos +
	// second_bit_pos) / 2

	auto bit_2_voxel_position = bit_voxel_position - Vec3<int>{2, 2, 2};

	if (bit_2_voxel_position.x < 0)
	{
		bit_2_voxel_position.x = 0;
		LogInfo("Clamping bit 2 position x to %d", bit_2_voxel_position.x);
	}
	if (bit_2_voxel_position.y < 0)
	{
		bit_2_voxel_position.y = 0;
		LogInfo("Clamping bit 2 position y to %d", bit_2_voxel_position.y);
	}
	if (bit_2_voxel_position.z < 0)
	{
		bit_2_voxel_position.z = 0;
		LogInfo("Clamping bit 2 position z to %d", bit_2_voxel_position.z);
	}
	auto slice2 = mksp<VoxelSlice>(Vec2<int>{voxel_size.x, voxel_size.y});
	if (bit_2_voxel_position.z == bit_voxel_position.z)
	{
		LogInfo("Slice of bit 2 same as bit 1");
		slice2 = slice;
	}

	slice2->setBit({bit_2_voxel_position.x, bit_2_voxel_position.y}, true);

	v.setSlice(bit_2_voxel_position.z, slice2);
	for (int z = -16; z < voxel_size.z + 33; z++)
	{
		for (int y = -64; y < voxel_size.y + 66; y++)
		{
			for (int x = -1; x < voxel_size.x + 1; x++)
			{
				Vec3<int> pos = {x, y, z};
				if (pos == bit_voxel_position || pos == bit_2_voxel_position)
					check_voxel({x, y, z}, v, true);
				else
					check_voxel({x, y, z}, v, false);
			}
		}
	}

// Now check the centre
#ifdef CHECK_VOXELMAP_CENTRE
	auto expected_centre = (bit_voxel_position + bit_2_voxel_position) / 2;
	v.calculateCentre();
	if (v.getCentre() != expected_centre)
	{
		LogError("Unexpected centre %s for 2 bit map, expected %s", v.getCentre(), expected_centre);
		exit(EXIT_FAILURE);
	}
#endif
	// Everything looks good
	return;
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	const std::vector<Vec3<int>> voxel_sizes = {
	    {1, 1, 1},
	    {32, 32, 16},
	    {33, 32, 16},
	    {77, 75, 2222},
	};
	for (auto &size : voxel_sizes)
	{
		LogInfo("Testing voxel size %s", size);
		test_voxel(size);
	}
}
