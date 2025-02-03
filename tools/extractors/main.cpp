#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapsector.h"
#include "game/state/rules/battle/battlemaptileset.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "library/strings_format.h"
#include "tools/extractors/extractors.h"
#include <SDL_main.h>
#include <list>

using namespace OpenApoc;

// FIXME: Make this dynamic?

std::vector<UString> supported_languages = {
    "cs.UTF-8",    "de_DE.UTF-8", "en.UTF-8",     "en_GB.UTF-8", "es.UTF-8",
    "et_EE.UTF-8", "fi.UTF-8",    "fil_PH.UTF-8", "fr_FR.UTF-8", "hu_HU.UTF-8",
    "it.UTF-8",    "ja.UTF-8",    "ja_JP.UTF-8",  "lt.UTF-8",    "nb_NO.UTF-8",
    "pl.UTF-8",    "pt_BR.UTF-8", "pt_PT.UTF-8",  "ro_RO.UTF-8", "ru_RU.UTF-8",
    "sk.UTF-8",    "sl_SI.UTF-8", "tr_TR.UTF-8",  "uk.UTF-8",    "zh_TW.UTF-8"};

static ConfigOptionString outputPath("Extractor", "output",
                                     "Path to the extractor output directory", "./data");

static void extractDifficulty(const InitialGameStateExtractor &e, UString outputPath,
                              InitialGameStateExtractor::Difficulty difficulty, UString patchPath)
{
	GameState s;
	e.extract(s, difficulty);
	// Alexey Andronov: This is problematic, as it duplicates stuff, so why were we doing it again!?
	// s.loadGame("data/common_patch");
	if (!patchPath.empty())
	{
		s.loadGame(patchPath);
	}
	s.saveGame(outputPath, true);
}

std::map<UString, std::function<void(const InitialGameStateExtractor &e)>> thingsToExtract = {
    {"difficulty0",
     [](const InitialGameStateExtractor &e)
     {
	     extractDifficulty(
	         e, outputPath.get() + "/mods/base/data/submods/org.openapoc.base/difficulty0",
	         InitialGameStateExtractor::Difficulty::DIFFICULTY_1, "data/difficulty0_patch");
     }},
    {"difficulty1",
     [](const InitialGameStateExtractor &e)
     {
	     extractDifficulty(
	         e, outputPath.get() + "/mods/base/data/submods/org.openapoc.base/difficulty1",
	         InitialGameStateExtractor::Difficulty::DIFFICULTY_2, "data/difficulty1_patch");
     }},
    {"difficulty2",
     [](const InitialGameStateExtractor &e)
     {
	     extractDifficulty(
	         e, outputPath.get() + "/mods/base/data/submods/org.openapoc.base/difficulty2",
	         InitialGameStateExtractor::Difficulty::DIFFICULTY_3, "data/difficulty2_patch");
     }},
    {"difficulty3",
     [](const InitialGameStateExtractor &e)
     {
	     extractDifficulty(
	         e, outputPath.get() + "/mods/base/data/submods/org.openapoc.base/difficulty3",
	         InitialGameStateExtractor::Difficulty::DIFFICULTY_4, "data/difficulty3_patch");
     }},
    {"difficulty4",
     [](const InitialGameStateExtractor &e)
     {
	     extractDifficulty(
	         e, outputPath.get() + "/mods/base/data/submods/org.openapoc.base/difficulty4",
	         InitialGameStateExtractor::Difficulty::DIFFICULTY_5, "data/difficulty4_patch");
     }},
    {"common_gamestate",
     [](const InitialGameStateExtractor &e)
     {
	     GameState s;
	     e.extractCommon(s);
	     s.loadGame("data/common_patch");
	     s.saveGame(outputPath.get() + "/mods/base/base_gamestate");
	     ModInfo info;
	     info.setName("OpenApoc base game");
	     info.setAuthor("OpenApoc team");
	     info.setVersion("0.1");
	     info.setDescription("The base OpenApoc game");
	     info.setLink("http://www.openapoc.org");
	     info.setID("org.openapoc.base");
	     info.setStatePath("base_gamestate");
	     info.setDataPath("data");
	     info.setModLoadScript("scripts/org.openapoc.base/onload.lua");

	     std::list<UString> languages;
	     for (const auto &name : supported_languages)
	     {
		     languages.push_back(name);
	     }
	     info.setSupportedLanguage(languages);

	     info.writeInfo(outputPath.get() + "/mods/base");
     }},
    {"city_bullet_sprites",
     [](const InitialGameStateExtractor &e)
     {
	     auto bullet_sprites = e.extractBulletSpritesCity();

	     for (auto &sprite_pair : bullet_sprites)
	     {
		     auto path = outputPath.get() + "/" + sprite_pair.first;
		     fw().data->writeImage(path, sprite_pair.second);
	     }
     }},
    {"battle_bullet_sprites",
     [](const InitialGameStateExtractor &e)
     {
	     auto bullet_sprites = e.extractBulletSpritesBattle();

	     for (auto &sprite_pair : bullet_sprites)
	     {
		     auto path = outputPath.get() + "/" + sprite_pair.first;
		     fw().data->writeImage(path, sprite_pair.second,
		                           fw().data->loadPalette("xcom3/tacdata/tactical.pal"));
	     }
     }},
    {"unit_image_packs",
     [](const InitialGameStateExtractor &e)
     {
	     for (auto &imagePackStrings : e.unitImagePackPaths)
	     {
		     GameState s;
		     LogInfo("Extracting image pack \"{}\"", imagePackStrings.first);

		     auto imagePack = e.extractImagePack(s, imagePackStrings.second, false);
		     if (!imagePack)
		     {
			     LogError("Failed to extract image pack \"{}\"", imagePackStrings.first);
		     }
		     else
		     {
			     if (!imagePack->saveImagePack(fw().getDataDir() +
			                                       BattleUnitImagePack::getImagePackPath() + "/" +
			                                       imagePackStrings.first,
			                                   true))
			     {
				     LogError("Failed to save image pack \"{}\"", imagePackStrings.first);
			     }
		     }
	     }
     }},
    {"item_image_packs",
     [](const InitialGameStateExtractor &e)
     {
	     int itemImagePacksCount = e.getItemImagePacksCount();
	     for (int i = 0; i < itemImagePacksCount; i++)
	     {
		     GameState s;
		     LogInfo("Extracting item image pack \"{}\"", i);

		     auto imagePack = e.extractItemImagePack(s, i);
		     if (!imagePack)
		     {
			     LogError("Failed to extract  item image pack \"{}\"", i);
		     }
		     else
		     {
			     if (!imagePack->saveImagePack(
			             fmt::format("{}{}{}",
			                         fw().getDataDir() + BattleUnitImagePack::getImagePackPath(),
			                         "/item", i),
			             true))
			     {
				     LogError("Failed to save  item image pack \"{}\"", i);
			     }
		     }
	     }
     }},
    {"unit_shadow_packs",
     [](const InitialGameStateExtractor &e)
     {
	     for (auto &imagePackStrings : e.unitShadowPackPaths)
	     {
		     GameState s;
		     LogInfo("Extracting image pack \"{}\"", imagePackStrings.first);

		     auto imagePack = e.extractImagePack(s, imagePackStrings.second, true);
		     if (!imagePack)
		     {
			     LogError("Failed to extract image pack \"{}\"", imagePackStrings.first);
		     }
		     else
		     {
			     if (!imagePack->saveImagePack(fw().getDataDir() +
			                                       BattleUnitImagePack::getImagePackPath() + "/" +
			                                       imagePackStrings.first,
			                                   true))
			     {
				     LogError("Failed to save image pack \"{}\"", imagePackStrings.first);
			     }
		     }
	     }
     }},
    {"unit_animation_packs",
     [](const InitialGameStateExtractor &e)
     {
	     for (auto &animationPackStrings : e.unitAnimationPackPaths)
	     {
		     GameState s;
		     LogInfo("Extracting animation pack \"{}\"", animationPackStrings.first);

		     auto animationPack =
		         e.extractAnimationPack(s, animationPackStrings.second, animationPackStrings.first);
		     if (!animationPack)
		     {
			     LogError("Failed to extract animation pack \"{}\"", animationPackStrings.first);
		     }
		     else
		     {
			     if (!animationPack->saveAnimationPack(
			             fw().getDataDir() + BattleUnitAnimationPack::getAnimationPackPath() + "/" +
			                 animationPackStrings.first,
			             true))
			     {
				     LogError("Failed to save animation pack \"{}\"", animationPackStrings.first);
			     }
		     }
	     }
     }},

    {"battle_map_tilesets",
     [](const InitialGameStateExtractor &e)
     {
	     for (auto &tileSetName : e.battleMapPaths)
	     {
		     // Some indices are empty?
		     if (tileSetName.empty())
			     continue;
		     GameState s;
		     LogInfo("Extracting tileset \"{}\"", tileSetName);

		     auto tileSet = e.extractTileSet(s, tileSetName);
		     if (!tileSet)
		     {
			     LogError("Failed to extract tileset \"{}\"", tileSetName);
		     }
		     else
		     {
			     if (!tileSet->saveTileset(BattleMapTileset::getTilesetPath() + "/" + tileSetName,
			                               true))
			     {
				     LogError("Failed to save tileset \"{}\"", tileSetName);
			     }
		     }
	     }
     }},
    {"battle_map_sectors",
     [](const InitialGameStateExtractor &e)
     {
	     for (auto &mapName : e.battleMapPaths)
	     {
		     // Some indices are empty?
		     if (mapName.empty())
			     continue;
		     GameState s;
		     LogInfo("Extracting map sectors from \"{}\"", mapName);

		     auto sectors = e.extractMapSectors(s, mapName);
		     LogInfo("Extracted {} sectors from \"{}\"", (unsigned)sectors.size(), mapName);
		     if (sectors.empty())
		     {
			     LogError("Failed to sectors from map \"{}\"", mapName);
		     }
		     for (auto &sectorPair : sectors)
		     {
			     auto &sectorName = sectorPair.first;
			     auto &sector = sectorPair.second;
			     auto path = BattleMapSectorTiles::getMapSectorPath();

			     sector->loadSector(s, "data/resource_patch/maps/" + sectorName);

			     if (!sector->saveSector(path + "/" + sectorName, true))
			     {
				     LogError("Failed to save map sector \"{}\"", sectorName);
			     }
		     }
	     }
     }},
};

int main(int argc, char *argv[])
{
	ConfigOptionString extractList(
	    "Extractor", "extract",
	    "Comma-separated list of things to extract  - \"all\" is special meaning everything",
	    "all");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	auto extractListString = extractList.get();

	std::list<std::pair<UString, std::function<void(const InitialGameStateExtractor &e)>>>
	    extractorsToRun;

	if (extractListString == "all")
	{
		LogWarning("Running all extractors");
		for (auto &ePair : thingsToExtract)
		{
			extractorsToRun.push_back(ePair);
		}
	}
	else
	{
		auto list = split(extractListString, ",");
		for (auto &extractorName : list)
		{
			auto extractor = thingsToExtract.find(extractorName);
			if (extractor == thingsToExtract.end())
			{
				LogError("Unknown extractor {}", extractorName);
				return EXIT_FAILURE;
			}
			else
			{
				extractorsToRun.push_back(*extractor);
			}
		}
	}
	Framework fw(UString(argv[0]), false);
	InitialGameStateExtractor initialGameStateExtractor;
	for (auto &ePair : extractorsToRun)
	{
		LogWarning("Running {}", ePair.first);
		ePair.second(initialGameStateExtractor);
	}

	return 0;
}
