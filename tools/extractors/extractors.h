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
	void extractBattlescapeMapParts(GameState &state, const std::vector<OpenApoc::UString> &paths);

	void readBattleMapParts(GameState &state, sp<BattleMapTileset> t, BattleMapPartType::Type type,
	                        const UString &idPrefix, const UString &dirName, const UString &datName,
	                        const UString &pckName, const UString &stratPckName);
	void extractBattlescapeMapPartsFromMap(GameState &state, const UString dirName,
	                                       const int index);
	void extractBattlescapeMapFromPath(GameState &state, const UString dirName, const int index);
};
}
