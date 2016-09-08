#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extract(GameState &state, Difficulty difficulty)
{
	// clang-format off
	static const std::vector<OpenApoc::UString> battle_map_paths = {
		{ "01senate" },
		{ "02police" },
		{ "03hospit" },
		{ "04school" },
		{ "05rescue" },
		{ "06office" },
		{ "07corphq" },
		{ "08port" },
		{ "50senso" },
		{ "10astro" },
		{ "11park" },
		{ "12shops" },
		{ "" },// 13 car park, not present in vanilla
		{ "14acnorm" },
		{ "15acposh" },
		{ "" },// 16 hotel, not present in vanilla
		{ "" },// 17 atmo, not present in vanilla
		{ "18hydro" },
		{ "19sewage" },
		{ "20water" },
		{ "21appl" },
		{ "22arms" },
		{ "23robots" },
		{ "" },// 24 car fac, not present in vanilla (uses 25 FLYER)
		{ "25flyer" },
		{ "26lflyer" },
		{ "27constr" },
		{ "28slums" },
		{ "" },// 29 ruins, not present in vanilla
		{ "30ware" },
		{ "" },// 31 spaceship, not present in vanilla
		{ "32power" },
		{ "33recycl" },
		{ "" },// 34 outdoor park, not present in vanilla
		{ "35tubes" },
		{ "36church" },
		{ "37base" },
		{ "" },// 38 UFOs, not present in vanilla
		{ "39incub" },
		{ "40spawn" },
		{ "41food" },
		{ "42megapd" },
		{ "43sleep" },
		{ "44organ" },
		{ "45farm" },
		{ "46contrl" },
		{ "47maint" },
		{ "48gate" },
		{ "" },// 49, not present in vanilla
		{ "" },// 50, not present in vanilla
		{ "51ufo1" },
		{ "52ufo2" },
		{ "53ufo3" },
		{ "54ufo4" },
		{ "55ufo5" },
		{ "56ufo6" },
		{ "57ufo7" },
		{ "58ufo8" },
	};
	// clang-format on

	this->extractOrganisations(state, difficulty);
	this->extractVehicleEquipment(state, difficulty);
	this->extractAgentEquipment(state, difficulty);
	this->extractAgentTypes(state, difficulty);
	this->extractVehicles(state, difficulty);
	this->extractFacilities(state, difficulty);
	this->extractBaseLayouts(state, difficulty);
	this->extractResearch(state, difficulty);

	std::map<Difficulty, UString> humanMapNames = {
	    {Difficulty::DIFFICULTY_1, "citymap1"}, {Difficulty::DIFFICULTY_2, "citymap2"},
	    {Difficulty::DIFFICULTY_3, "citymap3"}, {Difficulty::DIFFICULTY_4, "citymap4"},
	    {Difficulty::DIFFICULTY_5, "citymap5"},
	};

	UString humanMapId = City::getPrefix() + "HUMAN";
	UString alienMapId = City::getPrefix() + "ALIEN";

	state.cities[humanMapId] = std::make_shared<City>();
	state.cities[alienMapId] = std::make_shared<City>();

	this->extractBuildings(state, humanMapNames[difficulty], state.cities[humanMapId]);
	this->extractBuildings(state, "albuild", state.cities[alienMapId], true);

	this->extractCityMap(state, humanMapNames[difficulty], "CITYMAP_", state.cities[humanMapId]);
	this->extractCityMap(state, "alienmap", "ALIENMAP_", state.cities[alienMapId]);

	this->extractCityScenery(state, "CITYMAP_", "citymap", "city", "stratmap", "loftemps",
	                         "cityovr", state.cities[humanMapId]);
	this->extractCityScenery(state, "ALIENMAP_", "alienmap", "alien", "stratmap", "loftemps",
	                         "cityovr", state.cities[humanMapId]);

	this->extractBattlescapeMap(state, battle_map_paths);
	this->extractBattlescapeMapParts(state, battle_map_paths);
}

} // namespace OpenApoc
