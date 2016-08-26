#include "tools/extractors/extractors.h"

namespace OpenApoc
{

void InitialGameStateExtractor::extract(GameState &state, Difficulty difficulty)
{
	this->extractOrganisations(state, difficulty);
	this->extractVehicleEquipment(state, difficulty);
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

	this->extractBattlescapeMap(state, "", "");
	this->extractBattlescapeStuff(state, "", "");
}

} // namespace OpenApoc
