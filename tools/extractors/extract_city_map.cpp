#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/rules/scenery_tile_type.h"
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
	Vec3<int> fullSize(140, 140, 11);

	auto inFile = fw().data->fs.open(map_prefix + fileName);
	if (!inFile)
	{
		LogError("Failed to open \"%s\"", fileName.c_str());
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
				idx = std::uniform_int_distribution<int>{169, 172}(state.rng);
			}

			auto tileName = UString::format("%s%s%u", SceneryTileType::getPrefix().c_str(),
			                                tilePrefix.c_str(), (unsigned)idx);

			city->initial_tiles[Vec3<int>{x, y, 0}] = {&state, tileName};
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
					auto tileName = UString::format("%s%s%u", SceneryTileType::getPrefix().c_str(),
					                                tilePrefix.c_str(), (unsigned)idx);

					city->initial_tiles[Vec3<int>{x + 20, y + 20, z}] = {&state, tileName};
				}
				tileIndex++;
			}
		}
	}
}
} // namespace OpenApoc
