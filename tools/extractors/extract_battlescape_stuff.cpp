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
	uint8_t constitution;
	uint8_t unknown7;
	uint8_t explosion_power;
	uint8_t unknown8;
	uint8_t explosion_radius_divizor;
	uint8_t unknown9;
	uint8_t explosion_type;
	uint8_t unknown10;
	uint8_t unknown1[7];
	uint8_t loftemps[40];
	uint8_t unknown2[5];
	uint8_t damaged_idx;
	uint8_t unknown6;
	uint8_t animation_idx;
	uint8_t unknown5;
	uint8_t animation_length;
	uint8_t unknown4;
	uint8_t unknown3[20];
};
#pragma pack(pop)

static_assert(sizeof(struct battlemap_entry) == 86, "Unexpected battlemap_entry size");

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
		object->constitution = entry.constitution;
		object->explosion_power = entry.explosion_power;
		object->explosion_radius_divizor = entry.explosion_radius_divizor;
		switch (entry.explosion_type)
		{
			case 0:
				object->explosion_type = BattleMapPartType::ExplosionType::BlankOrSmoke;
				break;
			case 1:
				object->explosion_type = BattleMapPartType::ExplosionType::AlienGas;
				break;
			case 2:
				object->explosion_type = BattleMapPartType::ExplosionType::Incendary;
				break;
			case 3:
				object->explosion_type = BattleMapPartType::ExplosionType::StunGas;
				break;
			case 4:
				object->explosion_type = BattleMapPartType::ExplosionType::HighExplosive;
				break;
			default:
				LogError("Unexpected explosion type %d for ID %s", (int)entry.explosion_type,
				         id.cStr());
				break;
		}

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

		if (entry.damaged_idx)
		{
			object->damaged_map_part = {&state,
			                            UString::format("%s%u", idPrefix, entry.damaged_idx)};
		}

		// So far haven't seen an animated object with only 1 frame, but seen objects with 1 in this
		// field that are not actually animated via animated frames,
		if (entry.animation_length > 1)
		{
			for (int j = 0; j < entry.animation_length; j++)
			{
				auto animateString =
				    UString::format("PCK:%s%s.PCK:%s%s.TAB:%u", dirName.cStr(), "ANIMATE",
				                    dirName.cStr(), "ANIMATE", entry.animation_idx + j);
				object->animation_frames.push_back(fw().data->loadImage(animateString));
			}
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

		if (false) // debug output to get images into files
		{
			auto tmp = std::dynamic_pointer_cast<PaletteImage>(object->sprite)
			               ->toRGBImage(fw().data->loadPalette("xcom3/tacdata/tactical.pal"));
			tmp->saveBitmap("E:\\TEMP\\X\\" + id + ".bmp");
		}
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
