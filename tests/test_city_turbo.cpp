// GameState::canTurbo() (game/state/gamestate.cpp) has three independent blockers.
// This test exercises the two that turned out to be over-eager on real savegames:
//
//   - blocker (2): any live, in-city, aggressive vehicle owned by a Hostile
//     organisation used to lock out turbo forever, even when it was just an idle
//     home fleet holding no missions.
//   - blocker (3): any AttackVehicle/AttackBuilding mission used to lock out turbo
//     even after the engine's own liveness check (VehicleMission::isFinished) would
//     already consider it finished (e.g. the target moved to another city).
//
// Built on the base game's starting gamestate rather than a third-party savegame,
// so everything here is self-contained and safe to commit.
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/organisation.h"
#include <iostream>

using namespace OpenApoc;

namespace
{

// Clears every vehicle's mission list and any in-flight projectiles, so each
// scenario below starts from a known-clean canTurbo()==true baseline and only the
// fixture it sets up can affect the result.
void resetToBaseline(GameState &state)
{
	state.current_city->projectiles.clear();
	for (auto &v : state.vehicles)
	{
		v.second->missions.clear();
	}
}

// Registers a throwaway VehicleType under a unique id (VehicleType is a StateObject
// and so not copy-constructible - default-construct and set only the fields
// canTurbo()'s codepaths actually touch: a default Flying type with no equipment
// needs a positive health or Vehicle::isDead() reports it dead and canTurbo() ignores
// it entirely).
StateRef<VehicleType> makeTestVehicleType(GameState &state, const UString &id, float aggressiveness)
{
	auto type = mksp<VehicleType>();
	type->health = 100;
	type->aggressiveness = aggressiveness;
	// TileObjectVehicle::setPosition() looks up size by the closest entry in
	// voxelMaps (empty here, so it resolves to facing 0.0f) - without this entry
	// placing the vehicle on the map throws std::out_of_range.
	type->size[0.0f] = {1, 1, 1};
	state.vehicle_types[id] = type;
	return {&state, id};
}

// Registers a throwaway Organisation under a unique id, hostile to the player.
StateRef<Organisation> makeHostileOrg(GameState &state, const UString &id)
{
	auto org = mksp<Organisation>();
	org->name = id;
	state.organisations[id] = org;
	StateRef<Organisation> ref{&state, id};
	// -100 is safely under the "< -50" Hostile threshold in Organisation::isRelatedTo.
	ref->current_relations[state.getPlayer()] = -100.0f;
	return ref;
}

// Registers a throwaway Organisation under a unique id, neutral to the player (the
// default when no relation entry exists) - used where a fixture must NOT itself trip
// blocker (2), so only the behaviour under test can affect canTurbo().
StateRef<Organisation> makeNeutralOrg(GameState &state, const UString &id)
{
	auto org = mksp<Organisation>();
	org->name = id;
	state.organisations[id] = org;
	return {&state, id};
}

Vec3<float> testSpawnPosition(GameState &state, int offset)
{
	auto size = state.current_city->map->size;
	return {size.x / 2.0f + offset, size.y / 2.0f, size.z / 2.0f};
}

// Places a vehicle of the given type/owner directly on current_city's map, bypassing
// City::placeVehicle()'s equipDefaultEquipment() (which looks up default loadouts by
// type id and would fail for our synthetic types).
sp<Vehicle> spawnVehicle(GameState &state, StateRef<VehicleType> type, StateRef<Organisation> owner,
                         Vec3<float> position)
{
	auto city = state.current_city;
	auto v = city->createVehicle(state, type, owner);
	v->leaveBuilding(state, position);
	return v;
}

bool expectCanTurbo(GameState &state, bool expected, const UString &scenario)
{
	bool actual = state.canTurbo();
	if (actual != expected)
	{
		LogError("[{0}] expected canTurbo()=={1}, got {2}", scenario, expected, actual);
		return false;
	}
	LogInfo("[{0}] canTurbo()=={1} as expected", scenario, actual);
	return true;
}

// An idle hostile aggressive vehicle (missions.empty()) must NOT block turbo - it is
// just a parked home fleet, not something the player is being asked to react to.
bool test_idle_hostile_aggressive_does_not_block(sp<GameState> state)
{
	resetToBaseline(*state);
	if (!expectCanTurbo(*state, true, "baseline-before-idle-hostile"))
		return false;

	auto type = makeTestVehicleType(*state, "VEHICLETYPE_TEST_IDLE", 1.0f);
	auto org = makeHostileOrg(*state, "ORG_TEST_IDLE");
	auto v = spawnVehicle(*state, type, org, testSpawnPosition(*state, 0));
	(void)v; // missions left empty deliberately

	return expectCanTurbo(*state, true, "idle-hostile-aggressive-no-missions");
}

// The same hostile aggressive vehicle WITH any mission at all must still block turbo -
// it is now actually doing something, so the player should be made aware.
bool test_hostile_aggressive_with_mission_blocks(sp<GameState> state)
{
	resetToBaseline(*state);
	if (!expectCanTurbo(*state, true, "baseline-before-busy-hostile"))
		return false;

	auto type = makeTestVehicleType(*state, "VEHICLETYPE_TEST_BUSY", 1.0f);
	auto org = makeHostileOrg(*state, "ORG_TEST_BUSY");
	auto v = spawnVehicle(*state, type, org, testSpawnPosition(*state, 1));

	// Mission type/contents don't matter for blocker (2): a non-empty mission list is
	// the whole signal. Build a bare mission directly rather than going through a
	// factory that would try to pathfind against a specific target.
	VehicleMission mission;
	mission.type = VehicleMission::MissionType::GotoLocation;
	v->missions.push_back(mission);

	bool ok = expectCanTurbo(*state, false, "hostile-aggressive-with-mission");
	v->missions.clear();
	return ok;
}

// A live AttackVehicle mission against a target that is still on the map, in the
// current city, alive and uncrashed must block turbo - this is genuine ongoing
// combat.
bool test_live_attack_vehicle_mission_blocks(sp<GameState> state)
{
	resetToBaseline(*state);
	if (!expectCanTurbo(*state, true, "baseline-before-live-attack"))
		return false;

	// Non-aggressive/neutral so only blocker (3) - not blocker (2) - can be responsible
	// for the result.
	auto type = makeTestVehicleType(*state, "VEHICLETYPE_TEST_ATTACKER_LIVE", 0.0f);
	auto attackerOrg = makeNeutralOrg(*state, "ORG_TEST_ATTACKER_LIVE");
	auto targetOrg = makeNeutralOrg(*state, "ORG_TEST_TARGET_LIVE");

	auto attacker = spawnVehicle(*state, type, attackerOrg, testSpawnPosition(*state, 2));
	auto target = spawnVehicle(*state, type, targetOrg, testSpawnPosition(*state, 3));

	StateRef<Vehicle> targetRef{state.get(), target};
	attacker->missions.push_back(VehicleMission::attackVehicle(*state, *attacker, targetRef));

	bool ok = expectCanTurbo(*state, false, "live-attack-vehicle-mission");
	attacker->missions.clear();
	return ok;
}

// An AttackVehicle mission whose target is no longer on the current city's map (e.g.
// it moved to another city) must NOT block turbo - the engine's own
// isFinishedInternal() already considers this mission finished ("Target not on the
// map"), and gamestate.cpp's self-heal will clear it on the next tick.
bool test_stale_attack_vehicle_mission_does_not_block(sp<GameState> state)
{
	resetToBaseline(*state);
	if (!expectCanTurbo(*state, true, "baseline-before-stale-attack"))
		return false;

	auto type = makeTestVehicleType(*state, "VEHICLETYPE_TEST_ATTACKER_STALE", 0.0f);
	auto attackerOrg = makeNeutralOrg(*state, "ORG_TEST_ATTACKER_STALE");
	auto targetOrg = makeNeutralOrg(*state, "ORG_TEST_TARGET_STALE");

	auto attacker = spawnVehicle(*state, type, attackerOrg, testSpawnPosition(*state, 4));
	// Deliberately not placed on any map (no leaveBuilding call): tileObject stays
	// null, exactly as it does for a target that has left current_city for another
	// city - which is the condition VehicleMission::isFinishedInternal() actually
	// tests for AttackVehicle ("Target not on the map").
	auto target = state->current_city->createVehicle(*state, type, targetOrg);

	StateRef<Vehicle> targetRef{state.get(), target};
	attacker->missions.push_back(VehicleMission::attackVehicle(*state, *attacker, targetRef));

	bool ok = expectCanTurbo(*state, true, "stale-attack-vehicle-mission");
	attacker->missions.clear();
	return ok;
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");

	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto common_name = config().getString("common");
	auto gamestate_name = config().getString("gamestate");
	if (common_name.empty() || gamestate_name.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", false);

	auto state = mksp<GameState>();
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load common gamestate");
		return EXIT_FAILURE;
	}
	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load supplied gamestate");
		return EXIT_FAILURE;
	}

	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();

	bool allPassed = true;
	allPassed &= test_idle_hostile_aggressive_does_not_block(state);
	allPassed &= test_hostile_aggressive_with_mission_blocks(state);
	allPassed &= test_live_attack_vehicle_mission_blocks(state);
	allPassed &= test_stale_attack_vehicle_mission_does_not_block(state);

	if (!allPassed)
	{
		LogError("test_city_turbo: one or more scenarios failed");
		return EXIT_FAILURE;
	}

	LogInfo("test_city_turbo success - all scenarios passed");
	return EXIT_SUCCESS;
}
