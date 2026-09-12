// Verification harness for sub-problems A and B (line of fire through
// grav-lift geometry). NOT part of the committed fix: it needs the full
// extracted tileset/map/gamestate data that CI's minimal cd.iso does not
// provide, so it is kept out of tests/CMakeLists.txt and run manually from
// the scratchpad instead.
//
// Boots a real battle (common + gamestate, exactly like the other tests in
// tests/) in a building that actually uses a grav-lift tileset, so every
// StateRef in the chain (damage types, damage modifiers, agent body types) is
// resolved through the normal game data pipeline rather than being faked. A
// real player BattleUnit (real agent, real body type, real getMuzzleLocation)
// is then moved through a small controlled lift shaft built from the
// tileset's REAL decoded grav-lift map part types, and
// BattleUnit::hasLineToPosition is called directly, exactly as the game
// calls it when resolving a shot.
//
// Build (from an OpenApoc worktree with the fix applied or reverted):
//   1. Temporarily add to tests/CMakeLists.txt:
//        set(TEST test_gravlift_lof)
//        add_executable(${TEST} ${TEST}.cpp)
//        target_link_libraries(${TEST} OpenApoc_Library OpenApoc_Framework OpenApoc_GameState)
//        target_compile_definitions(${TEST} PRIVATE -DUNIT_TEST)
//      (copy this file into tests/ first)
//   2. cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_CCACHE=ON
//   3. flock -w 3600 /tmp/openapoc-build.lock ninja -C build -j2 test_gravlift_lof
//   4. ./build/bin/test_gravlift_lof <common> <gamestate> [tileset] \
//        --Framework.CD=data/cd.iso --Framework.Data=data
//      e.g. common    = data/mods/base/data/submods/org.openapoc.base/difficulty0
//           gamestate = data/mods/base/base_gamestate
//           tileset   = 02police | 07corphq | 12shops | 06office (optional;
//                       defaults to trying them in that order)
//   Needs data/{tilesets,maps,imagepacks,animationpacks,bulletsprites,cd.iso}
//   and data/mods/base/{base_gamestate,modinfo.xml,data/submods/.../difficulty*}
//   populated -- these are gitignored generated artifacts; reference them
//   read-only from a checkout that already has them extracted (symlink for
//   tilesets/maps/cd.iso; PhysFS refuses to traverse symlinked directories,
//   so imagepacks/animationpacks must be real copies, not symlinks).
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/building.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/agenttype.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tile.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "library/voxel.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <list>
#include <set>
#include <vector>

using namespace OpenApoc;

namespace
{
const std::set<TileObject::Type> mapPartSet = {TileObject::Type::Ground, TileObject::Type::LeftWall,
                                               TileObject::Type::RightWall,
                                               TileObject::Type::Feature};

const std::vector<UString> CANDIDATE_TILESETS = {"02police", "07corphq", "12shops", "06office"};

struct VoxelStats
{
	int totalBits = 0;
	int slicesWithBits = 0;
	int totalSlices = 0;
	int minX = -1, maxX = -1, minY = -1, maxY = -1, minZ = -1, maxZ = -1;
};

VoxelStats analyseVoxelMap(const sp<VoxelMap> &map)
{
	VoxelStats stats;
	if (!map)
	{
		return stats;
	}
	stats.totalSlices = map->size.z;
	for (int z = 0; z < map->size.z; z++)
	{
		int bitsInSlice = 0;
		for (int y = 0; y < map->size.y; y++)
		{
			for (int x = 0; x < map->size.x; x++)
			{
				if (map->getBit({x, y, z}))
				{
					bitsInSlice++;
					if (stats.minX == -1 || x < stats.minX)
						stats.minX = x;
					if (x > stats.maxX)
						stats.maxX = x;
					if (stats.minY == -1 || y < stats.minY)
						stats.minY = y;
					if (y > stats.maxY)
						stats.maxY = y;
					if (stats.minZ == -1 || z < stats.minZ)
						stats.minZ = z;
					if (z > stats.maxZ)
						stats.maxZ = z;
				}
			}
		}
		if (bitsInSlice > 0)
		{
			stats.slicesWithBits++;
		}
		stats.totalBits += bitsInSlice;
	}
	return stats;
}

void printPieceStats(const UString &id, const sp<BattleMapPartType> &type)
{
	auto lof = analyseVoxelMap(type->voxelMapLOF);
	printf("  %-28s type=%-10s floor=%d gravlift=%d height=%-4d LOF bits=%-4d slices=%d/%d",
	       id.c_str(),
	       type->type == BattleMapPartType::Type::Ground     ? "Ground"
	       : type->type == BattleMapPartType::Type::Feature  ? "Feature"
	       : type->type == BattleMapPartType::Type::LeftWall ? "LeftWall"
	                                                         : "RightWall",
	       (int)type->floor, (int)type->gravlift, type->height, lof.totalBits, lof.slicesWithBits,
	       lof.totalSlices);
	if (lof.totalBits > 0)
	{
		printf("  bbox x[%d-%d] y[%d-%d] z[%d-%d]", lof.minX, lof.maxX, lof.minY, lof.maxY,
		       lof.minZ, lof.maxZ);
	}
	printf("\n");
}

// Clears any existing real map content at a tile/z so our synthetic shaft is
// not confounded by whatever the procedurally-picked building happened to put
// there.
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

const char *blockingPieceLabel(const Collision &c)
{
	if (!c)
	{
		return "NONE (clear)";
	}
	auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(c.obj)->getOwner();
	if (mp->type->floor)
	{
		return "floor";
	}
	switch (mp->type->type)
	{
		case BattleMapPartType::Type::Feature:
			return "feature/frame";
		case BattleMapPartType::Type::LeftWall:
			return "leftwall";
		case BattleMapPartType::Type::RightWall:
			return "rightwall";
		case BattleMapPartType::Type::Ground:
			return "ground(non-floor)";
		default:
			return "other";
	}
}

} // namespace

int main(int argc, char **argv)
{
	config().addPositionalArgument("common", "Common gamestate to load");
	config().addPositionalArgument("gamestate", "Gamestate to load");
	config().addPositionalArgument("tileset", "Tileset to test (optional)");
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}
	auto common_name = config().getString("common");
	auto gamestate_name = config().getString("gamestate");
	auto forcedTileset = config().getString("tileset");
	if (common_name.empty() || gamestate_name.empty())
	{
		std::cerr << "Must provide common and gamestate\n";
		config().showHelp();
		return EXIT_FAILURE;
	}

	Framework fw("OpenApoc", false);

	auto state = mksp<GameState>();
	LogInfo("Loading common gamestate \"{0}\"", common_name);
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
	state->fillPlayerStartingProperty();
	state->fillOrgStartingProperty();

	std::vector<UString> candidates =
	    forcedTileset.empty() ? CANDIDATE_TILESETS : std::vector<UString>{forcedTileset};

	// Find a non-base building that uses one of the candidate grav-lift tilesets.
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
			{
				break;
			}
		}
		if (targetBuilding)
		{
			break;
		}
	}
	if (!targetBuilding)
	{
		LogError("No building found using any candidate grav-lift tileset");
		return EXIT_FAILURE;
	}
	printf("Using building \"%s\" (tileset \"%s\")\n", targetBuilding->name.c_str(),
	       chosenTileset.c_str());

	// Gather a player soldier and a player vehicle to run the battle
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
	printf("Battle created, map size = {%d,%d,%d}\n", battle->map->size.x, battle->map->size.y,
	       battle->map->size.z);

	// Spawns real units (including our player soldier) and, crucially, re-runs
	// Battle::loadResources() afterwards so the now-existing units' image and
	// animation packs actually get loaded (the earlier loadResources() call
	// inside createBattle only knows about org guard-type pools, not
	// yet-to-be-spawned player agents). Without this, setBodyState() below
	// segfaults on a null animation pack.
	Battle::enterBattle(*state);

	// Dump EVERY gravlift-flagged map part type in this tileset, not just one
	// per category: an earlier version of this harness picked the
	// alphabetically-first id per category via state->battleMapTiles (a
	// std::map<UString,...>), which for several tilesets happens to be an
	// empty placeholder variant while OTHER ids in the same category carry
	// real sliver geometry. Print all of them so nothing is hidden, and for
	// building the test shaft pick, per category, the id with the largest
	// LOF bit count (the "worst case" / most solid variant actually used by
	// the real map).
	UString prefix = "BATTLEMAPPART_" + chosenTileset + "_";
	struct Best
	{
		UString id;
		int bits = -1;
	};
	Best bestFloor, bestFeature, bestLeftWall, bestRightWall;
	printf("\nAll grav-lift map part types in tileset \"%s\":\n", chosenTileset.c_str());
	for (auto &p : state->battleMapTiles)
	{
		if (p.first.rfind(prefix, 0) != 0)
		{
			continue;
		}
		auto &t = p.second;
		if (!t->gravlift)
		{
			continue;
		}
		printPieceStats(p.first, t);
		auto lof = analyseVoxelMap(t->voxelMapLOF);
		Best *slot = t->floor                                        ? &bestFloor
		             : t->type == BattleMapPartType::Type::Feature   ? &bestFeature
		             : t->type == BattleMapPartType::Type::LeftWall  ? &bestLeftWall
		             : t->type == BattleMapPartType::Type::RightWall ? &bestRightWall
		                                                             : nullptr;
		if (slot && lof.totalBits > slot->bits)
		{
			slot->id = p.first;
			slot->bits = lof.totalBits;
		}
	}
	printf("\nRepresentative (largest-LOF-footprint) id chosen per category:\n");
	printf("  floor     -> %s (bits=%d)\n", bestFloor.id.empty() ? "NONE" : bestFloor.id.c_str(),
	       bestFloor.bits);
	printf("  feature   -> %s (bits=%d)\n",
	       bestFeature.id.empty() ? "NONE" : bestFeature.id.c_str(), bestFeature.bits);
	printf("  leftwall  -> %s (bits=%d)\n",
	       bestLeftWall.id.empty() ? "NONE" : bestLeftWall.id.c_str(), bestLeftWall.bits);
	printf("  rightwall -> %s (bits=%d)\n",
	       bestRightWall.id.empty() ? "NONE" : bestRightWall.id.c_str(), bestRightWall.bits);

	if (bestFloor.id.empty() && bestFeature.id.empty())
	{
		LogError("Tileset \"{0}\" resolved no usable grav-lift pieces", chosenTileset);
		return EXIT_FAILURE;
	}

	// Build a controlled 2-column, multi-level lift shaft out of the real
	// grav-lift pieces (the most solid variant per category), at a spot in
	// the real battle map cleared of whatever the procedurally-picked
	// building layout put there.
	const int LEVELS = std::min(4, battle->map->size.z - 1);
	const int X1 = 1, X2 = 2, Y = 1;
	for (int z = 0; z < LEVELS; z++)
	{
		clearTile(*battle->map, {X1, Y, z});
		clearTile(*battle->map, {X2, Y, z});
		for (int x : {X1, X2})
		{
			if (!bestFloor.id.empty())
				addPart(*state, *battle->map, {state.get(), bestFloor.id}, {x, Y, z});
			if (!bestFeature.id.empty())
				addPart(*state, *battle->map, {state.get(), bestFeature.id}, {x, Y, z});
			if (!bestLeftWall.id.empty())
				addPart(*state, *battle->map, {state.get(), bestLeftWall.id}, {x, Y, z});
			if (!bestRightWall.id.empty())
				addPart(*state, *battle->map, {state.get(), bestRightWall.id}, {x, Y, z});
		}
	}
	auto liftTile = battle->map->getTile(Vec3<int>{X1, Y, 0});
	printf("\nTile::hasLift at synthetic lift column (%d,%d,0) = %d\n", X1, Y,
	       (int)liftTile->hasLift);

	// A single real, fully-spawned player unit does all the shooting (same
	// lookup Battle::enterBattle itself uses to find the first player unit).
	sp<BattleUnit> unit;
	for (auto &f : battle->forces[state->getPlayer()].squads)
	{
		if (f.getNumUnits() > 0)
		{
			unit = f.units.front();
			break;
		}
	}
	if (!unit)
	{
		LogError("No spawned player unit found after Battle::enterBattle");
		return EXIT_FAILURE;
	}

	// Record the real per-stance muzzle height (as a fraction of tile height)
	// so the blocking bands reported below can be related back to it.
	for (auto stance : {BodyState::Kneeling, BodyState::Standing})
	{
		unit->setPosition(*state, {X1 + 0.5f, (float)Y, 0.0f});
		unit->setBodyState(*state, stance);
		auto muzzle = unit->getMuzzleLocation();
		printf("Real muzzle height (%s): z-offset = %.3f (voxel z = %.1f of 20)\n",
		       stance == BodyState::Kneeling ? "kneeling" : "standing", muzzle.z, muzzle.z * 20.0f);
	}

	printf("\n=== Sub-problem A: horizontal shot between adjacent lift tiles (x=%d -> x=%d, "
	       "same level) ===\n",
	       X1, X2);
	printf("%-10s %-8s %-8s %-14s\n", "stance", "level", "y-offset", "blocked-by");
	for (int z = 0; z < LEVELS; z++)
	{
		for (auto stance : {BodyState::Kneeling, BodyState::Standing})
		{
			const char *stanceName = (stance == BodyState::Kneeling) ? "kneeling" : "standing";
			for (float yOff : {0.0f, 0.25f, 0.5f})
			{
				Vec3<float> pos{X1 + 0.5f, Y + yOff, (float)z};
				unit->setPosition(*state, pos);
				unit->setBodyState(*state, stance);
				auto muzzle = unit->getMuzzleLocation();
				Vec3<float> target{X2 + 0.5f, Y + yOff, muzzle.z};
				bool clear = unit->hasLineToPosition(target, false);
				auto c =
				    battle->map->findCollision(muzzle, target, mapPartSet, unit->tileObject, false);
				printf("%-10s %-8d %-8.2f %-14s (hasLineToPosition=%s)\n", stanceName, z, yOff,
				       blockingPieceLabel(c), clear ? "true" : "false");
			}
		}
	}

	printf("\n=== Sub-problem B: vertical shot down through the shaft (top level -> bottom) "
	       "===\n");
	printf("%-10s %-14s %-14s\n", "stance", "from-level", "blocked-by");
	for (auto stance : {BodyState::Kneeling, BodyState::Standing})
	{
		const char *stanceName = (stance == BodyState::Kneeling) ? "kneeling" : "standing";
		for (int fromLevel = LEVELS - 1; fromLevel >= 1; fromLevel--)
		{
			Vec3<float> pos{X1 + 0.5f, (float)Y, (float)fromLevel};
			unit->setPosition(*state, pos);
			unit->setBodyState(*state, stance);
			auto muzzle = unit->getMuzzleLocation();
			Vec3<float> target{X1 + 0.5f, (float)Y, 0.5f};
			bool clear = unit->hasLineToPosition(target, false);
			auto c =
			    battle->map->findCollision(muzzle, target, mapPartSet, unit->tileObject, false);
			printf("%-10s %-14d %-14s (hasLineToPosition=%s)\n", stanceName, fromLevel,
			       blockingPieceLabel(c), clear ? "true" : "false");
		}
	}

	// Fine-grained geometry probe: real body stances only tell us whether THAT
	// specific muzzle height happens to clip something. Sweep raw z-offsets
	// across the lower part of the tile (where all the non-empty grav-lift
	// voxel data we decoded above actually sits) so a narrow blocking band
	// cannot hide between the two stance samples.
	printf("\n=== Sub-problem A geometry probe: raw z-offset sweep at level 0, x=%d -> x=%d "
	       "===\n",
	       X1, X2);
	printf("%-10s %-14s\n", "z-offset", "blocked-by");
	for (float zOff = 0.0f; zOff <= 0.5f; zOff += 0.05f)
	{
		Vec3<float> start{X1 + 0.5f, (float)Y, zOff};
		Vec3<float> target{X2 + 0.5f, (float)Y, zOff};
		auto c = battle->map->findCollision(start, target, mapPartSet, unit->tileObject, false);
		printf("%-10.2f %-14s\n", zOff, blockingPieceLabel(c));
	}

	printf("\n=== Sub-problem B geometry probe: raw z-offset sweep, vertical shot per level "
	       "===\n");
	printf("%-10s %-8s %-14s\n", "z-offset", "level", "blocked-by");
	for (float zOff = 0.0f; zOff <= 0.5f; zOff += 0.05f)
	{
		for (int fromLevel = LEVELS - 1; fromLevel >= 1; fromLevel--)
		{
			Vec3<float> start{X1 + 0.5f, (float)Y, (float)fromLevel + zOff};
			Vec3<float> target{X1 + 0.5f, (float)Y, 0.5f};
			auto c = battle->map->findCollision(start, target, mapPartSet, unit->tileObject, false);
			printf("%-10.2f %-8d %-14s\n", zOff, fromLevel, blockingPieceLabel(c));
		}
	}

	printf("\ntest_gravlift_lof done\n");
	return EXIT_SUCCESS;
}
