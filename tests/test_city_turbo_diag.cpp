// Diagnostic harness for GameState::canTurbo() ("game cannot enter the fastest
// speed").
//
// Loads a savegame on top of the base ruleset, replicates the exact logic of
// GameState::canTurbo() (game/state/gamestate.cpp) by calling it directly, and
// reports WHICH blocker fires and on which object, dumping the full state needed
// to evaluate a reachability predicate for blockers (2) and (3).
// Evidence-gathering only: this does not change any game logic.
//
// Not wired into ctest; deliberately not add_test'd.
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/gamestate_serialize.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/organisation.h"
#include "game/state/shared/projectile.h"
#include <iostream>

using namespace OpenApoc;

namespace
{

sp<GameState> loadSaveOnRuleset(const UString &common_name, const UString &gamestate_name)
{
	auto state = mksp<GameState>();
	if (!state->loadGame(common_name))
	{
		LogError("Failed to load common gamestate \"{0}\"", common_name);
		return nullptr;
	}
	if (!state->loadGame(gamestate_name))
	{
		LogError("Failed to load gamestate \"{0}\"", gamestate_name);
		return nullptr;
	}
	return state;
}

// Several attached saves (e.g. save_city2, from 2021) have a
// CITYMAP_ALIEN scenery entry whose SceneryTileType id fails to resolve
// against this build's ruleset+save merge (observed: "CITYTILE_ALIENMAP_5"),
// which segfaults inside GameState::initState()'s first loop
// (`!s->type->commonProperty` on unresolved StateRef) well before it even
// reaches per-city map setup. canTurbo() only ever looks at
// state->current_city, so we replicate here only the subset of initState()
// needed to populate current_city's map and vehicle tileObjects, touching
// nothing in any other city. This is a harness-only workaround, not an
// engine fix.
void initCurrentCityOnly(GameState &state)
{
	auto city = state.current_city;
	for (auto &s : city->scenery)
	{
		for (auto &b : city->buildings)
		{
			Vec2<int> pos2d{s->initialPosition.x, s->initialPosition.y};
			if (b->bounds.within(pos2d))
			{
				s->building = b;
				if (s->isAlive() && !s->type->commonProperty)
				{
					s->building->buildingParts.insert(s->initialPosition);
				}
				break;
			}
		}
	}
	city->initCity(state);
	for (auto &v : state.vehicles)
	{
		auto vehicle = v.second;
		if (vehicle->city == city && !vehicle->currentBuilding && !vehicle->betweenDimensions)
		{
			city->map->addObjectToMap(state, vehicle);
		}
	}
	// Deliberately not re-linking projectile->trackedObject here: canTurbo()
	// only needs city->projectiles.empty(), and doing so needs a
	// TileObjectVehicle->TileObject pointer cast this harness has no
	// reason to carry.
}

UString missionTypeName(VehicleMission::MissionType t)
{
	switch (t)
	{
		case VehicleMission::MissionType::GotoLocation:
			return "GotoLocation";
		case VehicleMission::MissionType::GotoBuilding:
			return "GotoBuilding";
		case VehicleMission::MissionType::FollowVehicle:
			return "FollowVehicle";
		case VehicleMission::MissionType::RecoverVehicle:
			return "RecoverVehicle";
		case VehicleMission::MissionType::AttackVehicle:
			return "AttackVehicle";
		case VehicleMission::MissionType::AttackBuilding:
			return "AttackBuilding";
		case VehicleMission::MissionType::RestartNextMission:
			return "RestartNextMission";
		case VehicleMission::MissionType::Snooze:
			return "Snooze";
		case VehicleMission::MissionType::TakeOff:
			return "TakeOff";
		case VehicleMission::MissionType::Land:
			return "Land";
		case VehicleMission::MissionType::Crash:
			return "Crash";
		case VehicleMission::MissionType::Patrol:
			return "Patrol";
		case VehicleMission::MissionType::GotoPortal:
			return "GotoPortal";
		case VehicleMission::MissionType::InfiltrateSubvert:
			return "InfiltrateSubvert";
		case VehicleMission::MissionType::OfferService:
			return "OfferService";
		case VehicleMission::MissionType::Teleport:
			return "Teleport";
		case VehicleMission::MissionType::SelfDestruct:
			return "SelfDestruct";
		case VehicleMission::MissionType::DepartToSpace:
			return "DepartToSpace";
		case VehicleMission::MissionType::ArriveFromDimensionGate:
			return "ArriveFromDimensionGate";
		case VehicleMission::MissionType::InvestigateBuilding:
			return "InvestigateBuilding";
	}
	return "Unknown";
}

UString vehicleTypeMovementName(VehicleType::Type t)
{
	switch (t)
	{
		case VehicleType::Type::Flying:
			return "Flying";
		case VehicleType::Type::UFO:
			return "UFO";
		case VehicleType::Type::Road:
			return "Road";
		case VehicleType::Type::ATV:
			return "ATV";
	}
	return "Unknown";
}

UString reachabilityName(VehicleTargetHelper::Reachability r)
{
	switch (r)
	{
		case VehicleTargetHelper::Reachability::Reachable:
			return "Reachable";
		case VehicleTargetHelper::Reachability::BlockedByVehicle:
			return "BlockedByVehicle";
		case VehicleTargetHelper::Reachability::BlockedByScenery:
			return "BlockedByScenery";
		case VehicleTargetHelper::Reachability::BlockedByBuilding:
			return "BlockedByBuilding";
	}
	return "Unknown";
}

UString relationName(Organisation::Relation r)
{
	switch (r)
	{
		case Organisation::Relation::Hostile:
			return "Hostile";
		case Organisation::Relation::Unfriendly:
			return "Unfriendly";
		case Organisation::Relation::Neutral:
			return "Neutral";
		case Organisation::Relation::Friendly:
			return "Friendly";
		case Organisation::Relation::Allied:
			return "Allied";
	}
	return "Unknown";
}

void dumpMission(GameState &state, Vehicle &owner, VehicleMission &m, const UString &indent)
{
	// isFinished(..., callUpdateIfFinished=false) is a pure read of
	// isFinishedInternal(): does the engine's OWN completion check already
	// consider this mission done on the state as loaded, with no tick
	// required? If true, a stuck-looking mission in this snapshot is
	// transient (it will clear itself the moment the sim next ticks); if
	// false, nothing about advancing time alone will ever clear it.
	bool wouldFinishNow = m.isFinished(state, owner, false);
	std::cout << indent << "mission type=" << missionTypeName(m.type)
	          << " wouldFinishNowWithNoFurtherTicks=" << wouldFinishNow
	          << " targetLocation=" << m.targetLocation.x << "," << m.targetLocation.y << ","
	          << m.targetLocation.z << " cancelled=" << m.cancelled
	          << " currentPlannedPath.size()=" << m.currentPlannedPath.size();
	if (m.targetBuilding)
	{
		auto b = m.targetBuilding;
		std::cout << " targetBuilding=" << b.id << " (bounds " << b->bounds.p0.x << ","
		          << b->bounds.p0.y << " - " << b->bounds.p1.x << "," << b->bounds.p1.y << ")";
		std::cout << " targetBuildingCity=" << (b->city ? b->city.id : UString("<null>"));
	}
	if (m.targetVehicle)
	{
		auto tv = m.targetVehicle;
		bool exists = state.vehicles.find(tv.id) != state.vehicles.end();
		std::cout << " targetVehicle=" << tv.id << " existsInState=" << exists;
		if (exists)
		{
			auto &t = state.vehicles.at(tv.id);
			std::cout << " targetOwner=" << (t->owner ? t->owner.id : UString("<null>"))
			          << " targetIsDead=" << t->isDead() << " targetCrashed=" << t->crashed
			          << " targetCity=" << (t->city ? t->city.id : UString("<null>"))
			          << " targetPos=" << t->position.x << "," << t->position.y << ","
			          << t->position.z
			          << " targetTileObject=" << (t->tileObject != nullptr ? "present" : "null");
		}
	}
	std::cout << "\n";
}

void dumpVehicle(GameState &state, const UString &id, sp<Vehicle> &v, const UString &why)
{
	std::cout << "  [" << why << "] vehicle id=" << id << " name=" << v->name
	          << " type=" << (v->type ? v->type.id : UString("<null>")) << " movementType="
	          << (v->type ? vehicleTypeMovementName(v->type->type) : UString("<null>"))
	          << " owner=" << (v->owner ? v->owner.id : UString("<null>"))
	          << " aggressiveness=" << (v->type ? v->type->aggressiveness : -1)
	          << " relationToPlayer="
	          << (v->owner ? relationName(v->owner->isRelatedTo(state.getPlayer()))
	                       : UString("<no owner>"))
	          << " crashed=" << v->crashed << " isDead=" << v->isDead()
	          << " city=" << (v->city ? v->city.id : UString("<null>"))
	          << " tileObject=" << (v->tileObject != nullptr ? "present" : "null")
	          << " position=" << v->position.x << "," << v->position.y << "," << v->position.z
	          << " missions.size()=" << v->missions.size()
	          << " stunTicksRemaining=" << v->stunTicksRemaining;
	// Is the vehicle's OWN current tile one its own movement type could ever
	// have entered? A ground/road vehicle sitting on a non-road tile (e.g.
	// inside a building footprint) can never move again regardless of what
	// mission it holds - this tests that directly against the real
	// per-movement-type reachability rules, rather than assuming it.
	if (v->tileObject != nullptr && v->city == state.current_city)
	{
		Vec3<int> herePos{(int)v->position.x, (int)v->position.y, (int)v->position.z};
		auto reach = VehicleTargetHelper::isReachableTarget(*v, herePos);
		std::cout << " ownTileReachableForOwnMovementType=" << reachabilityName(reach);
	}
	std::cout << "\n";
	for (auto &m : v->missions)
	{
		dumpMission(state, *v, m, "      ");
	}
}

void analyseSave(const UString &label, const UString &common_name, const UString &gamestate_name)
{
	std::cout << "\n================ " << label << " ================\n";
	auto state = loadSaveOnRuleset(common_name, gamestate_name);
	if (!state)
	{
		std::cout << "FAILED TO LOAD\n";
		return;
	}
	if (!state->current_city)
	{
		std::cout << "current_city is null before init, cannot evaluate canTurbo()\n";
		return;
	}
	initCurrentCityOnly(*state);
	std::cout << "current_city=" << state->current_city.id
	          << " projectiles.size()=" << state->current_city->projectiles.size() << "\n";

	bool blocker1 = !state->current_city->projectiles.empty();
	std::vector<UString> blocker2ids;
	std::vector<UString> blocker3ids;

	for (auto &v : state->vehicles)
	{
		if (!v.second->isDead() && v.second->city == state->current_city &&
		    v.second->tileObject != nullptr)
		{
			if (v.second->type->aggressiveness > 0 &&
			    v.second->owner->isRelatedTo(state->getPlayer()) ==
			        Organisation::Relation::Hostile &&
			    !v.second->crashed)
			{
				blocker2ids.push_back(v.first);
			}
			for (auto &m : v.second->missions)
			{
				if (m.type == VehicleMission::MissionType::AttackBuilding ||
				    m.type == VehicleMission::MissionType::AttackVehicle)
				{
					blocker3ids.push_back(v.first);
				}
			}
		}
	}

	bool result = state->canTurbo();
	std::cout << "canTurbo() = " << (result ? "TRUE (not locked)" : "FALSE (LOCKED)") << "\n";
	std::cout << "blocker(1) projectiles-in-flight: " << (blocker1 ? "FIRES" : "clear") << "\n";
	std::cout << "blocker(2) hostile-aggressive-vehicle count: " << blocker2ids.size() << "\n";
	for (auto &id : blocker2ids)
	{
		dumpVehicle(*state, id, state->vehicles.at(id), "blocker2");
	}
	std::cout << "blocker(3) pending-attack-mission count: " << blocker3ids.size() << "\n";
	for (auto &id : blocker3ids)
	{
		dumpVehicle(*state, id, state->vehicles.at(id), "blocker3");
	}

	// Also dump every non-dead vehicle in the current city that is NOT
	// mid-flight-attached to a tileObject, and every vehicle whose mission
	// list is empty but who is not idle at a base, as candidates for the
	// "stuck" reports even when they don't trip a blocker (useful for the
	// negative-control saves).
	std::cout << "-- all live vehicles in current_city (context) --\n";
	for (auto &v : state->vehicles)
	{
		if (!v.second->isDead() && v.second->city == state->current_city)
		{
			dumpVehicle(*state, v.first, v.second, "context");
		}
	}
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common ruleset gamestate to load first");
	config().addPositionalArgument("gamestate", "Savegame to load on top of it");

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

	analyseSave("city-turbo-save", common_name, gamestate_name);

	return EXIT_SUCCESS;
}
