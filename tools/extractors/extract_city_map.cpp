#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/city/city.h"
#include "game/state/rules/scenery_tile_type.h"
#include "library/strings_format.h"
#include "library/xorshift.h"
#include "tools/extractors/extractors.h"

#include <map>

namespace OpenApoc
{

void InitialGameStateExtractor::extractCityMap(GameState &state, UString fileName,
                                               UString tilePrefix, sp<City> city)
{
	UString map_prefix = "xcom3/ufodata/";
	unsigned int sizeX = 100;
	unsigned int sizeY = 100;
	unsigned int sizeZ = 10;
	Vec3<unsigned int> fullSize(140, 140, 12);

	// We want a predictable RNG state to generate the 'same' random grass each time
	Xorshift128Plus<uint32_t> rng{};

	auto inFile = fw().data->fs.open(map_prefix + fileName);
	if (!inFile)
	{
		LogError("Failed to open \"%s\"", fileName.cStr());
	}
	auto fileSize = inFile.size();

	unsigned int expectedFileSize = sizeX * sizeY * sizeZ * 2;

	if (fileSize != expectedFileSize)
	{
		LogError("Unexpected filesize %zu - expected %u", fileSize, expectedFileSize);
	}

	city->size = {fullSize.x, fullSize.y, fullSize.z};

	int tileIndex = 0;

	for (unsigned int y = 0; y < fullSize.y; y++)
	{
		for (unsigned int x = 0; x < fullSize.x; x++)
		{
			uint16_t idx;
			if (fileName == "alienmap")
			{
				idx = 5;
			}
			else
			{
				// NOTE: uniform_int_distribution would give a 'better' distribution here, but
				// doesn't give the same result over different platforms (tested: x86_64 gcc 5.4.0
				// and x64 msvc 2015u3)
				// range {169,172}
				auto off = rng() % 4;
				idx = 169 + off;
			}

			auto tileName =
			    format("%s%s%u", SceneryTileType::getPrefix(), tilePrefix, (unsigned)idx);

			city->initial_tiles[Vec3<int>{x, y, 1}] = {&state, tileName};
		}
	}

	for (unsigned int z = 0; z < sizeZ; z++)
	{
		for (unsigned int y = 0; y < sizeY; y++)
		{
			for (unsigned int x = 0; x < sizeX; x++)
			{
				uint16_t idx;
				inFile.read((char *)&idx, sizeof(idx));

				if (idx != 0)
				{
					auto tileName =
					    format("%s%s%u", SceneryTileType::getPrefix(), tilePrefix, (unsigned)idx);

					city->initial_tiles[Vec3<int>{x + 20, y + 20, z + 1}] = {&state, tileName};
				}
				tileIndex++;
			}
		}
	}
}
} // namespace OpenApoc
