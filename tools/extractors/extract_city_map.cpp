#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/city/city.h"
#include "game/state/rules/scenery_tile_type.h"
#include "library/strings_format.h"
#include "library/xorshift.h"
#include "tools/extractors/extractors.h"

#include <map>

// TUBE DEBUG OUTPUT
// BREAKS THE GAME
//#define TUBE_DEBUG_OUTPUT

namespace OpenApoc
{

void InitialGameStateExtractor::extractCityMap(GameState &state, UString fileName,
                                               UString tilePrefix, sp<City> city) const
{
	UString map_prefix = "xcom3/ufodata/";
	unsigned int sizeX = 100;
	unsigned int sizeY = 100;
	unsigned int sizeZ = 10;
	Vec3<unsigned int> fullSize(140, 140, 13);

	// We want a predictable RNG state to generate the 'same' random grass each time
	Xorshift128Plus<uint32_t> rng{};

	auto inFile = fw().data->fs.open(map_prefix + fileName);
	if (!inFile)
	{
		LogError("Failed to open \"%s\"", fileName);
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

// FIXME: FIX BUGGY TUBES
/*
citymap1 at 13,12,1 in dir 3
citymap1 at 43,41,1 in dir 3
citymap1 at 58,53,1 in dir 0
citymap1 at 44,54,1 in dir 3
citymap1 at 57,54,1 in dir 0
citymap1 at 56,55,1 in dir 0
citymap1 at 53,56,1 in dir 0
citymap1 at 28,73,1 in dir 3
citymap1 at 34,73,1 in dir 3
citymap1 at 31,75,1 in dir 3
citymap1 at 30,77,1 in dir 3
citymap1 at 28,80,1 in dir 3
citymap1 at 67,80,1 in dir 3
citymap1 at 31,87,1 in dir 3
citymap1 at 36,49,2 in dir 3
citymap1 at 50,53,2 in dir 3
citymap1 at 29,74,2 in dir 3
citymap1 at 34,77,2 in dir 0
citymap1 at 37,78,2 in dir 0
citymap1 at 68,81,2 in dir 3
citymap1 at 29,89,3 in dir 1
*/

#ifdef TUBE_DEBUG_OUTPUT
	for (unsigned int z = 0; z < sizeZ; z++)
	{
		for (unsigned int y = 0; y < sizeY; y++)
		{
			for (unsigned int x = 0; x < sizeX; x++)
			{
				auto curTileName = city->initial_tiles[Vec3<int>{x + 20, y + 20, z + 1}].id;
				if (tubes.find(curTileName) != tubes.end())
				{
					auto curTileMap = tubes.at(curTileName);
					std::set<std::pair<UString, int>> toCheck;

					// dont check 4ways
					if (curTileMap[0] && curTileMap[1] && curTileMap[2] && curTileMap[3])
					{
						continue;
					}
					// North
					if (curTileMap[0] && y - 1 >= 0)
					{
						toCheck.emplace(
						    city->initial_tiles[Vec3<int>{x + 20, y - 1 + 20, z + 1}].id, 0);
					}
					// East
					if (curTileMap[1] && x + 1 < sizeX)
					{
						toCheck.emplace(
						    city->initial_tiles[Vec3<int>{x + 1 + 20, y + 20, z + 1}].id, 1);
					}
					// South
					if (curTileMap[2] && y + 1 < sizeY)
					{
						toCheck.emplace(
						    city->initial_tiles[Vec3<int>{x + 20, y + 1 + 20, z + 1}].id, 2);
					}
					// West
					if (curTileMap[3] && x - 1 >= 0)
					{
						toCheck.emplace(
						    city->initial_tiles[Vec3<int>{x - 1 + 20, y + 20, z + 1}].id, 3);
					}
					for (auto &p : toCheck)
					{
						if (tubes.find(p.first) != tubes.end())
						{
							auto tarTileMap = tubes.at(p.first);

							int dir = p.second;
							int invDir = -1;
							switch (dir)
							{
								case 0:
									invDir = 2;
									break;
								case 1:
									invDir = 3;
									break;
								case 2:
									invDir = 0;
									break;
								case 3:
									invDir = 1;
									break;
							}

							if (!tarTileMap[invDir])
							{
								LogWarning("BUGGY TUBE DETECTED IN %s at %d,%d,%d in dir %d",
								           fileName, x, y, z, dir);
							}
						}
					}
				}
			}
		}
	}
#endif
}
} // namespace OpenApoc
