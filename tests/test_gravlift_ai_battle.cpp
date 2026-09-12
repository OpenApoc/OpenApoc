// Manual AI-behaviour verification harness for OpenApoc issue #1330.
// Not part of the committed fix (needs full extracted tileset/gamestate data
// CI does not provide); build and run exactly like test_gravlift_lof.cpp.
//
// Purpose: the GravliftLineOfFire option lets a unit SEE another unit
// vertically through a lift shaft that it could not see before. That feeds
// BattleUnit::calculateVisionToUnit -> Battle::visibleUnits per organisation,
// which drives AI target selection (AIBlockTactical::think, called every
// Battle::update from battle.cpp) and reaction fire. The hypothesis under
// test: an AI unit spots a target only reachable/shootable via a path it
// cannot use, fixates on it, and stalls or loops instead of making progress
// (the same class of bug as GameState::canTurbo() spinning forever on an
// unreachable vehicle attack mission).
//
// Method: build a real battle in a tileset with grav-lift floor pads (see
// CANDIDATE_TILESETS), carve a synthetic 2-column lift shaft out of the
// tileset's real grav-lift pieces (same construction as
// test_gravlift_lof.cpp), put the player unit at the bottom of the shaft and
// a hostile org unit at the top (same column, so the only new sightline is
// the vertical one through the grav-lift floor pads), then drive
// Battle::update() in RealTime mode for many ticks. Log target selection,
// position, mission queue and wall-clock time per batch; run once with the
// flag ON and once OFF (set via config before Framework construction) and
// diff.
//
// Build:
//   1. Copy into tests/, add to tests/CMakeLists.txt like test_gravlift_lof.
//   2. flock -w 3600 /tmp/openapoc-build.lock ninja -C build -j2 test_gravlift_ai_battle
//   3. ./build/bin/test_gravlift_ai_battle <common> <gamestate> [tileset] [seed] \
//        --OpenApoc.NewFeature.GravliftLineOfFire=true|false \
//        --Framework.CD=data/cd.iso --Framework.Data=data
//
// [seed] is an optional fixed RNG seed (default 424242) used to make battle
// map generation reproducible: OpenApoc.NewFeature.SeedRng defaults to true
// and reseeds GameState::rng from wall-clock time in GameState::startGame(),
// and BattleMap::generateMap consumes that same rng to decide vertical
// stacking, sector packing order and sector choice, so without a fixed seed
// two runs (e.g. flag=false vs flag=true) can silently generate different
// map geometry and any AI-behaviour comparison between them is not
// controlled. This harness forces OpenApoc.NewFeature.SeedRng off and pins
// the seed explicitly so both arms of a comparison run see the same map.
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/agenttype.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/tile.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject.h"
#include "library/voxel.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <glm/glm.hpp>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <vector>

using namespace OpenApoc;

namespace
{
const std::set<TileObject::Type> mapPartSet = {TileObject::Type::Ground, TileObject::Type::LeftWall,
                                               TileObject::Type::RightWall,
                                               TileObject::Type::Feature};

const std::vector<UString> CANDIDATE_TILESETS = {"02police", "12shops", "06office", "07corphq"};

void clearTile(TileMap &map, Vec3<int> tile)
{
	auto t = map.getTile(tile);
	std::vector<sp<TileObject>> toRemove;
	for (auto &o : t->intersectingObjects)
	{
		if (mapPartSet.count(o->getType()))
		{
			toRemove.push_back(o);
		}
	}
	for (auto &o : toRemove)
	{
		o->removeFromMap();
	}
}

sp<BattleMapPart> addPart(GameState &state, TileMap &map, StateRef<BattleMapPartType> type,
                          Vec3<int> tile)
{
	auto part = mksp<BattleMapPart>();
	part->owner = state.getAliens();
	part->type = type;
	part->position = Vec3<float>(tile) + Vec3<float>{0.5f, 0.5f, 0.0f};
	map.addObjectToMap(part);
	return part;
}

UString missionSummary(sp<BattleUnit> u)
{
	if (u->missions.empty())
	{
		return "idle";
	}
	return u->missions.front()->getName();
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");
	config().addPositionalArgument("tileset", "Tileset to test (optional)");
	config().addPositionalArgument("seed", "Fixed RNG seed for deterministic map generation "
	                                       "(optional, default 424242)");
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	auto common_name = config().getString("common");
	auto gamestate_name = config().getString("gamestate");
	auto forcedTileset = config().getString("tileset");
	auto seedArg = config().getString("seed");
	if (common_name.empty() || gamestate_name.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}
	const uint64_t rngSeed = seedArg.empty() ? 424242ULL : std::stoull(seedArg);

	Framework fw("OpenApoc", false);

	printf("GravliftLineOfFire option = %s\n",
	       config().getBool("OpenApoc.NewFeature.GravliftLineOfFire") ? "ON" : "OFF");

	auto state = mksp<GameState>();
	if (!state->loadGame(common_name) || !state->loadGame(gamestate_name))
	{
		LogError("Failed to load gamestate");
		return EXIT_FAILURE;
	}

	// GameState::rng is not part of the save and OpenApoc.NewFeature.SeedRng
	// defaults to true, which reseeds it from wall-clock time in startGame()
	// below. That makes battle map generation (BattleMap::generateMap picks
	// vertical stacking, sector packing order and sector choice off state.rng)
	// nondeterministic between runs, so comparing flag=false against flag=true
	// output was actually comparing two different maps. Force the time-based
	// reseed off and pin the RNG to a fixed, caller-controlled seed instead so
	// both arms of the comparison see identical map geometry.
	config().set("OpenApoc.NewFeature.SeedRng", false);
	state->rng.seed(rngSeed);
	printf("RNG seed = %llu (OpenApoc.NewFeature.SeedRng forced off)\n",
	       (unsigned long long)rngSeed);

	state->startGame();
	state->initState();
	state->fillPlayerStartingProperty();
	state->fillOrgStartingProperty();

	std::vector<UString> candidates =
	    forcedTileset.empty() ? CANDIDATE_TILESETS : std::vector<UString>{forcedTileset};

	StateRef<Building> targetBuilding;
	UString chosenTileset;
	for (auto &candidate : candidates)
	{
		for (auto &p : state->buildings)
		{
			auto &b = p.second;
			if (b->base || !b->battle_map)
			{
				continue;
			}
			for (auto &ts : b->battle_map->tilesets)
			{
				if (ts == candidate)
				{
					targetBuilding = {state.get(), p.first};
					chosenTileset = ts;
					break;
				}
			}
			if (targetBuilding)
				break;
		}
		if (targetBuilding)
			break;
	}
	if (!targetBuilding)
	{
		LogError("No building found using any candidate tileset");
		return EXIT_FAILURE;
	}
	printf("Using building \"%s\" (tileset \"%s\")\n", targetBuilding->name.c_str(),
	       chosenTileset.c_str());

	std::list<StateRef<Agent>> playerAgents;
	for (auto &a : state->agents)
	{
		if (a.second->type->role == AgentType::Role::Soldier &&
		    a.second->owner == state->getPlayer())
		{
			playerAgents.emplace_back(state.get(), a.first);
		}
	}
	if (playerAgents.empty())
	{
		LogError("No player soldier agents available");
		return EXIT_FAILURE;
	}
	StateRef<Vehicle> playerCraft;
	for (auto &v : state->vehicles)
	{
		if (v.second->owner == state->getPlayer())
		{
			playerCraft = {state.get(), v.first};
			break;
		}
	}
	if (!playerCraft)
	{
		LogError("No player vehicle available");
		return EXIT_FAILURE;
	}

	StateRef<Organisation> opponent = targetBuilding->owner;
	Battle::beginBattle(*state, false, opponent, playerAgents, nullptr, nullptr, nullptr,
	                    playerCraft, targetBuilding);
	if (!state->current_battle)
	{
		LogError("Battle::beginBattle failed to create a battle");
		return EXIT_FAILURE;
	}
	auto battle = state->current_battle;
	Battle::enterBattle(*state);
	printf("Battle created, map size = {%d,%d,%d}, mode=%s\n", battle->map->size.x,
	       battle->map->size.y, battle->map->size.z,
	       battle->mode == Battle::Mode::TurnBased ? "TurnBased" : "RealTime");

	// Find the tileset's most solid grav-lift floor piece, same "worst case"
	// selection as test_gravlift_lof.cpp, and build a 2-column shaft with it.
	UString prefix = "BATTLEMAPPART_" + chosenTileset + "_";
	UString bestFloorId;
	int bestBits = -1;
	for (auto &p : state->battleMapTiles)
	{
		if (p.first.rfind(prefix, 0) != 0)
			continue;
		auto &t = p.second;
		if (!t->gravlift || !t->floor)
			continue;
		int bits = 0;
		if (t->voxelMapLOF)
		{
			for (int z = 0; z < t->voxelMapLOF->size.z; z++)
				for (int y = 0; y < t->voxelMapLOF->size.y; y++)
					for (int x = 0; x < t->voxelMapLOF->size.x; x++)
						if (t->voxelMapLOF->getBit({x, y, z}))
							bits++;
		}
		if (bits > bestBits)
		{
			bestBits = bits;
			bestFloorId = p.first;
		}
	}
	if (bestFloorId.empty())
	{
		printf("Tileset \"%s\" has no grav-lift floor pad (control case) - "
		       "flag must have no effect here.\n",
		       chosenTileset.c_str());
	}
	else
	{
		printf("Using grav-lift floor pad \"%s\" (LOF bits=%d)\n", bestFloorId.c_str(), bestBits);
	}

	const int LEVELS = std::min(4, battle->map->size.z - 1);
	const int X1 = 1, X2 = 2, Y = 1;
	if (!bestFloorId.empty())
	{
		for (int z = 0; z < LEVELS; z++)
		{
			clearTile(*battle->map, {X1, Y, z});
			clearTile(*battle->map, {X2, Y, z});
			for (int x : {X1, X2})
			{
				addPart(*state, *battle->map, {state.get(), bestFloorId}, {x, Y, z});
			}
		}
	}

	// Grab one real player unit and one real hostile unit.
	sp<BattleUnit> playerUnit;
	for (auto &f : battle->forces[state->getPlayer()].squads)
	{
		if (f.getNumUnits() > 0)
		{
			playerUnit = f.units.front();
			break;
		}
	}
	sp<BattleUnit> hostileUnit;
	for (auto &f : battle->forces[opponent].squads)
	{
		for (auto &u : f.units)
		{
			if (u)
			{
				hostileUnit = u;
				break;
			}
		}
		if (hostileUnit)
			break;
	}
	if (!playerUnit || !hostileUnit)
	{
		LogError("Could not find both a player unit and a hostile unit "
		         "(player=%d, hostile=%d)",
		         (int)(bool)playerUnit, (int)(bool)hostileUnit);
		return EXIT_FAILURE;
	}
	printf("Player unit: %s (org=%s)\n", playerUnit->id.c_str(), playerUnit->owner->name.c_str());
	printf("Hostile unit: %s (org=%s)\n", hostileUnit->id.c_str(),
	       hostileUnit->owner->name.c_str());

	// Place them on different levels of the same shaft column so the only new
	// sightline exercised is the vertical one through the grav-lift pad(s).
	playerUnit->setPosition(*state, {X1 + 0.5f, (float)Y, 0.5f});
	playerUnit->setBodyState(*state, BodyState::Standing);
	hostileUnit->setPosition(*state, {X1 + 0.5f, (float)Y, (float)(LEVELS - 1) + 0.5f});
	hostileUnit->setBodyState(*state, BodyState::Standing);

	printf("hasLineToPosition (hostile -> player) at t=0: %s\n",
	       hostileUnit->hasLineToPosition(playerUnit->position, false) ? "true" : "false");

	// Drive the simulation. Battle::update() calls aiBlock.think() every tick
	// in RealTime mode (battle.cpp), which is the code path under test.
	const unsigned TICKS_PER_BATCH = TICKS_PER_SECOND; // ~1 simulated second
	const int BATCHES = 240;                           // ~1 simulated minute
	printf("\n%-6s %-9s %-24s %-24s %-10s %-10s %-10s\n", "batch", "ms/batch", "hostile-pos",
	       "player-pos", "h-mission", "h-target", "visible");
	double firstBatchMs = -1.0, lastBatchMs = -1.0;
	Vec3<float> firstHostilePos = hostileUnit->position;
	for (int batch = 0; batch < BATCHES; batch++)
	{
		auto t0 = std::chrono::steady_clock::now();
		battle->update(*state, TICKS_PER_BATCH);
		auto t1 = std::chrono::steady_clock::now();
		double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
		if (firstBatchMs < 0)
			firstBatchMs = ms;
		lastBatchMs = ms;

		if (batch % 5 == 0 || batch == BATCHES - 1)
		{
			auto hp = hostileUnit->position;
			auto pp = playerUnit->position;
			UString targetName = hostileUnit->targetUnit ? hostileUnit->targetUnit.id : "(none)";
			int visible = 0;
			auto vit = battle->visibleUnits.find(hostileUnit->owner);
			if (vit != battle->visibleUnits.end())
				visible = (int)vit->second.size();
			printf("%-6d %-9.3f (%5.2f,%5.2f,%5.2f)  (%5.2f,%5.2f,%5.2f)  %-10s %-10s %-10d\n",
			       batch, ms, hp.x, hp.y, hp.z, pp.x, pp.y, pp.z,
			       missionSummary(hostileUnit).c_str(), targetName.c_str(), visible);
		}
		if (hostileUnit->isDead() || playerUnit->isDead())
		{
			printf("Unit died at batch %d - ending early (combat resolved)\n", batch);
			break;
		}
	}
	printf("\nWall-clock ms: first batch=%.3f last batch=%.3f (ratio=%.2fx)\n", firstBatchMs,
	       lastBatchMs, firstBatchMs > 0 ? lastBatchMs / firstBatchMs : 0.0);
	printf("Hostile net displacement: %.3f tiles\n",
	       glm::length(hostileUnit->position - firstHostilePos));
	printf("Final: hostile alive=%d player alive=%d hostile mission=%s\n", !hostileUnit->isDead(),
	       !playerUnit->isDead(), missionSummary(hostileUnit).c_str());

	printf("\ntest_gravlift_ai_battle done\n");
	return EXIT_SUCCESS;
}
