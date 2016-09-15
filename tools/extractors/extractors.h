#pragma once
#include "game/state/gamestate.h"
#include "library/strings.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/common/ufo2p.h"

namespace OpenApoc
{

class InitialGameStateExtractor
{
  private:
	UFO2P ufo2p;
	TACP tacp;

  public:
	enum class Difficulty
	{
		DIFFICULTY_1,
		DIFFICULTY_2,
		DIFFICULTY_3,
		DIFFICULTY_4,
		DIFFICULTY_5,
	};
	InitialGameStateExtractor() = default;
	void extract(GameState &state, Difficulty difficulty);
	/* extractBulletSprites() returns a list of images, so doesn't affect a GameState */
	std::map<UString, sp<Image>> extractBulletSpritesCity();
	std::map<UString, sp<Image>> extractBulletSpritesBattle();

	// Though this takes a gamestate, that's just used to hang StateRef<>s off
	sp<BattleMapTileset> extractTileSet(GameState &state, const UString &name);
	sp<BattleUnitImagePack> extractImagePack(GameState &state, const UString &path, bool shadow);
	sp<BattleUnitAnimationPack> extractAnimationPack(GameState &state, const UString &path, const UString &name);
	std::map<UString, up<BattleMapSectorTiles>> extractMapSectors(GameState &state,
	                                                              const UString &mapRootName);

	// Lookup table of building function number -> battlemap path
	static const std::vector<UString> battleMapPaths;
	// List of paths and names for unit image packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitImagePackPaths;
	// List of paths and names for unit shadow packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitShadowPackPaths;
	// List of paths and names for animation packs
	static const std::map<OpenApoc::UString, OpenApoc::UString> unitAnimationPackPaths;
	
  private:
	void extractVehicles(GameState &state, Difficulty difficulty);
	void extractOrganisations(GameState &state, Difficulty difficulty);
	void extractFacilities(GameState &state, Difficulty difficulty);
	void extractBaseLayouts(GameState &state, Difficulty difficulty);
	void extractVehicleEquipment(GameState &state, Difficulty difficulty);
	void extractAgentEquipment(GameState &state, Difficulty difficulty);
	void extractAgentTypes(GameState &state, Difficulty difficulty);
	void extractResearch(GameState &state, Difficulty difficulty);

	void extractBuildings(GameState &state, UString bldFileName, sp<City> city,
	                      bool alienBuilding = false);
	void extractCityMap(GameState &state, UString fileName, UString tilePrefix, sp<City> city);
	void extractCityScenery(GameState &state, UString tilePrefix, UString datFile,
	                        UString spriteFile, UString stratmapFile, UString lofFile,
	                        UString ovrFile, sp<City> city);

	void extractBattlescapeMap(GameState &state, const std::vector<OpenApoc::UString> &paths);
	void extractBattlescapeMapFromPath(GameState &state, const UString dirName, const int index);
};
}
