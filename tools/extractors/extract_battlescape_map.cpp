#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/battle/battle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemap.h"
#include "library/strings_format.h"
#include "tools/extractors/common/battlemap.h"
#include "tools/extractors/extractors.h"
#include <unordered_map>

#include <map>

namespace OpenApoc
{

void InitialGameStateExtractor::extractBattlescapeMapFromPath(GameState &state,
                                                              const UString dirName,
                                                              const int index) const
{
	UString tilePrefix = format("%s_", dirName);
	UString map_prefix = "xcom3/maps/";
	UString mapunits_suffix = "/mapunits/";
	bool baseMap = dirName == "37base";

	BuildingDatStructure bdata;
	{
		auto fileName = dirName + UString("/building.dat");

		auto datFileName = map_prefix + fileName;
		auto inFile = fw().data->fs.open(datFileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName);
			return;
		}

		inFile.read((char *)&bdata, sizeof(bdata));
		if (!inFile)
		{
			LogError("Failed to read entry in \"%s\"", fileName);
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
			LogError("Failed to open \"%s\"", fileName);
			return;
		}

		inFile.read((char *)&rdata, sizeof(rdata));
		if (!inFile)
		{
			LogError("Failed to read entry in \"%s\"", fileName);
			return;
		}
	}

	int firstExitIdx = 0;
	{
		auto fileName = dirName + mapunits_suffix + UString("grounmap.dat");

		auto fullPath = map_prefix + fileName;
		auto inFile = fw().data->fs.open(fullPath);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName);
			return;
		}

		auto fileSize = inFile.size();
		auto objectCount = fileSize / sizeof(struct BattleMapPartEntry);
		firstExitIdx = objectCount - 4;
	}

	auto m = mksp<BattleMap>();

	UString id = format("%s%s", BattleMap::getPrefix(), this->battleMapPaths[index]);

	m->id = id;
	m->chunk_size = {bdata.chunk_x, bdata.chunk_y, bdata.chunk_z};
	m->max_battle_size = {bdata.battle_x, bdata.battle_y, bdata.battle_z};

	uint8_t north_flag = 0b0001;
	uint8_t east_flag = 0b0010;
	uint8_t south_flag = 0b0100;
	uint8_t west_flag = 0b1000;

	m->allow_entrance[MapDirection::North] = bdata.allow_entrance_from & north_flag;
	m->allow_entrance[MapDirection::East] = bdata.allow_entrance_from & east_flag;
	m->allow_entrance[MapDirection::South] = bdata.allow_entrance_from & south_flag;
	m->allow_entrance[MapDirection::West] = bdata.allow_entrance_from & west_flag;
	m->allow_exit[MapDirection::North] = bdata.allow_exit_from & north_flag;
	m->allow_exit[MapDirection::East] = bdata.allow_exit_from & east_flag;
	m->allow_exit[MapDirection::South] = bdata.allow_exit_from & south_flag;
	m->allow_exit[MapDirection::West] = bdata.allow_exit_from & west_flag;
	m->entrance_level_min = bdata.entrance_min_level;
	m->entrance_level_max = bdata.entrance_max_level;
	m->exit_level_min = bdata.exit_min_level;
	m->exit_level_max = bdata.exit_max_level;
	m->tilesets.emplace_back(tilePrefix.substr(0, tilePrefix.length() - 1));

	// Side 0 = exits by X axis, Side 1 = exits by Y axis
	for (int l = 0; l < 15; l++)
	{
		for (int e = 0; e < 14; e++)
		{
			/* As the exits array isn't uint32_t aligned, pull it out rather than passing a
			 * reference directly to the Vec3<> constructor
			 * Copying it out to the stack here allows the compiler to do whatever lowering is
			 * necessary to realign the reads*/
			uint32_t val = bdata.exits[0][l].exits[e];
			if (val != 0xffffffff)
				m->exitsX.emplace(val, 0, l);
			val = bdata.exits[1][l].exits[e];
			if (val != 0xffffffff)
				m->exitsY.emplace(0, val, l);
		}
	}

	if (bdata.destroyed_ground_idx != 0)
		m->destroyed_ground_tile = {&state,
		                            format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix,
		                                   "GD_", (unsigned)bdata.destroyed_ground_idx)};

	for (int i = 0; i < 5; i++)
	{
		if (rdata.left_wall[i] != 0)
		{
			m->rubble_left_wall.emplace_back(
			    &state, format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "LW_",
			                   (unsigned)rdata.left_wall[i]));
		}
		if (rdata.right_wall[i] != 0)
		{
			m->rubble_right_wall.emplace_back(
			    &state, format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "RW_",
			                   (unsigned)rdata.right_wall[i]));
		}
		if (rdata.feature[i] != 0)
		{
			m->rubble_feature.emplace_back(&state,
			                               format("%s%s%s%u", BattleMapPartType::getPrefix(),
			                                      tilePrefix, "FT_", (unsigned)rdata.feature[i]));
		}
	}

	for (int i = 0; i < 4; i++)
	{
		m->exit_grounds.emplace_back(&state, format("%s%s%s%u", BattleMapPartType::getPrefix(),
		                                            tilePrefix, "GD_", (unsigned)firstExitIdx + i));
	}

	if (reinforcementTimers.find(dirName) != reinforcementTimers.end())
	{
		m->reinforcementsInterval = reinforcementTimers.at(dirName);
	}

	// Trying all possible names, because game actually has some maps missing sectors in the middle
	// (like, 05RESCUE has no SEC04 but has SEC05 and on)
	auto sdtFiles = fw().data->fs.enumerateDirectory(map_prefix + "/" + dirName, ".sdt");
	int fileCounter = -1;
	for (const auto &sdtFile : sdtFiles)
	{
		fileCounter++;
		LogInfo("Reading map %s", sdtFile);
		/*  Trim off '.sdt' to get the base map name */
		LogAssert(sdtFile.length() >= 4);
		auto secName = sdtFile.substr(0, sdtFile.length() - 4);

		auto sector = secName.substr(secName.length() - 2);

		int groundCounter = 0;
		do
		{
			UString tilesName;
			UString secID;
			if (baseMap)
			{
				if (fileCounter == 0)
				{
					tilesName = format("%s_%02d", dirName, groundCounter);
					secID = format("SEC%02d", groundCounter);
				}
				else
				{
					tilesName = format("%s_%02d", dirName, fileCounter + 15);
					secID = format("SEC%02d", fileCounter + 15);
				}
			}
			else
			{
				tilesName = format("%s_%s", dirName, sector);
				secID = format("SEC%s", sector);
			}

			SecSdtStructure sdata;
			{
				auto fileName = dirName + UString("/") + sdtFile;

				auto fullPath = map_prefix + fileName;
				auto inFile = fw().data->fs.open(fullPath);
				if (!inFile)
				{
					LogInfo("Sector %s not present for map %d", sector, index);
					continue;
				}

				inFile.read((char *)&sdata, sizeof(sdata));
				if (!inFile)
				{
					LogError("Failed to read entry in \"%s\"", fileName);
					return;
				}
			}

			auto s = mksp<BattleMapSector>();

			s->size = {sdata.chunks_x, sdata.chunks_y, sdata.chunks_z};
			s->occurrence_min = sdata.occurrence_min;
			s->occurrence_max = sdata.occurrence_max;
			s->sectorTilesName = tilesName;
			if (tilesName == "40spawn_01")
			{
				s->spawnLocations[{&state, "AGENTTYPE_QUEENSPAWN"}].push_back({22, 15, 2});
				for (int x = 23; x < 28; x++)
				{
					for (int y = 13; y < 18; y++)
					{
						s->spawnLocations[{&state, "AGENTTYPE_MULTIWORM_EGG"}].push_back({x, y, 2});
					}
				}
				for (int x = 16; x < 21; x++)
				{
					for (int y = 12; y < 18; y++)
					{
						s->spawnLocations[{&state, "AGENTTYPE_MULTIWORM_EGG"}].push_back({x, y, 2});
					}
				}
			}
			m->sectors[secID] = s;
		} while (baseMap && ++groundCounter < 16);
	}

	if (bdata.destroyed_ground_idx != 0)
		m->destroyed_ground_tile = {&state,
		                            format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix,
		                                   "GD_", (unsigned)bdata.destroyed_ground_idx)};

	state.battle_maps[id] = m;
}

std::map<UString, up<BattleMapSectorTiles>>
InitialGameStateExtractor::extractMapSectors(GameState &state, const UString &mapRootName) const
{
	std::map<UString, up<BattleMapSectorTiles>> sectors;
	UString map_prefix = "xcom3/maps/";
	UString dirName = mapRootName;
	UString tilePrefix = format("%s_", dirName);
	bool baseMap = mapRootName == "37base";
	BuildingDatStructure bdata;
	{
		auto fileName = dirName + UString("/building.dat");

		auto datFileName = map_prefix + fileName;
		auto inFile = fw().data->fs.open(datFileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName);
			return {};
		}

		inFile.read((char *)&bdata, sizeof(bdata));
		if (!inFile)
		{
			LogError("Failed to read entry in \"%s\"", fileName);
			return {};
		}
	}
	auto sdtFiles = fw().data->fs.enumerateDirectory(map_prefix + "/" + dirName, ".sdt");

	int fileCounter = -1;
	for (const auto &sdtFile : sdtFiles)
	{
		fileCounter++;
		LogInfo("Reading map %s", sdtFile);
		/*  Trim off '.sdt' to get the base map name */
		LogAssert(sdtFile.length() >= 4);
		auto secName = sdtFile.substr(0, sdtFile.length() - 4);

		auto sector = secName.substr(secName.length() - 2);

		// We will make 16 total grounds
		int groundCounter = 0;
		do
		{
			UString tilesName;
			if (baseMap)
			{
				if (fileCounter == 0)
				{
					tilesName = format("%s_%02d", dirName, groundCounter);
				}
				else
				{
					tilesName = format("%s_%02d", dirName, fileCounter + 15);
				}
			}
			else
			{
				tilesName = format("%s_%s", dirName, sector);
			}
			up<BattleMapSectorTiles> tiles(new BattleMapSectorTiles());
			SecSdtStructure sdata;
			{
				auto fileName = dirName + UString("/") + secName + UString(".sdt");

				auto fullPath = map_prefix + fileName;
				auto inFile = fw().data->fs.open(fullPath);
				if (!inFile)
				{
					LogInfo("Sector %s not present for map %s", sector, mapRootName);
					continue;
				}

				inFile.read((char *)&sdata, sizeof(sdata));
				if (!inFile)
				{
					LogError("Failed to read entry in \"%s\"", fileName);
					return {};
				}
			}

			// Read LOS blocks
			{
				auto fileName = dirName + UString("/") + secName + UString(".sls");

				auto fullPath = map_prefix + fileName;
				auto inFile = fw().data->fs.open(fullPath);
				if (!inFile)
				{
					LogError("Failed to open \"%s\"", fileName);
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
						LogError("Failed to read entry %d in \"%s\"", i, fileName);
						return {};
					}

					auto los_block = mksp<BattleMapSector::LineOfSightBlock>();

					// Vanilla had inclusive boundaries, and they can go both ways, but we must make
					// them exclusive
					int x_min = std::min(ldata.begin_x, ldata.end_x);
					int x_max = std::max(ldata.begin_x, ldata.end_x);
					int y_min = std::min(ldata.begin_y, ldata.end_y);
					int y_max = std::max(ldata.begin_y, ldata.end_y);
					int z_min = std::min(ldata.begin_z, ldata.end_z);
					int z_max = std::max(ldata.begin_z, ldata.end_z);
					los_block->start = {x_min, y_min, z_min};
					los_block->end = {x_max + 1, y_max + 1, z_max + 1};
					los_block->ai_patrol_priority = ldata.ai_patrol_priority;
					los_block->ai_target_priority = ldata.ai_target_priority;

					// If this is alien map, then up the spawn priority for player spawns
					if (ldata.spawn_priority == 0 && ldata.spawn_type == SPAWN_TYPE_PLAYER)
					{
						for (auto &entry : missionObjectives)
						{
							if (entry.first == mapRootName)
							{
								ldata.spawn_priority = 1;
								break;
							}
						}
					}
					los_block->spawn_priority = ldata.spawn_priority;
					// It only matters if spawn priority is >0, so don't bother bloating xml with
					// unused
					// values
					if (los_block->spawn_priority > 0)
					{
						los_block->spawn_large_units = ldata.spawn_large == 1 && z_min != z_max;
						los_block->spawn_walking_units = ldata.spawn_walkers == 1;

						// Bases are special
						// Enemy and player spawn spots are different.
						// - Player spawns at Civilian (2)
						// - Enemy spawns at Player (0)
						// Priority 4 seems to mean noncombatants and priority 2 seems to mean
						// agents
						// For enemy spawns, priority 4 is encountered in hangars and 2 in lifts
						if (baseMap)
						{
							switch (ldata.spawn_type)
							{
								case SPAWN_TYPE_PLAYER:
									los_block->spawn_type = SpawnType::Enemy;
									// Fix this otherwise noone spawns in the lift
									// since there's so much more spawns in the repair bays
									los_block->spawn_priority = 2;
									break;
								case SPAWN_TYPE_CIVILIAN:
									if (los_block->spawn_priority == 4)
									{
										los_block->spawn_type = SpawnType::Civilian;
									}
									else
									{
										los_block->spawn_type = SpawnType::Player;
									}
									break;
								case SPAWN_TYPE_ENEMY:
									los_block->spawn_priority = 0;
									break;
								default:
									// Disable spawning and leave the type to its default value
									los_block->spawn_priority = 0;
									break;
							}
						}
						else
						{
							switch (ldata.spawn_type)
							{
								case SPAWN_TYPE_PLAYER:
									los_block->spawn_type = SpawnType::Player;
									break;
								case SPAWN_TYPE_ENEMY:
									los_block->spawn_type = SpawnType::Enemy;
									if (mapRootName == "41food")
									{
										los_block->also_allow_civilians = true;
									}
									break;
								case SPAWN_TYPE_CIVILIAN:
									los_block->spawn_type = SpawnType::Civilian;
									// Prevent civ spawn on spawn map, this was used to spawn queen
									if (mapRootName == "40spawn" &&
									    (los_block->spawn_priority == 4 ||
									     los_block->spawn_priority == 1))
									{
										los_block->spawn_priority = 0;
									}
									break;
								// TacEdit (map editor from vanilla creators) allows values up to 8,
								// but
								// they are unused, so we should accommodate for that
								default:
									// Disable spawning and leave the type to its default value
									los_block->spawn_priority = 0;
									break;
							}
						}
					}

					tiles->losBlocks.push_back(los_block);
				}
			}

			// Read Loot locations
			{
				auto fileName = dirName + UString("/") + secName + UString(".sob");

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
							LogError("Failed to read entry %d in \"%s\"", i, fileName);
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
				auto fileName = dirName + UString("/") + secName + UString(".smp");

				auto expectedFileSize = bdata.chunk_x * bdata.chunk_y * bdata.chunk_z *
				                        sdata.chunks_x * sdata.chunks_y * sdata.chunks_z * 4;
				auto fullPath = map_prefix + fileName;
				auto inFile = fw().data->fs.open(fullPath);
				if (!inFile)
				{
					LogError("Failed to open \"%s\"", fileName);
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
								         fileName);
								return {};
							}
							// read ground
							if (tdata.GD != 0)
							{
								auto tileName = format("%s%s%s%u", BattleMapPartType::getPrefix(),
								                       tilePrefix, "GD_", (unsigned)tdata.GD);

								tiles->initial_grounds[Vec3<int>{x, y, z}] = {&state, tileName};
							}
							// read left wall
							if (tdata.LW != 0)
							{
								auto tileName = format("%s%s%s%u", BattleMapPartType::getPrefix(),
								                       tilePrefix, "LW_", (unsigned)tdata.LW);

								tiles->initial_left_walls[Vec3<int>{x, y, z}] = {&state, tileName};
							}
							// read right wall
							if (tdata.RW != 0)
							{
								auto tileName = format("%s%s%s%u", BattleMapPartType::getPrefix(),
								                       tilePrefix, "RW_", (unsigned)tdata.RW);

								tiles->initial_right_walls[Vec3<int>{x, y, z}] = {&state, tileName};
							}
							// read scenery
							if (tdata.FT != 0)
							{
								if (baseMap && tdata.FT >= 230 && tdata.FT <= 237)
								{
									if (sector == "23")
									{
										tiles
										    ->guardianLocations[{
										        &state, "AGENTTYPE_X-COM_BASE_TURRET_LASER"}]
										    .push_back(Vec3<int>{x, y, z});
									}
									else if (sector == "24")
									{
										tiles
										    ->guardianLocations[{
										        &state, "AGENTTYPE_X-COM_BASE_TURRET_DISRUPTOR"}]
										    .push_back(Vec3<int>{x, y, z});
									}
									else
									{
										LogError("Encountered gun emplacement %d in sector %s",
										         tdata.FT, sector);
									}
								}
								else
								{
									auto tileName =
									    format("%s%s%s%u", BattleMapPartType::getPrefix(),
									           tilePrefix, "FT_", (unsigned)tdata.FT);

									tiles->initial_features[Vec3<int>{x, y, z}] = {&state,
									                                               tileName};
								}
							}
						}
					}
				}
			}

			// Manipulate sector map
			if (baseMap && fileCounter == 0 && groundCounter != 15)
			{
				auto tileName =
				    format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "FT_", 78);

				// key is North South West East (true = occupied, false = vacant)
				const std::unordered_map<int, std::vector<bool>> PRESENT_ROOMS = {
				    {0, {false, false, false, false}}, {1, {true, false, false, false}},
				    {2, {false, false, false, true}},  {3, {true, false, false, true}},
				    {4, {false, true, false, false}},  {5, {true, true, false, false}},
				    {6, {false, true, false, true}},   {7, {true, true, false, true}},
				    {8, {false, false, true, false}},  {9, {true, false, true, false}},
				    {10, {false, false, true, true}},  {11, {true, false, true, true}},
				    {12, {false, true, true, false}},  {13, {true, true, true, false}},
				    {14, {false, true, true, true}}};
				// Wipe north
				if (!PRESENT_ROOMS.at(groundCounter).at(0))
				{
					// y=0,y=1
					for (int x = 0; x < bdata.chunk_x * sdata.chunks_x; x++)
					{
						for (int y = 0; y < 2; y++)
						{
							for (int z = 2; z < bdata.chunk_z * sdata.chunks_z; z++)
							{
								if (tiles->initial_left_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_left_walls.end())
								{
									tiles->initial_left_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_right_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_right_walls.end())
								{
									tiles->initial_right_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_grounds.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_grounds.end())
								{
									tiles->initial_grounds.erase(Vec3<int>{x, y, z});
								}
								tiles->initial_features[Vec3<int>{x, y, z}] =
								    tiles->initial_features[Vec3<int>{x, y, 0}];
							}
						}
					}
				}
				// Wipe south
				if (!PRESENT_ROOMS.at(groundCounter).at(1))
				{
					// y = max-1
					for (int x = 0; x < bdata.chunk_x * sdata.chunks_x; x++)
					{
						for (int y = bdata.chunk_y * sdata.chunks_y - 1;
						     y < bdata.chunk_y * sdata.chunks_y; y++)
						{
							for (int z = 2; z < bdata.chunk_z * sdata.chunks_z; z++)
							{
								if (tiles->initial_left_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_left_walls.end())
								{
									tiles->initial_left_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_right_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_right_walls.end())
								{
									tiles->initial_right_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_grounds.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_grounds.end())
								{
									tiles->initial_grounds.erase(Vec3<int>{x, y, z});
								}
								tiles->initial_features[Vec3<int>{x, y, z}] =
								    tiles->initial_features[Vec3<int>{x, y, 0}];
							}
						}
					}
				}
				// Wipe west
				if (!PRESENT_ROOMS.at(groundCounter).at(2))
				{
					// x=0,x=1
					for (int x = 0; x < 2; x++)
					{
						for (int y = 0; y < bdata.chunk_y * sdata.chunks_y; y++)
						{
							for (int z = 2; z < bdata.chunk_z * sdata.chunks_z; z++)
							{
								if (tiles->initial_left_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_left_walls.end())
								{
									tiles->initial_left_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_right_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_right_walls.end())
								{
									tiles->initial_right_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_grounds.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_grounds.end())
								{
									tiles->initial_grounds.erase(Vec3<int>{x, y, z});
								}
								tiles->initial_features[Vec3<int>{x, y, z}] =
								    tiles->initial_features[Vec3<int>{x, y, 0}];
							}
						}
					}
				}
				// Wipe east
				if (!PRESENT_ROOMS.at(groundCounter).at(3))
				{
					// x = max-1
					for (int x = bdata.chunk_x * sdata.chunks_x - 1;
					     x < bdata.chunk_x * sdata.chunks_x; x++)
					{
						for (int y = 0; y < bdata.chunk_y * sdata.chunks_y; y++)
						{
							for (int z = 2; z < bdata.chunk_z * sdata.chunks_z; z++)
							{
								if (tiles->initial_left_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_left_walls.end())
								{
									tiles->initial_left_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_right_walls.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_right_walls.end())
								{
									tiles->initial_right_walls.erase(Vec3<int>{x, y, z});
								}
								if (tiles->initial_grounds.find(Vec3<int>{x, y, z}) !=
								    tiles->initial_grounds.end())
								{
									tiles->initial_grounds.erase(Vec3<int>{x, y, z});
								}
								tiles->initial_features[Vec3<int>{x, y, z}] =
								    tiles->initial_features[Vec3<int>{x, y, 0}];
							}
						}
					}
				}
			}

			sectors[tilesName] = std::move(tiles);
		} while (baseMap && ++groundCounter < 16);
	}

	return sectors;
}

void InitialGameStateExtractor::extractBattlescapeMap(
    GameState &state, const std::vector<OpenApoc::UString> &paths) const
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
