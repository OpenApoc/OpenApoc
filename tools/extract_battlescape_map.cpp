#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/battle.h"
#include "tools/extractors/extractors.h"

#include <map>

namespace OpenApoc
{
void InitialGameStateExtractor::extractBattlescapeMap(GameState &state, UString dirName,
                                                      UString tilePrefix)
{
	// FIXME: Right now we only know how to read 58UFO8
	dirName = UString("58UFO8");
	tilePrefix = UString("58UFO8_");

	UString map_prefix = "xcom3/MAPS/";
	unsigned int sizeX = 0;
	unsigned int sizeY = 0;
	unsigned int sizeZ = 0;
	unsigned int chunksX = 0;
	unsigned int chunksY = 0;
	unsigned int chunksZ = 0;
	UString fileName;
	unsigned int expectedFileSize;

	// First we read BUILDING.DAT to see the size of a chunk
	{
		fileName = dirName + UString("/BUILDING.DAT");
		expectedFileSize = 1966;

		auto inFile = fw().data->fs.open(map_prefix + fileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
		}
		auto fileSize = inFile.size();

		if (fileSize != expectedFileSize)
		{
			LogError("Unexpected filesize %zu - expected %u", fileSize, expectedFileSize);
		}

		uint32_t idx;
		inFile.read((char *)&idx, sizeof(idx));
		sizeX = (unsigned)idx;
		inFile.read((char *)&idx, sizeof(idx));
		sizeY = (unsigned)idx;
		inFile.read((char *)&idx, sizeof(idx));
		sizeZ = (unsigned)idx;
	}

	// Then we read 58SEC01.SDT to see how many chunks
	{
		fileName = dirName + UString("/58SEC01.SDT");
		expectedFileSize = 20;

		auto inFile = fw().data->fs.open(map_prefix + fileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
		}
		auto fileSize = inFile.size();

		if (fileSize != expectedFileSize)
		{
			LogError("Unexpected filesize %zu - expected %u", fileSize, expectedFileSize);
		}

		uint32_t idx;
		inFile.read((char *)&idx, sizeof(idx));
		chunksX = (unsigned)idx;
		inFile.read((char *)&idx, sizeof(idx));
		chunksY = (unsigned)idx;
		inFile.read((char *)&idx, sizeof(idx));
		chunksZ = (unsigned)idx;
	}

	// Then we read 58SEC01.SMP itself for the map
	{
		fileName = dirName + UString("/58SEC01.SMP");
		expectedFileSize = sizeX * sizeY * sizeZ * chunksX * chunksY * chunksZ * 4;

		auto inFile = fw().data->fs.open(map_prefix + fileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
		}
		auto fileSize = inFile.size();

		if (fileSize != expectedFileSize)
		{
			LogError("Unexpected filesize %zu - expected %u", fileSize, expectedFileSize);
		}

		for (unsigned int cz = 1; cz <= chunksZ; cz++)
		{
			for (unsigned int cy = 1; cy <= chunksY; cy++)
			{
				for (unsigned int cx = 1; cx <= chunksX; cx++)
				{
					for (unsigned int z = 0; z < sizeZ; z++)
					{
						for (unsigned int y = 0; y < sizeY; y++)
						{
							for (unsigned int x = 0; x < sizeX; x++)
							{
								// read ground tile
								{
									uint8_t idx;
									inFile.read((char *)&idx, sizeof(idx));

									if (idx != 0)
									{
										auto tileName =
										    UString::format("%s%s%u", BattleTileType::getPrefix(),
										                    tilePrefix, (unsigned)idx);

										state.battle
										    .initial_tiles[Vec3<int>{x * cx, y * cy, z * cz}] = {
										    &state, tileName};
									}
								}
								// read left wall
								{
									uint8_t idx;
									inFile.read((char *)&idx, sizeof(idx));

									if (idx != 0)
									{
										auto tileName = UString::format(
										    "%s%s%u", BattleLeftWallType::getPrefix(), tilePrefix,
										    (unsigned)idx);

										state.battle.initial_left_walls[Vec3<int>{
										    x * cx, y * cy, z * cz}] = {&state, tileName};
									}
								}
								// read left wall
								{
									uint8_t idx;
									inFile.read((char *)&idx, sizeof(idx));

									if (idx != 0)
									{
										auto tileName = UString::format(
										    "%s%s%u", BattleRightWallType::getPrefix(), tilePrefix,
										    (unsigned)idx);

										state.battle.initial_right_walls[Vec3<int>{
										    x * cx, y * cy, z * cz}] = {&state, tileName};
									}
								}
								// read scenery
								{
									uint8_t idx;
									inFile.read((char *)&idx, sizeof(idx));

									if (idx != 0)
									{
										auto tileName = UString::format(
										    "%s%s%u", BattleSceneryType::getPrefix(), tilePrefix,
										    (unsigned)idx);

										state.battle
										    .initial_scenery[Vec3<int>{x * cx, y * cy, z * cz}] = {
										    &state, tileName};
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
} // namespace OpenApoc
