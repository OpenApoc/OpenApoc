#include "tools/extractors/extractors.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/tilemap/tilemap.h"

namespace OpenApoc
{ // clang-format off

const std::vector<OpenApoc::UString> InitialGameStateExtractor::battleMapPaths = {
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

const std::map<OpenApoc::UString, std::set<int>> InitialGameStateExtractor::initialFires = {
	{ "51ufo1",{ 25 } },
	{ "52ufo2",{ 25 } },
	{ "53ufo3",{ 25 } },
	{ "54ufo4",{ 25 } },
	{ "55ufo5",{ 25 } },
	{ "56ufo6",{ 25 } },
	{ "57ufo7",{ 25 } },
	{ "58ufo8",{ 25 } },
};

// On a second look, it seems those are just unused, 
// and the game never spawns smoke, just fire, which then spawns smoke adjacent to it
const std::map<OpenApoc::UString, std::set<int>> InitialGameStateExtractor::initialSmokes = {
	//{ "51ufo1",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
	//{ "52ufo2",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
	//{ "53ufo3",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
	//{ "54ufo4",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
	//{ "55ufo5",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
	//{ "56ufo6",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
	//{ "57ufo7",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
	//{ "58ufo8",{ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42 } },
};

const std::map<OpenApoc::UString, std::set<int>> InitialGameStateExtractor::reinforcementSpawners = {
	{ "39incub"	,{ 208,209,210,211,212,213,214,215 } },
	{ "40spawn"	,{ 208,209,210,211,212,213,214,215 } },
	{ "41food"	,{ 208,209,210,211,212,213,214,215 } },
	{ "42megapd",{ 208,209,210,211,212,213,214,215 } },
	{ "43sleep" ,{ 208,209,210,211,212,213,214,215 } },
	{ "44organ" ,{ 208,209,210,211,212,213,214,215 } },
	{ "45farm" ,{ 208,209,210,211,212,213,214,215 } },
	{ "46contrl" ,{ 208,209,210,211,212,213,214,215 } },
	{ "47maint",{ 208,209,210,211,212,213,214,215 } },
	{ "48gate" ,{ 208,209,210,211,212,213,214,215 } },
};

const std::map<OpenApoc::UString, int> InitialGameStateExtractor::reinforcementTimers = {
	{ "39incub"	,{ 20 } },
	{ "40spawn"	,{ 18 } },
	{ "41food"	,{ 35 } },
	{ "42megapd",{ 10 } },
	{ "43sleep" ,{ 40 } },
	{ "44organ" ,{ 20 } },
	{ "45farm"  ,{ 12 } },
	{ "46contrl",{ 10 } },
	{ "47maint" ,{ 30 } },
	{ "48gate"  ,{ 5  } },
};

const std::map<OpenApoc::UString, std::set<int>> InitialGameStateExtractor::missionObjectives = {
	{ "39incub"	,{ 139,140,141,142,143,144,145,146} },
	{ "40spawn"	,{ } },
	{ "41food"	,{ 140 } },
	{ "42megapd",{ 204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229, 230} },
	{ "43sleep" ,{ 180,181,183} },
	{ "44organ" ,{ 137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158} },
	{ "45farm" , { 180,181,182 } },
	{ "46contrl",{ 155,156,157,158} },
	{ "47maint" ,{ 142,143,144,145} },
	{ "48gate"  ,{ 71,73,74,75,76,77,78,79,80,81,82,83,84,85} },
};

const std::map<OpenApoc::UString, std::vector<int>> InitialGameStateExtractor::tubes = {
	/*{ "CITYTILE_CITYMAP_63",{ 0,1,0,1 } },
	{ "CITYTILE_CITYMAP_64",{ 1,0,1,0 } },
	{ "CITYTILE_CITYMAP_65",{ 0,1,0,1 } },
	{ "CITYTILE_CITYMAP_66",{ 1,0,1,0 } },
	{ "CITYTILE_CITYMAP_67",{ 1,0,0,1 } },
	{ "CITYTILE_CITYMAP_68",{ 0,1,1,0 } },
	{ "CITYTILE_CITYMAP_69",{ 0,0,1,1 } },
	{ "CITYTILE_CITYMAP_70",{ 1,1,0,0 } },
	{ "CITYTILE_CITYMAP_71",{ 0,0,0,1 } },*/
	{ "CITYTILE_CITYMAP_72",{ 1,0,1,0 } },
	{ "CITYTILE_CITYMAP_73",{ 0,1,0,1 } },
	{ "CITYTILE_CITYMAP_74",{ 0,0,0,0 } },
	{ "CITYTILE_CITYMAP_75",{ 0,1,0,0 } },
	{ "CITYTILE_CITYMAP_76",{ 1,0,0,0 } },
	{ "CITYTILE_CITYMAP_77",{ 0,0,1,0 } },
	{ "CITYTILE_CITYMAP_78",{ 0,0,0,1 } },
	{ "CITYTILE_CITYMAP_79",{ 0,1,0,0 } },
	{ "CITYTILE_CITYMAP_80",{ 1,0,0,0 } },
	{ "CITYTILE_CITYMAP_81",{ 0,0,1,0 } },
	{ "CITYTILE_CITYMAP_82",{ 0,0,0,1 } },
	{ "CITYTILE_CITYMAP_83",{ 0,0,1,1 } },
	{ "CITYTILE_CITYMAP_84",{ 1,1,0,0 } },
	{ "CITYTILE_CITYMAP_85",{ 0,1,1,0 } },
	{ "CITYTILE_CITYMAP_86",{ 1,0,0,1 } },
	{ "CITYTILE_CITYMAP_87",{ 0,1,0,1 } },
	{ "CITYTILE_CITYMAP_88",{ 1,1,1,1 } },
	{ "CITYTILE_CITYMAP_89",{ 1,1,1,1 } },
	{ "CITYTILE_CITYMAP_90",{ 1,1,1,1 } },
};

const std::map<OpenApoc::UString, OpenApoc::UString> InitialGameStateExtractor::unitImagePackPaths = {
	{ "antrpa",		"unit/antrpa" },
	{ "antrpb",		"unit/antrpb" },
	{ "antrpc",		"unit/antrpc" },
	{ "antrpd",		"unit/antrpd" },
	{ "antrpe",		"unit/antrpe" },
	{ "culta",		"unit/culta" },
	{ "cultb",		"unit/cultb" },
	{ "cultc",		"unit/cultc" },
	{ "cultd",		"unit/cultd" },
	{ "culte",		"unit/culte" },
	{ "cultla",		"unit/cultla" },
	{ "cultlb",		"unit/cultlb" },
	{ "cultlc",		"unit/cultlc" },
	{ "cultld",		"unit/cultld" },
	{ "cultle",		"unit/cultle" },
	{ "gang2a",		"unit/gang2a" },
	{ "gang2b",		"unit/gang2b" },
	{ "gang2c",		"unit/gang2c" },
	{ "gang2d",		"unit/gang2d" },
	{ "gang2e",		"unit/gang2e" },
	{ "ganga",		"unit/ganga" },
	{ "gangb",		"unit/gangb" },
	{ "gangc",		"unit/gangc" },
	{ "gangd",		"unit/gangd" },
	{ "gange",		"unit/gange" },
	{ "gangla",		"unit/gangla" },
	{ "ganglb",		"unit/ganglb" },
	{ "ganglc",		"unit/ganglc" },
	{ "gangld",		"unit/gangld" },
	{ "gangle",		"unit/gangle" },
	{ "polica",		"unit/polica" },
	{ "policb",		"unit/policb" },
	{ "policc",		"unit/policc" },
	{ "policd",		"unit/policd" },
	{ "police",		"unit/police" },
	{ "seca",		"unit/seca" },
	{ "secb",		"unit/secb" },
	{ "secc",		"unit/secc" },
	{ "secd",		"unit/secd" },
	{ "sece",		"unit/sece" },
	{ "skela",		"unit/skela" },
	{ "skelb",		"unit/skelb" },
	{ "skelc",		"unit/skelc" },
	{ "skeld",		"unit/skeld" },
	{ "skele",		"unit/skele" },
	{ "xcom1a",		"unit/xcom1a" },
	{ "xcom1b",		"unit/xcom1b" },
	{ "xcom1c",		"unit/xcom1c" },
	{ "xcom1d",		"unit/xcom1d" },
	{ "xcom1e",		"unit/xcom1e" },
	{ "xcom2a",		"unit/xcom2a" },
	{ "xcom2b",		"unit/xcom2b" },
	{ "xcom2c",		"unit/xcom2c" },
	{ "xcom2d",		"unit/xcom2d" },
	{ "xcom2e",		"unit/xcom2e" },
	{ "xcom3a",		"unit/xcom3a" },
	{ "xcom3b",		"unit/xcom3b" },
	{ "xcom3c",		"unit/xcom3c" },
	{ "xcom3d",		"unit/xcom3d" },
	{ "xcom3e",		"unit/xcom3e" },
	{ "xcom4a",		"unit/xcom4a" },
	{ "xcom4b",		"unit/xcom4b" },
	{ "xcom4c",		"unit/xcom4c" },
	{ "xcom4d",		"unit/xcom4d" },
	{ "xcom4e",		"unit/xcom4e" },
	{ "bsk",		"alien/bsk-" },
	{ "chrysa",		"alien/chrysa" },
	{ "chrysb",		"alien/chrysb" },
	{ "gun",		"alien/gun" },
	{ "hypr",		"alien/hypr-" },
	{ "megaa",		"alien/megaa" },
	{ "megab",		"alien/megab" },
	{ "megad",		"alien/megad" },
	{ "megae",		"alien/megae" },
	{ "micro",		"alien/micro-" },
	{ "multi",		"alien/multi-" },
	{ "mwegga",		"alien/mwegga" },
	{ "mweggb",		"alien/mweggb" },
	{ "popper",		"alien/popper-" },
	{ "psi",		"alien/psi-" },
	{ "queena",		"alien/queena" },
	{ "queenb",		"alien/queenb" },
	{ "spitr",		"alien/spitr" },
	{ "grey",		"civ/grey-" },
	{ "nm1",		"civ/nm1-" },
	{ "nm2",		"civ/nm2-" },
	{ "nm3",		"civ/nm3-" },
	{ "nw1",		"civ/nw1-" },
	{ "nw2",		"civ/nw2-" },
	{ "nw3",		"civ/nw3-" },
	{ "rm1",		"civ/rm1-" },
	{ "rm2",		"civ/rm2-" },
	{ "rm3",		"civ/rm3-" },
	{ "robo1",		"civ/robo1-" },
	{ "robo2",		"civ/robo2-" },
	{ "robo3",		"civ/robo3-" },
	{ "robo4",		"civ/robo4-" },
	{ "robot",		"civ/robot-" },
	{ "rw1",		"civ/rw1-" },
	{ "rw2",		"civ/rw2-" },
	{ "rw3",		"civ/rw3-" },
	{ "scntst",		"civ/scntst" },
	{ "sm1",		"civ/sm1-" },
	{ "sm2",		"civ/sm2-" },
	{ "sm3",		"civ/sm3-" },
	{ "sw1",		"civ/sw1-" },
	{ "sw2",		"civ/sw2-" },
	{ "sw3",		"civ/sw3-" },
};

const std::map<OpenApoc::UString, OpenApoc::UString> InitialGameStateExtractor::unitShadowPackPaths = {
	{ "shadow",		"unit/shadow" },
	{ "bsks",		"alien/bsks" },
	{ "hyprs",		"alien/hyprs" },
	{ "megas",		"alien/mega-s" },
	{ "poppers",	"alien/poppers" },
	{ "psis",		"alien/psi-s" },
	{ "spitrs",		"alien/spit-s" },
};

// Should not be changed. List of keys here is expected to be the same in 
// extract_unit_animation_packs.cpp 's InitialGameStateExtractor::extractAnimationPack
const std::map<OpenApoc::UString, OpenApoc::UString> InitialGameStateExtractor::unitAnimationPackPaths = {
	{ "unit",		"unit/anim" },
	{ "bsk",		"alien/bsk" },
	{ "chrys1",		"" }, // facing #1
	{ "chrys2",		"" }, // facing #2
	{ "gun",		"" },
	{ "hypr",		"alien/hypr" },
	{ "mega",		"alien/mega" },
	{ "micro",		"" },
	{ "multi",		"alien/multi" },
	{ "mwegg1",		"" }, // facing #1
	{ "mwegg2",		"" }, // facing #2
	{ "popper",		"" },
	{ "psi",		"alien/psi" },
	{ "queen",		"alien/queen", },
	{ "spitr",		"alien/spit" },
	{ "civ",		"civ/anim" },
};

const std::vector<int> InitialGameStateExtractor::buildingFunctionDetectionWeights = {
//	0	 1	  2	   3    4    5    6    7    8    9
	100, 155, 135, 110, 115, 80,  120, 130, 120, 90,  
	80,  80,  130, 100, 130, 130, 100, 70,  70,  70,  
	70,  90,  110, 90,  90,  90,  90,  90,  95,  100, 
	90,  100, 75,  85,  100, 100, 0,   0,   100, 100, 
	100, 100, 100, 100, 100, 100, 100, 100, 100
};

// clang-format on

void InitialGameStateExtractor::extractCommon(GameState &state) const
{
	this->extractOrganisations(state);
	this->extractVehicleEquipment(state);
	this->extractAgentBodyTypes(state);
	this->extractAgentTypes(state);
	this->extractVehicles(state);
	this->extractFacilities(state);
	this->extractBaseLayouts(state);
	this->extractResearch(state);
	this->extractAgentEquipment(state);
	this->extractDoodads(state);
	this->extractBuildingFunctions(state);
	this->extractEconomy(state);

	// The alien map doesn't change
	UString alienMapId = City::getPrefix() + "ALIEN";
	state.cities[alienMapId] = std::make_shared<City>();
	state.cities[alienMapId]->id = alienMapId;
	state.cities[alienMapId]->researchUnlock.emplace_back(&state,
	                                                      "RESEARCH_UNLOCK_ALIEN_DIMENSION");
	this->extractBuildings(state, "albuild", state.cities[alienMapId], true);
	this->extractCityMap(state, "alienmap", "ALIENMAP_", state.cities[alienMapId]);
	this->extractCityScenery(state, "ALIENMAP_", "alienmap", "alien", "alien_s", "loftemps",
	                         "cityovr", state.cities[alienMapId]);

	this->extractBattlescapeMap(state, battleMapPaths);
	this->extractSharedBattleResources(state);
	this->extractSharedCityResources(state);
}

void InitialGameStateExtractor::extract(GameState &state, Difficulty difficulty) const
{
	state.difficulty = (int)difficulty;

	this->extractAlienEquipmentSets(state, difficulty);

	std::map<Difficulty, UString> humanMapNames = {
	    {Difficulty::DIFFICULTY_1, "citymap1"}, {Difficulty::DIFFICULTY_2, "citymap2"},
	    {Difficulty::DIFFICULTY_3, "citymap3"}, {Difficulty::DIFFICULTY_4, "citymap4"},
	    {Difficulty::DIFFICULTY_5, "citymap5"},
	};

	UString humanMapId = City::getPrefix() + "HUMAN";

	state.cities[humanMapId] = std::make_shared<City>();
	state.cities[humanMapId]->id = humanMapId;
	state.cities[humanMapId]->civilianSalary = 65; // this is default value in UFO2P code
	state.cities[humanMapId]->researchUnlock.emplace_back(&state,
	                                                      "RESEARCH_UNLOCK_DIMENSION_GATES");

	this->extractBuildings(state, humanMapNames[difficulty], state.cities[humanMapId]);

	this->extractCityMap(state, humanMapNames[difficulty], "CITYMAP_", state.cities[humanMapId]);

	this->extractCityScenery(state, "CITYMAP_", "citymap", "city", "stratmap", "loftemps",
	                         "cityovr", state.cities[humanMapId]);
}

} // namespace OpenApoc
