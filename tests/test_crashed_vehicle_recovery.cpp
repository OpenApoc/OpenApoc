// Headless harness for issues #983 / #1008: crashed vehicles are recovered at a rate that
// depends on real-world game speed, because Organisation::updateMissions dispatches at most
// one rescue mission per organisation per call, while GameState::update() is called once per
// render frame regardless of speed. This drives the same one-hour game-time window through
// updateTurbo() (few, large-tick calls) and through update(1) (many, small-tick calls matching
// normal-speed cadence) and asserts both dispatch paths recover the same number of vehicles.
//
// Not registered as a ctest - it is a standalone harness, built as its own ninja target so it
// does not change the ctest count on this branch.
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/gametime.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "library/strings.h"
#include <iostream>
#include <vector>

using namespace OpenApoc;

namespace
{

// A non-player, non-alien organisation is required: Organisation::updateMissions is a no-op
// for the player org, and alien-owned targets have separate storming rules we don't want here.
StateRef<Organisation> findTestOrg(sp<GameState> state)
{
	for (auto &o : state->organisations)
	{
		if (o.first == state->getPlayer().id || o.first == state->getAliens().id ||
		    o.first == state->getCivilian().id)
		{
			continue;
		}
		return {state.get(), o.first};
	}
	return {};
}

// Any non-Road, rescue-capable type works for the rescuer; re-used for victims too since the
// target side of canRecoverVehicle only cares about crashed/dead/carried state, not type.
StateRef<VehicleType> findRescueType(sp<GameState> state)
{
	for (auto &t : state->vehicle_types)
	{
		if (t.second->canRescueCrashed && t.second->type != VehicleType::Type::Road)
		{
			return {state.get(), t.first};
		}
	}
	return {};
}

// Any building with a landing pad is a valid RecoverVehicle destination; used only as the
// target for the post-pickup carry-home leg, deliberately kept separate from where craft spawn
// (see findOpenSkyTile) so the pickup and the delivery are not the same tile.
bool findHomeBuilding(sp<GameState> state, StateRef<Building> &outBuilding)
{
	for (auto &b : state->current_city->buildings)
	{
		if (!b->landingPadLocations.empty())
		{
			outBuilding = b;
			return true;
		}
	}
	LogError("No building with a landing pad found");
	return false;
}

// Co-locating every rescuer and victim on one open sky tile removes flight time as a variable
// for the pickup itself: a RecoverVehicle mission starting with rescuer and target already at
// the same position resolves on the very next mission update instead of after a multi-tile
// chase, so what the test measures is whether dispatch happened at all before the target's own
// hour-long SelfDestruct timer expired - not how fast a craft happens to fly.
Vec3<int> findOpenSkyTile(sp<GameState> state)
{
	auto &map = *state->current_city->map;
	Vec3<int> centre = {map.size.x / 2, map.size.y / 2, 0};
	for (int z = map.size.z - 1; z >= 0; z--)
	{
		Vec3<int> candidate = {centre.x, centre.y, z};
		auto tile = map.getTile(candidate);
		if (!tile)
		{
			continue;
		}
		bool blocked = false;
		for (auto &obj : tile->ownedObjects)
		{
			if (obj->getType() == TileObject::Type::Scenery)
			{
				auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(obj);
				if (sceneryTile->scenery.lock()->type->isBuildingPart)
				{
					blocked = true;
					break;
				}
			}
		}
		if (!blocked)
		{
			return candidate;
		}
	}
	LogError("No open sky tile found");
	return {-1, -1, -1};
}

sp<GameState> loadFreshState(const UString &commonPath, const UString &gamestatePath)
{
	auto state = mksp<GameState>();
	if (!state->loadGame(commonPath))
	{
		LogError("Failed to load common gamestate");
		return nullptr;
	}
	if (!state->loadGame(gamestatePath))
	{
		LogError("Failed to load supplied gamestate");
		return nullptr;
	}
	state->startGame();
	state->initState();
	// setCurrentCity() only happens inside fillPlayerStartingProperty(); without it
	// current_city stays an unresolved StateRef and any city/building/map lookup segfaults.
	state->fillPlayerStartingProperty();

	// Strip pre-existing city traffic (including the player property just created above) so
	// per-tick cost is dominated by our own test craft rather than the hundreds of NPC vehicles
	// a fresh save otherwise contains, and so no leftover AttackBuilding/AttackVehicle mission
	// can permanently block canTurbo().
	state->vehicles.clear();
	state->agents.clear();

	return state;
}

struct ScenarioResult
{
	int recovered = 0;
	int selfDestructed = 0;
	int unresolved = 0;
	unsigned dispatchCalls = 0;
	unsigned elapsedTicks = 0;
	bool aborted = false;
};

ScenarioResult runScenario(sp<GameState> state, int numRescuers, int numVictims, bool turbo)
{
	ScenarioResult result;

	auto org = findTestOrg(state);
	auto craftType = findRescueType(state);
	StateRef<Building> homeBuilding;
	bool haveBuilding = findHomeBuilding(state, homeBuilding);
	auto spawnPos = findOpenSkyTile(state);

	if (!org || !craftType || !haveBuilding || spawnPos.x < 0)
	{
		LogError("Scenario setup failed: org={0} craftType={1} building={2} spawnPos valid={3}",
		         (bool)org, (bool)craftType, haveBuilding, spawnPos.x >= 0);
		result.unresolved = numVictims;
		return result;
	}
	// Force the test org friendly to the player: canTurbo() refuses to run while any
	// non-crashed, aggressive vehicle is owned by an org Hostile to the player (a real,
	// separate stuck-turbo bug - see the canTurbo() writeup), and that is not what this
	// harness is exercising. Pin the relation so only our own dispatch-throughput fix is
	// under test.
	org->current_relations[state->getPlayer()] = 100.0f;

	Vec3<float> spawnPosF = {(float)spawnPos.x + 0.5f, (float)spawnPos.y + 0.5f,
	                         (float)spawnPos.z + 0.5f};

	for (int i = 0; i < numRescuers; i++)
	{
		state->current_city->placeVehicle(*state, craftType, org, spawnPosF, 0.0f);
	}

	// Hold the actual sp<Vehicle>, not just its id: a recovered-and-delivered or
	// self-destructed vehicle can be erased from state->vehicles entirely (delivery folds it
	// into the destination building via processRecoveredVehicle; death runs through
	// cleanUpDeathNote), and re-resolving a StateRef by id afterwards would then just report
	// "not found" for both outcomes alike. isDead() (health <= 0, set by Vehicle::die()) on our
	// own retained handle is unaffected by map membership and tells the two apart correctly.
	std::vector<sp<Vehicle>> victims;
	for (int i = 0; i < numVictims; i++)
	{
		auto v = state->current_city->placeVehicle(*state, craftType, org, spawnPosF, 0.0f);
		v->homeBuilding = homeBuilding;
		v->crash(*state, {});
		victims.push_back(v);
	}

	// +1 tick past the hour: SelfDestruct's isFinished() is checked at the *start* of a
	// mission update, so a victim whose timer hits zero on the very last tick of the window
	// only actually dies on the tick after - without this the window would silently make
	// every undispatched victim look "recovered" by exiting one tick too early.
	const unsigned totalTicks = TICKS_PER_HOUR + 1;
	if (turbo)
	{
		unsigned elapsed = 0;
		while (elapsed < totalTicks)
		{
			if (!state->canTurbo())
			{
				LogError("canTurbo() false mid-scenario at tick {0}", elapsed);
				result.aborted = true;
				break;
			}
			auto before = state->gameTime.getTicks();
			state->updateTurbo();
			elapsed += state->gameTime.getTicks() - before;
			result.dispatchCalls++;
		}
		result.elapsedTicks = elapsed;
	}
	else
	{
		for (unsigned t = 0; t < totalTicks; t++)
		{
			state->update(1);
			result.dispatchCalls++;
		}
		result.elapsedTicks = totalTicks;
	}

	for (auto &v : victims)
	{
		if (v->isDead())
		{
			result.selfDestructed++;
		}
		else
		{
			result.recovered++;
		}
	}

	return result;
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");
	config().addPositionalArgument("rescuers", "Number of rescue-capable craft");
	config().addPositionalArgument("victims", "Number of simultaneously crashed victims");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto commonPath = config().getString("common");
	auto gamestatePath = config().getString("gamestate");
	if (commonPath.empty() || gamestatePath.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}
	int numRescuers = config().getString("rescuers").empty()
	                      ? 2
	                      : Strings::toInteger(config().getString("rescuers"));
	int numVictims = config().getString("victims").empty()
	                     ? 8
	                     : Strings::toInteger(config().getString("victims"));

	Framework fw("OpenApoc", false);

	LogWarning("=== Scenario: turbo path, {0} rescuers, {1} victims, 1h window ===", numRescuers,
	           numVictims);
	auto turboState = loadFreshState(commonPath, gamestatePath);
	if (!turboState)
	{
		return EXIT_FAILURE;
	}
	auto turboResult = runScenario(turboState, numRescuers, numVictims, true);
	LogWarning("turbo: {0} updateTurbo() calls, {1} ticks elapsed, recovered={2} "
	           "selfDestructed={3} unresolved={4} aborted={5}",
	           turboResult.dispatchCalls, turboResult.elapsedTicks, turboResult.recovered,
	           turboResult.selfDestructed, turboResult.unresolved, turboResult.aborted);
	if (turboResult.aborted)
	{
		std::cout << "ABORTED: turbo path could not run to completion (canTurbo() went false)\n";
		return EXIT_FAILURE;
	}

	LogWarning("=== Scenario: normal-cadence path, {0} rescuers, {1} victims, 1h window ===",
	           numRescuers, numVictims);
	auto normalState = loadFreshState(commonPath, gamestatePath);
	if (!normalState)
	{
		return EXIT_FAILURE;
	}
	auto normalResult = runScenario(normalState, numRescuers, numVictims, false);
	LogWarning("normal: {0} update(1) calls, recovered={1} selfDestructed={2} unresolved={3}",
	           normalResult.dispatchCalls, normalResult.recovered, normalResult.selfDestructed,
	           normalResult.unresolved);

	std::cout << "RESULT turbo_recovered=" << turboResult.recovered
	          << " turbo_selfDestructed=" << turboResult.selfDestructed
	          << " normal_recovered=" << normalResult.recovered
	          << " normal_selfDestructed=" << normalResult.selfDestructed << "\n";

	if (turboResult.recovered != normalResult.recovered)
	{
		std::cout << "MISMATCH: turbo and normal-cadence recovery rates differ\n";
		return EXIT_FAILURE;
	}

	std::cout << "MATCH: turbo and normal-cadence recovery rates are equal\n";
	return EXIT_SUCCESS;
}
