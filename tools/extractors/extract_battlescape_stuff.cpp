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

static void readBattleMapParts(GameState &state, BattleMapPartType::Type type,
                               const UString &idPrefix, const UString &dirName,
                               const UString &datName, const UString &pckName,
                               const UString &stratPckName)
{
	const UString loftempsFile = "xcom3/TACDATA/LOFTEMPS.DAT";
	const UString loftempsTab = "xcom3/TACDATA/LOFTEMPS.TAB";
	auto datFileName = dirName + "/" + datName + ".DAT";
	auto inFile = fw().data->fs.open(datFileName);
	if (!inFile)
	{
		LogError("Failed to open mapunits DAT file at \"%s\"", datFileName.cStr());
		return;
	}
	auto fileSize = inFile.size();
	auto objectCount = fileSize / sizeof(struct battlemap_entry);

	auto strategySpriteTabFileName = dirName + "/" + stratPckName + ".tab";
	auto strategySpriteTabFile = fw().data->fs.open(strategySpriteTabFileName);
	if (!strategySpriteTabFile)
	{
		LogError("Failed to open strategy sprite TAB file \"%s\"",
		         strategySpriteTabFileName.cStr());
		return;
	}

	size_t strategySpriteCount = strategySpriteTabFile.size() / 4;

	LogInfo("Loading %zu entries from \"%s\"", objectCount, datFileName.cStr());

	for (size_t i = 0; i < objectCount; i++)
	{

		struct battlemap_entry entry;
		inFile.read((char *)&entry, sizeof(entry));
		if (!inFile)
		{
			LogError("Failed to read entry %zu in \"%s\"", i, datFileName.cStr());
			return;
		}

		UString id = UString::format("%s%u", idPrefix, i);
		auto object = mksp<BattleMapPartType>();
		object->type = type;
		object->voxelMap = mksp<VoxelMap>(Vec3<int>{32, 32, 40});
		for (int slice = 0; slice < 40; slice++)
		{
			auto lofString =
			    UString::format("LOFTEMPS:%s:%s:%u", loftempsFile.cStr(), loftempsTab.cStr(),
			                    (unsigned int)entry.loftemps[slice]);
			auto loftemp = fw().data->loadVoxelSlice(lofString);
			if (!loftemp)
			{
				LogError("Failed to open voxel slice \"%s\"", lofString.cStr());
				return;
			}
			object->voxelMap->slices[slice] = loftemp;
		}
		auto imageString = UString::format("PCK:%s%s.PCK:%s%s.TAB:%u", dirName.cStr(),
		                                   pckName.cStr(), dirName.cStr(), pckName.cStr(), i);
		object->sprite = fw().data->loadImage(imageString);
		if (i < strategySpriteCount)
		{
			auto stratImageString =
			    UString::format("PCKSTRAT:%s%s.PCK:%s%s.TAB:%u", dirName.cStr(),
			                    stratPckName.cStr(), dirName.cStr(), stratPckName.cStr(), i);
			object->strategySprite = fw().data->loadImage(stratImageString);
		}
		object->imageOffset = {24, 48};

		state.battle.map_part_types[id] = object;
	}
}

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
		readBattleMapParts(state, BattleMapPartType::Type::Ground,
		                   BattleMapPartType::getPrefix() + tilePrefix + "GD_",
		                   map_prefix + dirName + mapunits_suffix, "GROUNMAP", "GROUND", "SGROUND");
	}

	// Read left walls
	{
		readBattleMapParts(state, BattleMapPartType::Type::LeftWall,
		                   BattleMapPartType::getPrefix() + tilePrefix + "LW_",
		                   map_prefix + dirName + mapunits_suffix, "LEFTMAP", "LEFT", "SLEFT");
	}

	// Read right walls
	{
		readBattleMapParts(state, BattleMapPartType::Type::RightWall,
		                   BattleMapPartType::getPrefix() + tilePrefix + "RW_",
		                   map_prefix + dirName + mapunits_suffix, "RIGHTMAP", "RIGHT", "SRIGHT");
	}

	// Read scenery
	{
		readBattleMapParts(state, BattleMapPartType::Type::Scenery,
		                   BattleMapPartType::getPrefix() + tilePrefix + "SC_",
		                   map_prefix + dirName + mapunits_suffix, "FEATMAP", "FEATURE",
		                   "SFEATURE");
	}
}

} // namespace OpenApoc
