#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extract(GameState &state, Difficulty difficulty)
{
	// clang-format off
	static const std::vector<OpenApoc::UString> battle_map_paths = {
		{ "01SENATE" },
		{ "02POLICE" },
		{ "03HOSPIT" },
		{ "04SCHOOL" },
		{ "05RESCUE" },
		{ "06OFFICE" },
		{ "07CORPHQ" },
		{ "08PORT" },
		{ "50SENSO" },
		{ "10ASTRO" },
		{ "11PARK" },
		{ "12SHOPS" },
		{ "" },// 13 car park, not present in vanilla
		{ "14ACNORM" },
		{ "15ACPOSH" },
		{ "" },// 16 hotel, not present in vanilla
		{ "" },// 17 atmo, not present in vanilla
		{ "18HYDRO" },
		{ "19SEWAGE" },
		{ "20WATER" },
		{ "21APPL" },
		{ "22ARMS" },
		{ "23ROBOTS" },
		{ "" },// 24 car fac, not present in vanilla (uses 25 FLYER)
		{ "25FLYER" },
		{ "26LFLYER" },
		{ "27CONSTR" },
		{ "28SLUMS" },
		{ "" },// 29 ruins, not present in vanilla
		{ "30WARE" },
		{ "" },// 31 spaceship, not present in vanilla
		{ "32POWER" },
		{ "33RECYCL" },
		{ "" },// 34 outdoor park, not present in vanilla
		{ "35TUBES" },
		{ "36CHURCH" },
		{ "37BASE" },
		{ "" },// 38 UFOs, not present in vanilla
		{ "39INCUB" },
		{ "40SPAWN" },
		{ "41FOOD" },
		{ "42MEGAPD" },
		{ "43SLEEP" },
		{ "44ORGAN" },
		{ "45FARM" },
		{ "46CONTRL" },
		{ "47MAINT" },
		{ "48GATE" },
		{ "" },// 49, not present in vanilla
		{ "" },// 50, not present in vanilla
		{ "51UFO1" },
		{ "52UFO2" },
		{ "53UFO3" },
		{ "54UFO4" },
		{ "55UFO5" },
		{ "56UFO6" },
		{ "57UFO7" },
		{ "58UFO8" },
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
