#pragma once
#include "game/gamestate.h"
#include "library/strings.h"

#include "tools/extractors/common/ufo2p.h"

namespace OpenApoc
{

class InitialGameStateExtractor
{
  private:
	UFO2P ufo2p;

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

  private:
	void extractVehicles(GameState &state, Difficulty difficulty);
	void extractOrganisations(GameState &state, Difficulty difficulty);
	void extractFacilities(GameState &state, Difficulty difficulty);
	void extractBaseLayouts(GameState &state, Difficulty difficulty);
	void extractVehicleEquipment(GameState &state, Difficulty difficulty);

	void extractBuildings(GameState &state, UString bldFileName, sp<City> city);
	void extractCityMap(GameState &state, UString fileName, UString tilePrefix, sp<City> city);
	void extractCityScenery(GameState &state, UString tilePrefix, UString datFile,
	                        UString spriteFile, UString stratmapFile, UString lofFile,
	                        UString ovrFile, sp<City> city);
};
}
