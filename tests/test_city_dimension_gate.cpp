#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/vehicletype.h"
#include <iostream>

using namespace OpenApoc;

// This reproduces the escape without going through the dimension-gate sound/portal machinery:
// it moves the target the same way enterDimensionGate()/leaveDimensionGate() do at the
// storage level (removeFromMap, reassign city, addObjectToMap on the other city's map), which is
// enough to give the target a live tileObject in a city that is no longer the pursuer's.
static bool test_attack_mission_ends_when_target_leaves_city(sp<GameState> state)
{
	LogInfo("Testing AttackVehicle mission ends when target leaves the pursuer's city...");

	StateRef<City> cityHuman{state.get(), UString("CITYMAP_HUMAN")};
	StateRef<City> cityAlien{state.get(), UString("CITYMAP_ALIEN")};
	if (!cityHuman || !cityAlien || !cityHuman->map || !cityAlien->map)
	{
		LogError("CITYMAP_HUMAN/CITYMAP_ALIEN not available with a built tile map");
		return false;
	}

	StateRef<VehicleType> pursuerType{state.get(), UString("VEHICLETYPE_VALKYRIE_INTERCEPTOR")};
	StateRef<VehicleType> targetType{state.get(), UString("VEHICLETYPE_ALIEN_ASSAULT_SHIP")};
	if (!pursuerType || !targetType)
	{
		LogError("Expected vehicle types missing");
		return false;
	}

	auto pursuer =
	    cityHuman->placeVehicle(*state, pursuerType, state->getPlayer(), {20, 20, 2}, 0.0f);
	auto target =
	    cityHuman->placeVehicle(*state, targetType, state->getAliens(), {30, 30, 2}, 0.0f);
	if (!pursuer || !target || !pursuer->tileObject || !target->tileObject)
	{
		LogError("Failed to place pursuer/target on CITYMAP_HUMAN");
		return false;
	}

	StateRef<Vehicle> targetRef{state.get(), target};
	pursuer->setMission(*state, VehicleMission::attackVehicle(*state, *pursuer, targetRef));
	if (pursuer->missions.empty())
	{
		LogError("AttackVehicle mission was not set");
		return false;
	}

	if (pursuer->missions.front().isFinished(*state, *pursuer))
	{
		LogError("Mission reported finished before the target ever left the city");
		return false;
	}

	// Move the target to the other city the way a dimension-gate transition does: gone from
	// one map, re-added live on the other, in a single atomic step.
	target->removeFromMap(*state);
	target->city = cityAlien;
	cityAlien->map->addObjectToMap(*state, target);
	if (!target->tileObject || target->city != cityAlien)
	{
		LogError("Failed to relocate target to CITYMAP_ALIEN");
		return false;
	}

	if (!pursuer->missions.front().isFinished(*state, *pursuer))
	{
		LogError("Mission still reports unfinished after the target left the city");
		return false;
	}

	if (!pursuer->popFinishedMissions(*state))
	{
		LogError("popFinishedMissions() did not pop the stale AttackVehicle mission");
		return false;
	}
	if (!pursuer->missions.empty())
	{
		LogError("Pursuer still carries a mission after the phantom AttackVehicle was popped");
		return false;
	}

	LogInfo("AttackVehicle dimension-gate test passed");
	return true;
}

int main(int argc, char **argv)
{
	OpenApoc::config().addPositionalArgument("common", "Common gamestate to load");
	OpenApoc::config().addPositionalArgument("gamestate", "Gamestate to load");

	if (OpenApoc::config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto gamestate_name = OpenApoc::config().getString("gamestate");
	auto common_name = OpenApoc::config().getString("common");
	if (common_name.empty() || gamestate_name.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		OpenApoc::config().showHelp();
		return EXIT_FAILURE;
	}

	OpenApoc::Framework fw("OpenApoc", false);

	LogInfo("Loading common gamestate \"{0}\"", common_name);
	auto state = OpenApoc::mksp<OpenApoc::GameState>();
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load common gamestate");
		return EXIT_FAILURE;
	}

	LogInfo("Loading gamestate \"{0}\"", gamestate_name);
	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load supplied gamestate");
		return EXIT_FAILURE;
	}

	state->startGame();
	state->initState();

	if (!test_attack_mission_ends_when_target_leaves_city(state))
	{
		LogError("AttackVehicle dimension-gate test failed");
		return EXIT_FAILURE;
	}

	LogInfo("test_city_dimension_gate success - all tests passed");
	return EXIT_SUCCESS;
}
