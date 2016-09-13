#include "framework/framework.h"
#include "framework/trace.h"
#include "tools/extractors/extractors.h"
#include <SDL_main.h>

using namespace OpenApoc;

int main(int argc, char *argv[])
{
	bool enable_trace = false;
	LogInfo("Starting OpenApoc_DataExtractor");
	std::vector<UString> cmdline;

	for (int i = 1; i < argc; i++)
	{
		// Special handling of tracing as we want it to be started before the framework
		// parses the rest of the options
		if (UString(argv[i]) == "--enable-tracing")
		{
			enable_trace = true;
			continue;
		}
		else if (UString(argv[i]) == "--disable-tracing")
		{
			enable_trace = false;
			continue;
		}
		cmdline.emplace_back(UString(argv[i]));
	}

	if (enable_trace)
	{
		Trace::enable();
		LogInfo("Tracing enabled");
	}

	{

		Trace::setThreadName("main");

		TraceObj obj("main");
		Framework *fw = new Framework(UString(argv[0]), cmdline, false);
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

		/*auto dpair = std::pair<UString,
		InitialGameStateExtractor::Difficulty>("data/difficulty5",
		InitialGameStateExtractor::Difficulty::DIFFICULTY_5);*/
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

	if (enable_trace)
	{
		Trace::disable();
	}

	return 0;
}
