#include "framework/data.h"
#include "framework/framework.h"
#include "framework/palette.h"
#include "library/voxel.h"
#include "tools/extractors/extractors.h"

namespace OpenApoc
{

#pragma pack(push, 1)
struct battlemap_entry
{
	uint8_t unknown1[15];
	uint8_t loftemps[40];
	uint8_t unknown2[31];
};
#pragma pack(pop)

static_assert(sizeof(struct battlemap_entry) == 86, "Unexpected citymap_tile_entry size");

void InitialGameStateExtractor::extractBattlescapeStuff(GameState &state, UString dirName)
{
	UString tilePrefix = dirName + UString("_");

	UString map_prefix = "xcom3/MAPS/";
	UString mapunits_suffix = "/MAPUNITS/";
	UString spriteFile;
	UString datFile;
	UString fileName;

	// Read ground (tiles)
	{
		spriteFile = "GROUND";
		datFile = "GROUNMAP";
		fileName = dirName + mapunits_suffix + datFile + UString(".DAT");

		auto inFile = fw().data->fs.open(map_prefix + fileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
		}
		auto fileSize = inFile.size();
		auto objectCount = fileSize / sizeof(struct battlemap_entry);

		auto strategyTabFile = fw().data->fs.open(map_prefix + dirName + mapunits_suffix +
		                                          UString("S") + spriteFile + UString(".TAB"));
		unsigned strategySpritesCount = 0;
		if (strategyTabFile)
		{
			strategySpritesCount = strategyTabFile.size() / 4;
		}

		LogInfo("Loading %zu ground entries, %zu strategy sprites", objectCount,
		        strategySpritesCount);
		for (unsigned i = 0; i < objectCount; i++)
		{
			struct battlemap_entry entry;
			inFile.read((char *)&entry, sizeof(entry));

			UString id =
			    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "GD_", i);

			auto object = mksp<BattleMapPartType>();
			object->type = BattleMapPartType::Type::Ground;

			// FIXME: In the future, here we will be reading tile information like health etc.

			object->voxelMap = mksp<VoxelMap>(Vec3<int>{32, 32, 40});
			for (unsigned z = 0; z < 40; z++)
			{
				auto lofString = UString::format(
				    "LOFTEMPS:xcom3/TACDATA/LOFTEMPS.DAT:xcom3/TACDATA/LOFTEMPS.TAB:%d",
				    (int)entry.loftemps[z]);
				object->voxelMap->slices[z] = fw().data->loadVoxelSlice(lofString);
			}

			auto imageString = UString::format("PCK:" + map_prefix + dirName + mapunits_suffix +
			                                       spriteFile + ".PCK:" + map_prefix + dirName +
			                                       mapunits_suffix + spriteFile + ".TAB:%u",
			                                   i);
			object->sprite = fw().data->loadImage(imageString);
			if (i < strategySpritesCount)
			{
				imageString = UString::format(
				    "PCK:" + map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".PCK:" +
				        map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".TAB:%u",
				    i);
				object->strategySprite = fw().data->loadImage(imageString);
			}

			object->imageOffset = {24, 48};

			state.battle.map_part_types[id] = object;
		}
	}

	// Read left walls
	{
		spriteFile = "LEFT";
		datFile = "LEFTMAP";
		fileName = dirName + mapunits_suffix + datFile + UString(".DAT");

		auto inFile = fw().data->fs.open(map_prefix + fileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
		}
		auto fileSize = inFile.size();
		auto objectCount = fileSize / sizeof(struct battlemap_entry);

		auto strategyTabFile = fw().data->fs.open(map_prefix + dirName + mapunits_suffix +
		                                          UString("S") + spriteFile + UString(".TAB"));
		auto strategySpritesCount = 0;
		if (strategyTabFile)
		{
			strategySpritesCount = strategyTabFile.size() / 4;
		}

		LogInfo("Loading %zu left wall entries, %zu strategy sprites", objectCount,
		        strategySpritesCount);
		for (unsigned i = 0; i < objectCount; i++)
		{
			struct battlemap_entry entry;
			inFile.read((char *)&entry, sizeof(entry));

			UString id =
			    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "LW_", i);

			auto object = mksp<BattleMapPartType>();
			object->type = BattleMapPartType::Type::LeftWall;

			// FIXME: In the future, here we will be reading tile information like health etc.

			object->voxelMap = mksp<VoxelMap>(Vec3<int>{32, 32, 40});
			for (unsigned z = 0; z < 40; z++)
			{
				auto lofString = UString::format(
				    "LOFTEMPS:xcom3/TACDATA/LOFTEMPS.DAT:xcom3/TACDATA/LOFTEMPS.TAB:%d",
				    (int)entry.loftemps[z]);
				object->voxelMap->slices[z] = fw().data->loadVoxelSlice(lofString);
			}

			auto imageString = UString::format("PCK:" + map_prefix + dirName + mapunits_suffix +
			                                       spriteFile + ".PCK:" + map_prefix + dirName +
			                                       mapunits_suffix + spriteFile + ".TAB:%u",
			                                   i);
			object->sprite = fw().data->loadImage(imageString);
			if (i < strategySpritesCount)
			{
				imageString = UString::format(
				    "PCK:" + map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".PCK:" +
				        map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".TAB:%u",
				    i);
				object->strategySprite = fw().data->loadImage(imageString);
			}

			object->imageOffset = {24, 48};

			state.battle.map_part_types[id] = object;
		}
	}

	// Read right walls
	{
		spriteFile = "RIGHT";
		datFile = "RIGHTMAP";

		fileName = dirName + mapunits_suffix + datFile + UString(".DAT");

		auto inFile = fw().data->fs.open(map_prefix + fileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
		}
		auto fileSize = inFile.size();
		auto objectCount = fileSize / sizeof(struct battlemap_entry);

		auto strategyTabFile = fw().data->fs.open(map_prefix + dirName + mapunits_suffix +
		                                          UString("S") + spriteFile + UString(".TAB"));
		auto strategySpritesCount = 0;
		if (strategyTabFile)
		{
			strategySpritesCount = strategyTabFile.size() / 4;
		}

		LogInfo("Loading %zu right wall entries, %zu strategy sprites", objectCount,
		        strategySpritesCount);
		for (unsigned i = 0; i < objectCount; i++)
		{
			struct battlemap_entry entry;
			inFile.read((char *)&entry, sizeof(entry));

			UString id =
			    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "RW_", i);

			auto object = mksp<BattleMapPartType>();
			object->type = BattleMapPartType::Type::RightWall;

			// FIXME: In the future, here we will be reading tile information like health etc.

			object->voxelMap = mksp<VoxelMap>(Vec3<int>{32, 32, 40});
			for (unsigned z = 0; z < 40; z++)
			{
				auto lofString = UString::format(
				    "LOFTEMPS:xcom3/TACDATA/LOFTEMPS.DAT:xcom3/TACDATA/LOFTEMPS.TAB:%d",
				    (int)entry.loftemps[z]);
				object->voxelMap->slices[z] = fw().data->loadVoxelSlice(lofString);
			}

			auto imageString = UString::format("PCK:" + map_prefix + dirName + mapunits_suffix +
			                                       spriteFile + ".PCK:" + map_prefix + dirName +
			                                       mapunits_suffix + spriteFile + ".TAB:%u",
			                                   i);
			object->sprite = fw().data->loadImage(imageString);
			if (i < strategySpritesCount)
			{
				imageString = UString::format(
				    "PCK:" + map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".PCK:" +
				        map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".TAB:%u",
				    i);
				object->strategySprite = fw().data->loadImage(imageString);
			}

			object->imageOffset = {24, 48};

			state.battle.map_part_types[id] = object;
		}
	}

	// Read scenery
	{
		spriteFile = "FEATURE";
		datFile = "FEATMAP";

		fileName = dirName + mapunits_suffix + datFile + UString(".DAT");

		auto inFile = fw().data->fs.open(map_prefix + fileName);
		if (!inFile)
		{
			LogError("Failed to open \"%s\"", fileName.cStr());
		}
		auto fileSize = inFile.size();
		auto objectCount = fileSize / sizeof(struct battlemap_entry);

		auto strategyTabFile = fw().data->fs.open(map_prefix + dirName + mapunits_suffix +
		                                          UString("S") + spriteFile + UString(".TAB"));
		auto strategySpritesCount = 0;
		if (strategyTabFile)
		{
			strategySpritesCount = strategyTabFile.size() / 4;
		}

		LogInfo("Loading %zu feature entries, %zu strategy sprites", objectCount,
		        strategySpritesCount);
		for (unsigned i = 0; i < objectCount; i++)
		{
			struct battlemap_entry entry;
			inFile.read((char *)&entry, sizeof(entry));

			UString id =
			    UString::format("%s%s%s%u", BattleMapPartType::getPrefix(), tilePrefix, "SC_", i);

			auto object = mksp<BattleMapPartType>();
			object->type = BattleMapPartType::Type::Scenery;

			// FIXME: In the future, here we will be reading tile information like health etc.

			object->voxelMap = mksp<VoxelMap>(Vec3<int>{32, 32, 40});
			for (unsigned z = 0; z < 40; z++)
			{
				auto lofString = UString::format(
				    "LOFTEMPS:xcom3/TACDATA/LOFTEMPS.DAT:xcom3/TACDATA/LOFTEMPS.TAB:%d",
				    (int)entry.loftemps[z]);
				object->voxelMap->slices[z] = fw().data->loadVoxelSlice(lofString);
			}

			auto imageString = UString::format("PCK:" + map_prefix + dirName + mapunits_suffix +
			                                       spriteFile + ".PCK:" + map_prefix + dirName +
			                                       mapunits_suffix + spriteFile + ".TAB:%u",
			                                   i);
			object->sprite = fw().data->loadImage(imageString);
			if (i < strategySpritesCount)
			{
				imageString = UString::format(
				    "PCK:" + map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".PCK:" +
				        map_prefix + dirName + mapunits_suffix + "S" + spriteFile + ".TAB:%u",
				    i);
				object->strategySprite = fw().data->loadImage(imageString);
			}
			object->imageOffset = {24, 48};

			state.battle.map_part_types[id] = object;
		}
	}
}

} // namespace OpenApoc
