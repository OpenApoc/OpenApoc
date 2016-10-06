#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "library/strings_format.h"
#include "tools/extractors/extractors.h"
#include <SDL_main.h>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	LogInfo("Starting OpenApoc_DataExtractor");

	{

		Trace::setThreadName("main");

		TraceObj obj("main");
		Framework *fw = new Framework(UString(argv[0]), false);
		InitialGameStateExtractor e;

		{
			auto bullet_sprites = e.extractBulletSpritesCity();

			for (auto &sprite_pair : bullet_sprites)
			{
				auto path = "data/" + sprite_pair.first;
				fw->data->writeImage(path, sprite_pair.second);
			}
		}
		{
			auto bullet_sprites = e.extractBulletSpritesBattle();

			for (auto &sprite_pair : bullet_sprites)
			{
				auto path = "data/" + sprite_pair.first;
				fw->data->writeImage(path, sprite_pair.second,
				                     fw->data->loadPalette("xcom3/tacdata/tactical.pal"));
			}
		}

		std::map<UString, InitialGameStateExtractor::Difficulty> difficultyOutputFiles = {
		    {"data/difficulty1", InitialGameStateExtractor::Difficulty::DIFFICULTY_1},
		    {"data/difficulty2", InitialGameStateExtractor::Difficulty::DIFFICULTY_2},
		    {"data/difficulty3", InitialGameStateExtractor::Difficulty::DIFFICULTY_3},
		    {"data/difficulty4", InitialGameStateExtractor::Difficulty::DIFFICULTY_4},
		    {"data/difficulty5", InitialGameStateExtractor::Difficulty::DIFFICULTY_5},
		};

		/*
		auto dpair = std::pair<UString,
		InitialGameStateExtractor::Difficulty>("data/difficulty1",
		InitialGameStateExtractor::Difficulty::DIFFICULTY_1);
		*/
		for (auto &dpair : difficultyOutputFiles)
		{
			GameState s;
			LogWarning("Extracting initial game state for \"%s\"", dpair.first.cStr());
			e.extract(s, dpair.second);
			LogWarning("Finished extracting initial game state for \"%s\"", dpair.first.cStr());

			LogWarning("Importing common patch");
			s.loadGame("data/common_patch");
			LogWarning("Done importing common patch");

			UString patchName = dpair.first + "_patch";
			LogWarning("Trying to import patch \"%s\"", patchName.cStr());
			s.loadGame(patchName);
			LogWarning("Patching finished");

			UString patchedOutputName = dpair.first + "_patched";
			LogWarning("Saving patched state to \"%s\"", patchedOutputName.cStr());
			s.saveGame(patchedOutputName, false);
			LogWarning("Done saving patched state");
		}

		LogWarning("Extracting Unit Image Packs");
		for (auto &imagePackStrings : e.unitImagePackPaths)
		{
			GameState s;
			LogInfo("Extracting image pack \"%s\"", imagePackStrings.first.cStr());

			auto imagePack = e.extractImagePack(s, imagePackStrings.second, false);
			if (!imagePack)
			{
				LogError("Failed to extract image pack \"%s\"", imagePackStrings.first.cStr());
			}
			else
			{
				if (!imagePack->saveImagePack(
				        BattleUnitImagePack::imagePackPath + "/" + imagePackStrings.first, false))
				{
					LogError("Failed to save image pack \"%s\"", imagePackStrings.first.cStr());
				}
			}
		}

		LogWarning("Extracting Item Image Packs");
		int itemImagePacksCount = e.getItemImagePacksCount();
		for (int i = 0; i < itemImagePacksCount; i++)
		{
			GameState s;
			LogInfo("Extracting item image pack \"%d\"", i);

			auto imagePack = e.extractItemImagePack(s, i);
			if (!imagePack)
			{
				LogError("Failed to extract  item image pack \"%d\"", i);
			}
			else
			{
				if (!imagePack->saveImagePack(
				        format("%s%s%d", BattleUnitImagePack::imagePackPath, "/item", i), false))
				{
					LogError("Failed to save  item image pack \"%d\"", i);
				}
			}
		}

		LogWarning("Extracting Unit Shadow Packs");
		for (auto &imagePackStrings : e.unitShadowPackPaths)
		{
			GameState s;
			LogInfo("Extracting image pack \"%s\"", imagePackStrings.first.cStr());

			auto imagePack = e.extractImagePack(s, imagePackStrings.second, true);
			if (!imagePack)
			{
				LogError("Failed to extract image pack \"%s\"", imagePackStrings.first.cStr());
			}
			else
			{
				if (!imagePack->saveImagePack(
				        BattleUnitImagePack::imagePackPath + "/" + imagePackStrings.first, false))
				{
					LogError("Failed to save image pack \"%s\"", imagePackStrings.first.cStr());
				}
			}
		}

		LogWarning("Extracting Unit Animation Packs");
		for (auto &animationPackStrings : e.unitAnimationPackPaths)
		{
			GameState s;
			LogInfo("Extracting animation pack \"%s\"", animationPackStrings.first.cStr());

			auto animationPack =
			    e.extractAnimationPack(s, animationPackStrings.second, animationPackStrings.first);
			if (!animationPack)
			{
				LogError("Failed to extract animation pack \"%s\"",
				         animationPackStrings.first.cStr());
			}
			else
			{
				if (!animationPack->saveAnimationPack(BattleUnitAnimationPack::animationPackPath +
				                                          "/" + animationPackStrings.first,
				                                      false))
				{
					LogError("Failed to save animation pack \"%s\"",
					         animationPackStrings.first.cStr());
				}
			}
		}

		LogWarning("Extracting Battle Map Tilesets");
		for (auto &tileSetName : e.battleMapPaths)
		{
			// Some indices are empty?
			if (tileSetName.empty())
				continue;
			GameState s;
			LogInfo("Extracting tileset \"%s\"", tileSetName.cStr());

			auto tileSet = e.extractTileSet(s, tileSetName);
			if (!tileSet)
			{
				LogError("Failed to extract tileset \"%s\"", tileSetName.cStr());
			}
			else
			{
				if (!tileSet->saveTileset(BattleMapTileset::tilesetPath + "/" + tileSetName, false))
				{
					LogError("Failed to save tileset \"%s\"", tileSetName.cStr());
				}
			}
		}

		LogWarning("Extracting Battle Map Sectors");
		for (auto &mapName : e.battleMapPaths)
		{
			// Some indices are empty?
			if (mapName.empty())
				continue;
			GameState s;
			LogInfo("Extracting map sectors from \"%s\"", mapName.cStr());

			auto sectors = e.extractMapSectors(s, mapName);
			LogInfo("Extracted %u sectors from \"%s\"", (unsigned)sectors.size(), mapName.cStr());
			if (sectors.empty())
			{
				LogError("Failed to sectors from map \"%s\"", mapName.cStr());
			}
			for (auto &sectorPair : sectors)
			{
				auto &sectorName = sectorPair.first;
				auto &sector = sectorPair.second;
				auto path = BattleMapSectorTiles::mapSectorPath;
				if (!sector->saveSector(BattleMapSectorTiles::mapSectorPath + "/" + sectorName,
				                        false))
				{
					LogError("Failed to save map sector \"%s\"", sectorName.cStr());
				}
			}
		}

		delete fw;
	}

	return 0;
}
