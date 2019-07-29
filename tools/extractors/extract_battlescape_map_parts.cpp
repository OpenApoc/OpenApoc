#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/battle/battlemaptileset.h"
#include "game/state/tilemap/tilemap.h"
#include "library/strings_format.h"
#include "library/voxel.h"
#include "tools/extractors/common/battlemap.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::readBattleMapParts(
    GameState &state, const TACP &data_t, sp<BattleMapTileset> t, BattleMapPartType::Type type,
    const UString &idPrefix, const UString &mapName, const UString &dirName, const UString &datName,
    const UString &pckName, const UString &stratPckName) const
{
	const UString loftempsFile = "xcom3/tacdata/loftemps.dat";
	const UString loftempsTab = "xcom3/tacdata/loftemps.tab";

	auto datFileName = dirName + "/" + datName + ".dat";
	auto inFile = fw().data->fs.open(datFileName);
	if (!inFile)
	{
		LogError("Failed to open mapunits DAT file at \"%s\"", datFileName);
		return;
	}
	auto fileSize = inFile.size();
	auto objectCount = fileSize / sizeof(struct BattleMapPartEntry);
	auto firstExitIdx = objectCount - 4;

	auto strategySpriteTabFileName = dirName + "/" + stratPckName + ".tab";
	auto strategySpriteTabFile = fw().data->fs.open(strategySpriteTabFileName);
	if (!strategySpriteTabFile)
	{
		LogError("Failed to open strategy sprite TAB file \"%s\"", strategySpriteTabFileName);
		return;
	}
	size_t strategySpriteCount = strategySpriteTabFile.size() / 4;

	LogInfo("Loading %zu entries from \"%s\"", objectCount, datFileName);

	for (size_t i = 0; i < objectCount; i++)
	{

		struct BattleMapPartEntry entry;
		inFile.read((char *)&entry, sizeof(entry));
		if (!inFile)
		{
			LogError("Failed to read entry %zu in \"%s\"", i, datFileName);
			return;
		}

		UString id = format("%s%u", idPrefix, i);
		auto object = mksp<BattleMapPartType>();
		if (entry.alternative_object_idx != 0)
		{
			object->alternative_map_part = {&state,
			                                format("%s%u", idPrefix, entry.alternative_object_idx)};
		}
		object->type = type;
		object->constitution = entry.constitution;
		object->explosion_power = entry.explosion_power;
		object->explosion_depletion_rate = entry.explosion_depletion_rate;
		object->explosion_type = {&state, data_t.getDTypeId(entry.explosion_type)};

		object->fire_resist = entry.fire_resist;
		object->fire_burn_time = entry.fire_burn_time;
		object->block[DamageType::BlockType::Physical] = entry.block_physical;
		object->block[DamageType::BlockType::Gas] = entry.block_gas;
		object->block[DamageType::BlockType::Fire] = entry.block_fire;
		object->block[DamageType::BlockType::Psionic] = entry.block_psionic;
		object->size = entry.size;

		object->voxelMapLOF = mksp<VoxelMap>(Vec3<int>{24, 24, 20});
		for (int slice = 0; slice < 20; slice++)
		{
			if ((unsigned int)entry.loftemps_lof[slice] == 0)
				continue;
			auto lofString = format("LOFTEMPS:%s:%s:%u", loftempsFile, loftempsTab,
			                        (unsigned int)entry.loftemps_lof[slice]);
			object->voxelMapLOF->slices[slice] = fw().data->loadVoxelSlice(lofString);
		}
		object->voxelMapLOS = mksp<VoxelMap>(Vec3<int>{24, 24, 20});
		for (int slice = 0; slice < 20; slice++)
		{
			if ((unsigned int)entry.loftemps_los[slice] == 0)
				continue;
			auto lofString = format("LOFTEMPS:%s:%s:%u", loftempsFile, loftempsTab,
			                        (unsigned int)entry.loftemps_los[slice]);
			object->voxelMapLOS->slices[slice] = fw().data->loadVoxelSlice(lofString);
		}
		if (entry.damaged_idx)
		{
			object->damaged_map_part = {&state, format("%s%u", idPrefix, entry.damaged_idx)};
		}

		// So far haven't seen an animated object with only 1 frame, but seen objects with 1 in this
		// field that are not actually animated via animated frames, therefore, ignore them
		if (entry.animation_length > 1)
		{
			auto animateTabFileName = dirName + "/" + "animate.tab";
			auto animateTabFile = fw().data->fs.open(animateTabFileName);
			if (!animateTabFile)
			{
				LogError("Failed to open animate sprite TAB file \"%s\"", animateTabFileName);
				return;
			}
			size_t animateSpriteCount = animateTabFile.size() / 4;

			if (animateSpriteCount < entry.animation_idx + entry.animation_length)
			{
				LogWarning("Bogus animation value, animation frames not present for ID %s", id);
			}
			else
			{
				for (int j = 0; j < entry.animation_length; j++)
				{
					auto animateString = format("PCK:%s%s.pck:%s%s.tab:%u", dirName, "animate",
					                            dirName, "animate", entry.animation_idx + j);
					object->animation_frames.push_back(fw().data->loadImage(animateString));
				}
			}
		}

		auto imageString =
		    format("PCK:%s%s.pck:%s%s.tab:%u", dirName, pckName, dirName, pckName, i);
		object->sprite = fw().data->loadImage(imageString);
		if (i < strategySpriteCount)
		{
			auto stratImageString = format("PCKSTRAT:%s%s.pck:%s%s.tab:%u", dirName, stratPckName,
			                               dirName, stratPckName, i);
			object->strategySprite = fw().data->loadImage(stratImageString);
		}
		// It should be {24,34} I guess, since 48/2=24, but 23 gives a little better visual
		// correlation with the sprites
		object->imageOffset = BATTLE_IMAGE_OFFSET;

		object->transparent = entry.transparent == 1;
		object->sfxIndex = entry.sfx - 1;
		object->door = entry.is_door == 1 && entry.is_door_closed == 1;
		// Unused in vanilla
		// object->los_through_terrain = entry.los_through_terrain == 1;
		// Instead we mark objects based on whether they block los or not
		// For now, we simply cheat and check several voxels
		switch (type)
		{
			case BattleMapPartType::Type::Ground:
				// Ground blocks LOS if there's something in the middle column
				for (int i = 0; i < 20; i++)
				{
					if (object->voxelMapLOS->getBit({12, 12, i}))
					{
						object->blocksLOS = true;
						break;
					}
				}
				break;
			case BattleMapPartType::Type::LeftWall:
				// Wall blocks LOS if there's something in the middle line
				for (int i = 0; i < 24; i++)
				{
					if (object->voxelMapLOS->getBit({i, 12, 10}))
					{
						object->blocksLOS = true;
						break;
					}
				}
				break;
			case BattleMapPartType::Type::RightWall:
				// Wall blocks LOS if there's something in the middle line
				for (int i = 0; i < 24; i++)
				{
					if (object->voxelMapLOS->getBit({12, i, 10}))
					{
						object->blocksLOS = true;
						break;
					}
				}
				break;
			case BattleMapPartType::Type::Feature:
				// Feature blocks LOS if there's something in the middle cross
				for (int i = 0; i < 24; i++)
				{
					if (object->voxelMapLOS->getBit({12, i, 10}) ||
					    object->voxelMapLOS->getBit({i, 12, 10}))
					{
						object->blocksLOS = true;
						break;
					}
				}
				break;
		}
		object->floor = entry.is_floor == 1;
		object->gravlift = entry.is_gravlift == 1;
		object->movement_cost = entry.movement_cost;
		object->height = entry.height;
		object->floating = entry.is_floating;
		object->provides_support = entry.provides_support;

		int gets_support_from = 0;
		switch (entry.gets_support_from)
		{
			// 32 is a mistake, it should read "0"
			case 32:
			case 0:
				break;
			case 7:
			case 20:
				object->vanillaSupportedById = entry.gets_support_from;
				break;
			// 30 is a mistake, it should read "1"
			case 30:
				gets_support_from = 1;
				break;
			default:
				gets_support_from = entry.gets_support_from;
		}
		if (gets_support_from == 36)
		{
			object->supportedByDirections.insert(MapDirection::North);
			object->supportedByDirections.insert(MapDirection::West);
			object->supportedByTypes.insert(BattleMapPartType::Type::LeftWall);
			object->supportedByTypes.insert(BattleMapPartType::Type::RightWall);
			object->supportedByTypes.insert(BattleMapPartType::Type::Feature);
		}
		else if (gets_support_from == 5)
		{
			object->supportedByAbove = true;
		}
		else if (gets_support_from)
		{
			if (gets_support_from % 10 < 1 || gets_support_from % 10 > 4)
			{
				LogError("Unrecognized support by id %d", (int)entry.gets_support_from);
				return;
			}
			object->supportedByDirections.insert((MapDirection)(gets_support_from % 10));
			switch (gets_support_from / 10)
			{
				case 0:
					break;
				case 1:
					object->supportedByTypes.insert(BattleMapPartType::Type::Ground);
					break;
				case 2:
					object->supportedByTypes.insert(BattleMapPartType::Type::Feature);
					break;
				case 4:
					object->supportedByTypes.insert(object->type);
					if (object->type != BattleMapPartType::Type::Ground)
					{
						object->supportedByTypes.insert(BattleMapPartType::Type::Ground);
					}
					break;
				case 5:
					object->supportedByTypes.insert(BattleMapPartType::Type::Ground);
					object->supportedByTypes.insert(BattleMapPartType::Type::Feature);
					break;
				default:
					LogError("Unrecognized support by id %d", (int)entry.gets_support_from);
					return;
			}
		}

		object->independent_structure = entry.independent_structure;

		if (type == BattleMapPartType::Type::Ground && i >= firstExitIdx)
			object->exit = true;

		object->autoConvert = BattleMapPartType::AutoConvert::None;
		if (type == BattleMapPartType::Type::Feature)
		{
			if (initialFires.find(mapName) != initialFires.end() &&
			    initialFires.at(mapName).find(i) != initialFires.at(mapName).end())
			{
				object->autoConvert = BattleMapPartType::AutoConvert::Fire;
			}
			else if (initialSmokes.find(mapName) != initialSmokes.end() &&
			         initialSmokes.at(mapName).find(i) != initialSmokes.at(mapName).end())
			{
				object->autoConvert = BattleMapPartType::AutoConvert::Smoke;
			}
			if (missionObjectives.find(mapName) != missionObjectives.end() &&
			    missionObjectives.at(mapName).find(i) != missionObjectives.at(mapName).end())
			{
				object->missionObjective = true;
			}
		}
		if (type == BattleMapPartType::Type::Ground)
		{
			if (reinforcementSpawners.find(mapName) != reinforcementSpawners.end() &&
			    reinforcementSpawners.at(mapName).find(i) !=
			        reinforcementSpawners.at(mapName).end())
			{
				object->reinforcementSpawner = true;
			}
		}

		t->map_part_types[id] = object;
	}
}

sp<BattleMapTileset> InitialGameStateExtractor::extractTileSet(GameState &state,
                                                               const UString &name) const
{
	UString tilePrefix = format("%s_", name);
	UString map_prefix = "xcom3/maps/";
	UString mapunits_suffix = "/mapunits/";
	UString spriteFile;
	UString datFile;
	UString fileName;

	auto t = mksp<BattleMapTileset>();

	// Read ground (tiles)
	{
		readBattleMapParts(state, this->tacp, t, BattleMapPartType::Type::Ground,
		                   BattleMapPartType::getPrefix() + tilePrefix + "GD_", name,
		                   map_prefix + name + mapunits_suffix, "grounmap", "ground", "sground");
	}

	// Read left walls
	{
		readBattleMapParts(state, this->tacp, t, BattleMapPartType::Type::LeftWall,
		                   BattleMapPartType::getPrefix() + tilePrefix + "LW_", name,
		                   map_prefix + name + mapunits_suffix, "leftmap", "left", "sleft");
	}

	// Read right walls
	{
		readBattleMapParts(state, this->tacp, t, BattleMapPartType::Type::RightWall,
		                   BattleMapPartType::getPrefix() + tilePrefix + "RW_", name,
		                   map_prefix + name + mapunits_suffix, "rightmap", "right", "sright");
	}

	// Read feature
	{
		readBattleMapParts(state, this->tacp, t, BattleMapPartType::Type::Feature,
		                   BattleMapPartType::getPrefix() + tilePrefix + "FT_", name,
		                   map_prefix + name + mapunits_suffix, "featmap", "feature", "sfeature");
	}

	return t;
}

} // namespace OpenApoc
