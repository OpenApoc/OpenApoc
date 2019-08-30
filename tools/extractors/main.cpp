#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapsector.h"
#include "game/state/rules/battle/battlemaptileset.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "library/strings_format.h"
#include "tools/extractors/extractors.h"
#include <SDL_main.h>
#include <list>

using namespace OpenApoc;

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
    {"difficulty1",
     [](const InitialGameStateExtractor &e) {
	     extractDifficulty(e, "data/difficulty1_patched",
	                       InitialGameStateExtractor::Difficulty::DIFFICULTY_1,
	                       "data/difficulty1_patch");
     }},
    {"difficulty2",
     [](const InitialGameStateExtractor &e) {
	     extractDifficulty(e, "data/difficulty2_patched",
	                       InitialGameStateExtractor::Difficulty::DIFFICULTY_2,
	                       "data/difficulty2_patch");
     }},
    {"difficulty3",
     [](const InitialGameStateExtractor &e) {
	     extractDifficulty(e, "data/difficulty3_patched",
	                       InitialGameStateExtractor::Difficulty::DIFFICULTY_3,
	                       "data/difficulty3_patch");
     }},
    {"difficulty4",
     [](const InitialGameStateExtractor &e) {
	     extractDifficulty(e, "data/difficulty4_patched",
	                       InitialGameStateExtractor::Difficulty::DIFFICULTY_4,
	                       "data/difficulty4_patch");
     }},
    {"difficulty5",
     [](const InitialGameStateExtractor &e) {
	     extractDifficulty(e, "data/difficulty5_patched",
	                       InitialGameStateExtractor::Difficulty::DIFFICULTY_5,
	                       "data/difficulty5_patch");
     }},
    {"common_gamestate",
     [](const InitialGameStateExtractor &e) {
	     GameState s;
	     e.extractCommon(s);
	     s.loadGame("data/common_patch");
	     s.saveGame("data/gamestate_common");
     }},
    {"city_bullet_sprites",
     [](const InitialGameStateExtractor &e) {
	     auto bullet_sprites = e.extractBulletSpritesCity();

	     for (auto &sprite_pair : bullet_sprites)
	     {
		     auto path = "data/" + sprite_pair.first;
		     fw().data->writeImage(path, sprite_pair.second);
	     }
     }},
    {"battle_bullet_sprites",
     [](const InitialGameStateExtractor &e) {
	     auto bullet_sprites = e.extractBulletSpritesBattle();

	     for (auto &sprite_pair : bullet_sprites)
	     {
		     auto path = "data/" + sprite_pair.first;
		     fw().data->writeImage(path, sprite_pair.second,
		                           fw().data->loadPalette("xcom3/tacdata/tactical.pal"));
	     }
     }},
    {"unit_image_packs",
     [](const InitialGameStateExtractor &e) {
	     for (auto &imagePackStrings : e.unitImagePackPaths)
	     {
		     GameState s;
		     LogInfo("Extracting image pack \"%s\"", imagePackStrings.first);

		     auto imagePack = e.extractImagePack(s, imagePackStrings.second, false);
		     if (!imagePack)
		     {
			     LogError("Failed to extract image pack \"%s\"", imagePackStrings.first);
		     }
		     else
		     {
			     if (!imagePack->saveImagePack(BattleUnitImagePack::getImagePackPath() + "/" +
			                                       imagePackStrings.first,
			                                   true))
			     {
				     LogError("Failed to save image pack \"%s\"", imagePackStrings.first);
			     }
		     }
	     }
     }},
    {"item_image_packs",
     [](const InitialGameStateExtractor &e) {
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
			             format("%s%s%d", BattleUnitImagePack::getImagePackPath(), "/item", i),
			             true))
			     {
				     LogError("Failed to save  item image pack \"%d\"", i);
			     }
		     }
	     }
     }},
    {"unit_shadow_packs",
     [](const InitialGameStateExtractor &e) {
	     for (auto &imagePackStrings : e.unitShadowPackPaths)
	     {
		     GameState s;
		     LogInfo("Extracting image pack \"%s\"", imagePackStrings.first);

		     auto imagePack = e.extractImagePack(s, imagePackStrings.second, true);
		     if (!imagePack)
		     {
			     LogError("Failed to extract image pack \"%s\"", imagePackStrings.first);
		     }
		     else
		     {
			     if (!imagePack->saveImagePack(BattleUnitImagePack::getImagePackPath() + "/" +
			                                       imagePackStrings.first,
			                                   true))
			     {
				     LogError("Failed to save image pack \"%s\"", imagePackStrings.first);
			     }
		     }
	     }
     }},
    {"unit_animation_packs",
     [](const InitialGameStateExtractor &e) {
	     for (auto &animationPackStrings : e.unitAnimationPackPaths)
	     {
		     GameState s;
		     LogInfo("Extracting animation pack \"%s\"", animationPackStrings.first);

		     auto animationPack =
		         e.extractAnimationPack(s, animationPackStrings.second, animationPackStrings.first);
		     if (!animationPack)
		     {
			     LogError("Failed to extract animation pack \"%s\"", animationPackStrings.first);
		     }
		     else
		     {
			     if (!animationPack->saveAnimationPack(
			             BattleUnitAnimationPack::getAnimationPackPath() + "/" +
			                 animationPackStrings.first,
			             true))
			     {
				     LogError("Failed to save animation pack \"%s\"", animationPackStrings.first);
			     }
		     }
	     }
     }},

    {"battle_map_tilesets",
     [](const InitialGameStateExtractor &e) {
	     for (auto &tileSetName : e.battleMapPaths)
	     {
		     // Some indices are empty?
		     if (tileSetName.empty())
			     continue;
		     GameState s;
		     LogInfo("Extracting tileset \"%s\"", tileSetName);

		     auto tileSet = e.extractTileSet(s, tileSetName);
		     if (!tileSet)
		     {
			     LogError("Failed to extract tileset \"%s\"", tileSetName);
		     }
		     else
		     {
			     if (!tileSet->saveTileset(BattleMapTileset::getTilesetPath() + "/" + tileSetName,
			                               true))
			     {
				     LogError("Failed to save tileset \"%s\"", tileSetName);
			     }
		     }
	     }
     }},
    {"battle_map_sectors",
     [](const InitialGameStateExtractor &e) {
	     for (auto &mapName : e.battleMapPaths)
	     {
		     // Some indices are empty?
		     if (mapName.empty())
			     continue;
		     GameState s;
		     LogInfo("Extracting map sectors from \"%s\"", mapName);

		     auto sectors = e.extractMapSectors(s, mapName);
		     LogInfo("Extracted %u sectors from \"%s\"", (unsigned)sectors.size(), mapName);
		     if (sectors.empty())
		     {
			     LogError("Failed to sectors from map \"%s\"", mapName);
		     }
		     for (auto &sectorPair : sectors)
		     {
			     auto &sectorName = sectorPair.first;
			     auto &sector = sectorPair.second;
			     auto path = BattleMapSectorTiles::getMapSectorPath();
			     if (!sector->saveSector(path + "/" + sectorName, true))
			     {
				     LogError("Failed to save map sector \"%s\"", sectorName);
			     }
		     }
	     }
     }},
};

int main(int argc, const char *argv[])
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
		auto list = extractListString.split(",");
		for (auto &extractorName : list)
		{
			auto extractor = thingsToExtract.find(extractorName);
			if (extractor == thingsToExtract.end())
			{
				LogError("Unknown extractor %s", extractorName);
				return EXIT_FAILURE;
			}
			else
			{
				extractorsToRun.push_back(*extractor);
			}
		}
	}
	TraceObj mainTrace("main");
	Framework fw(UString(argv[0]), false);
	InitialGameStateExtractor initialGameStateExtractor;
	for (auto &ePair : extractorsToRun)
	{
		LogWarning("Running %s", ePair.first);
		TraceObj exTrace(ePair.first);
		ePair.second(initialGameStateExtractor);
	}

	return 0;
}
