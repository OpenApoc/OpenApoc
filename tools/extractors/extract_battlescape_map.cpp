#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/battle.h"
#include "game/state/battlemap.h"
#include "tools/extractors/common/battlemap.h"
#include "tools/extractors/extractors.h"

#include <map>

namespace OpenApoc
{

void InitialGameStateExtractor::extractBattlescapeMapFromPath(GameState &state,
                                                              const UString dirName,
                                                              const int index)
{
	UString tilePrefix = UString::format("%s_", dirName);
	UString map_prefix = "xcom3/maps/";
	UString mapunits_suffix = "/mapunits/";

	BuildingDatStructure bdata;
	{
		auto fileName = dirName + UString("/building.dat");

		auto datFileName = map_prefix + fileName;
		auto inFile = fw().data->fs.open(datFileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
			return;
		}

		inFile.read((char *)&bdata, sizeof(bdata));
		if (!inFile)
		{
			LogError("Failed to read entry in \"%s\"", fileName.cStr());
			return;
		}
	}

	RubbleDatStructure rdata;
	{
		auto fileName = dirName + mapunits_suffix + UString("rubble.dat");
		auto fullPath = map_prefix + fileName;
		auto inFile = fw().data->fs.open(fullPath);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
			return;
		}

		inFile.read((char *)&rdata, sizeof(rdata));
		if (!inFile)
		{
			LogError("Failed to read entry in \"%s\"", fileName.cStr());
			return;
		}
	}

	auto m = mksp<BattleMap>();

	UString id = UString::format("%s%s", BattleMap::getPrefix(), this->battleMapPaths[index]);

	m->id = id;
	m->chunk_size = {bdata.chunk_x, bdata.chunk_y, bdata.chunk_z};
	m->max_battle_size = {bdata.battle_x, bdata.battle_y, bdata.battle_z};

	uint8_t north_flag = 0b0001;
	uint8_t east_flag = 0b0010;
	uint8_t south_flag = 0b0100;
	uint8_t west_flag = 0b1000;

	m->allow_entrance[Battle::MapBorder::North] = bdata.allow_entrance_from & north_flag;
	m->allow_entrance[Battle::MapBorder::East] = bdata.allow_entrance_from & east_flag;
	m->allow_entrance[Battle::MapBorder::South] = bdata.allow_entrance_from & south_flag;
	m->allow_entrance[Battle::MapBorder::West] = bdata.allow_entrance_from & west_flag;
	m->allow_exit[Battle::MapBorder::North] = bdata.allow_exit_from & north_flag;
	m->allow_exit[Battle::MapBorder::East] = bdata.allow_exit_from & east_flag;
	m->allow_exit[Battle::MapBorder::South] = bdata.allow_exit_from & south_flag;
	m->allow_exit[Battle::MapBorder::West] = bdata.allow_exit_from & west_flag;
	m->entrance_level_min = bdata.entrance_min_level;
	m->entrance_level_max = bdata.entrance_max_level;
	m->exit_level_min = bdata.exit_min_level;
	m->exit_level_max = bdata.exit_max_level;
	m->tilesets.emplace_back(tilePrefix.substr(0, tilePrefix.length() - 1));

	// Side 0 = exits by X axis, Side 1 = exits by Y axis
	for (int s = 0; s < 2; s++)
	{
		for (int l = 0; l < 15; l++)
		{
			for (int e = 0; e < 14; e++)
			{
				if (bdata.exits[s][l].exits[e] != 0xffffffff)
				{
					m->exits.emplace_back(s == 0 ? bdata.exits[s][l].exits[e] : 0,
					                      s == 1 ? bdata.exits[s][l].exits[e] : 0, l);
				}
			}
		}
	}

	if (bdata.destroyed_ground_idx != 0)
		m->destroyed_ground_tile = {
		    &state, UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "GD_",
		                            (unsigned)bdata.destroyed_ground_idx)};

	for (int i = 0; i < 5; i++)
	{
		if (rdata.left_wall[i] != 0)
		{
			m->rubble_left_wall.emplace_back(
			    &state, UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix,
			                            "LW_", (unsigned)rdata.left_wall[i]));
		}
		if (rdata.right_wall[i] != 0)
		{
			m->rubble_right_wall.emplace_back(
			    &state, UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix,
			                            "RW_", (unsigned)rdata.right_wall[i]));
		}
		if (rdata.scenery[i] != 0)
		{
			m->rubble_scenery.emplace_back(
			    &state, UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix,
			                            "SC_", (unsigned)rdata.scenery[i]));
		}
	}

	// Trying all possible names, because game actually has some maps missing sectors in the middle
	// (like, 05RESCUE has no SEC04 but has SEC05 and on)
	for (int sector = 1; sector < 100; sector++)
	{
		UString secName = UString::format("%02d", sector);

		UString tilesName = UString::format("%s_%02d", dirName, sector);

		SecSdtStructure sdata;
		{
			auto fileName = dirName + UString("/") + dirName.substr(0, 2) + UString("sec") +
			                secName + UString(".sdt");

			auto fullPath = map_prefix + fileName;
			auto inFile = fw().data->fs.open(fullPath);
			if (!inFile)
			{
				LogInfo("Sector %d not present for map %d", sector, index);
				continue;
			}

			inFile.read((char *)&sdata, sizeof(sdata));
			if (!inFile)
			{
				LogError("Failed to read entry in \"%s\"", fileName.cStr());
				return;
			}
		}

		auto s = mksp<BattleMapSector>();

		s->size = {sdata.chunks_x, sdata.chunks_y, sdata.chunks_z};
		s->occurrence_min = sdata.occurrence_min;
		s->occurrence_max = sdata.occurrence_max;
		s->sectorTilesName = tilesName;

		m->sectors["SEC" + secName] = s;
	}

	state.battle_maps[id] = m;
}

std::map<UString, up<BattleMapSectorTiles>>
InitialGameStateExtractor::extractMapSectors(GameState &state, const UString &mapRootName)
{
	std::map<UString, up<BattleMapSectorTiles>> sectors;
	UString map_prefix = "xcom3/maps/";
	UString dirName = mapRootName;
	UString tilePrefix = UString::format("%s_", dirName);
	BuildingDatStructure bdata;
	{
		auto fileName = dirName + UString("/building.dat");

		auto datFileName = map_prefix + fileName;
		auto inFile = fw().data->fs.open(datFileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
			return {};
		}

		inFile.read((char *)&bdata, sizeof(bdata));
		if (!inFile)
		{
			LogError("Failed to read entry in \"%s\"", fileName.cStr());
			return {};
		}
	}
	// Trying all possible names, because game actually has some maps missing sectors in the middle
	// (like, 05RESCUE has no SEC04 but has SEC05 and on)
	for (int sector = 1; sector < 100; sector++)
	{
		UString secName = UString::format("%02d", sector);
		UString tilesName = UString::format("%s_%02d", dirName, sector);
		up<BattleMapSectorTiles> tiles(new BattleMapSectorTiles());

		SecSdtStructure sdata;
		{
			auto fileName = dirName + UString("/") + dirName.substr(0, 2) + UString("sec") +
			                secName + UString(".sdt");

			auto fullPath = map_prefix + fileName;
			auto inFile = fw().data->fs.open(fullPath);
			if (!inFile)
			{
				LogInfo("Sector %d not present for map %s", sector, mapRootName.cStr());
				continue;
			}

			inFile.read((char *)&sdata, sizeof(sdata));
			if (!inFile)
			{
				LogError("Failed to read entry in \"%s\"", fileName.cStr());
				return {};
			}
		}

		// Read LOS blocks
		{
			auto fileName = dirName + UString("/") + dirName.substr(0, 2) + UString("sec") +
			                secName + UString(".sls");

			auto fullPath = map_prefix + fileName;
			auto inFile = fw().data->fs.open(fullPath);
			if (!inFile)
			{
				LogError("Failed to open \"%s\"", fileName.cStr());
				return {};
			}

			auto fileSize = inFile.size();
			auto objectCount = fileSize / sizeof(struct LineOfSightData);

			for (unsigned i = 0; i < objectCount; i++)
			{
				LineOfSightData ldata;

				inFile.read((char *)&ldata, sizeof(ldata));
				if (!inFile)
				{
					LogError("Failed to read entry %d in \"%s\"", i, fileName.cStr());
					return {};
				}

				auto los_block = mksp<BattleMapSector::LineOfSightBlock>();

				los_block->start = {ldata.begin_x, ldata.begin_y, ldata.begin_z};
				los_block->end = {ldata.end_x, ldata.end_y, ldata.end_z};
				los_block->ai_patrol_priority = ldata.ai_patrol_priority;
				los_block->ai_target_priority = ldata.ai_target_priority;

				los_block->spawn_priority = ldata.spawn_priority;
				// It only matters if spawn priority is >0, so don't bother bloating xml with unused
				// values
				if (los_block->spawn_priority > 0)
				{
					los_block->spawn_large_units = ldata.spawn_large == 1;
					los_block->spawn_walking_units = ldata.spawn_walkers == 1;
					switch (ldata.spawn_type)
					{
						case SPAWN_TYPE_PLAYER:
							los_block->spawn_type =
							    BattleMapSector::LineOfSightBlock::SpawnType::Player;
							break;
						case SPAWN_TYPE_ENEMY:
							los_block->spawn_type =
							    BattleMapSector::LineOfSightBlock::SpawnType::Enemy;
							break;
						case SPAWN_TYPE_CIVILIAN:
							los_block->spawn_type =
							    BattleMapSector::LineOfSightBlock::SpawnType::Civilian;
							break;
						// TacEdit (map editor from vanilla creators) allows values up to 8, but
						// they
						// are unused, so we should accomodate for that
						default:
							// Disable spawning and leave the type to its default value
							los_block->spawn_priority = 0;
							break;
					}
				}

				tiles->los_blocks.push_back(los_block);
			}
		}

		// Read Loot locations
		{
			auto fileName = dirName + UString("/") + dirName.substr(0, 2) + UString("sec") +
			                secName + UString(".sob");

			auto fullPath = map_prefix + fileName;
			auto inFile = fw().data->fs.open(fullPath);
			if (inFile)
			{
				auto fileSize = inFile.size();
				auto objectCount = fileSize / sizeof(struct LootLocationData);

				for (unsigned i = 0; i < objectCount; i++)
				{
					LootLocationData ldata;

					inFile.read((char *)&ldata, sizeof(ldata));
					if (!inFile)
					{
						LogError("Failed to read entry %d in \"%s\"", i, fileName.cStr());
						return {};
					}

					if (ldata.priority == 0)
						continue;

					Organisation::LootPriority lp;
					switch (ldata.priority)
					{
						case 1:
							lp = Organisation::LootPriority::A;
							break;
						case 2:
							lp = Organisation::LootPriority::B;
							break;
						case 3:
							lp = Organisation::LootPriority::C;
							break;
						default:
							LogError("Encountered invalid loot priority in %d for sector %d", i,
							         sector);
							return {};
					}
					tiles->loot_locations[{ldata.x, ldata.y, ldata.z}] = lp;
				}
			}
		}

		// Read sector map
		{
			auto fileName = dirName + UString("/") + dirName.substr(0, 2) + UString("sec") +
			                secName + UString(".smp");

			auto expectedFileSize = bdata.chunk_x * bdata.chunk_y * bdata.chunk_z * sdata.chunks_x *
			                        sdata.chunks_y * sdata.chunks_z * 4;
			auto fullPath = map_prefix + fileName;
			auto inFile = fw().data->fs.open(fullPath);
			if (!inFile)
			{
				LogError("Failed to open \"%s\"", fileName.cStr());
			}
			auto fileSize = inFile.size();

			if (fileSize != expectedFileSize)
			{
				LogError("Unexpected filesize %zu - expected %u", fileSize, expectedFileSize);
			}

			for (unsigned int z = 0; z < bdata.chunk_z * sdata.chunks_z; z++)
			{
				for (unsigned int y = 0; y < bdata.chunk_y * sdata.chunks_y; y++)
				{
					for (unsigned int x = 0; x < bdata.chunk_x * sdata.chunks_x; x++)
					{
						SmpData tdata;

						inFile.read((char *)&tdata, sizeof(tdata));
						if (!inFile)
						{
							LogError("Failed to read entry %d,%d,%d in \"%s\"", x, y, z,
							         fileName.cStr());
							return {};
						}
						// read ground
						if (tdata.GD != 0)
						{
							auto tileName =
							    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(),
							                    tilePrefix, "GD_", (unsigned)tdata.GD);

							tiles->initial_grounds[Vec3<int>{x, y, z}] = {&state, tileName};
						}
						// read left wall
						if (tdata.LW != 0)
						{
							auto tileName =
							    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(),
							                    tilePrefix, "LW_", (unsigned)tdata.LW);

							tiles->initial_left_walls[Vec3<int>{x, y, z}] = {&state, tileName};
						}
						// read right wall
						if (tdata.RW != 0)
						{
							auto tileName =
							    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(),
							                    tilePrefix, "RW_", (unsigned)tdata.RW);

							tiles->initial_right_walls[Vec3<int>{x, y, z}] = {&state, tileName};
						}
						// read scenery
						if (tdata.SC != 0)
						{
							auto tileName =
							    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(),
							                    tilePrefix, "SC_", (unsigned)tdata.SC);

							tiles->initial_scenery[Vec3<int>{x, y, z}] = {&state, tileName};
						}
					}
				}
			}
		}
		sectors[tilesName] = std::move(tiles);
	}

	return sectors;
}

void InitialGameStateExtractor::extractBattlescapeMap(GameState &state,
                                                      const std::vector<OpenApoc::UString> &paths)
{
	for (unsigned i = 0; i < paths.size(); i++)
	{
		if (paths[i].length() > 0)
		{
			extractBattlescapeMapFromPath(state, paths[i], i);
		}
	}
}
} // namespace OpenApoc
