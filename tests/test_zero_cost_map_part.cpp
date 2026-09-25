// Verifies that a tile whose only ground or feature parts carry a movement
// cost of 0 falls back to the standard entry cost instead of becoming free to
// enter. Uses synthetic map part types (no extracted tileset/gamestate data
// required), so this runs under normal CI like test_gravlift_lineoffire.cpp.
#include "framework/configfile.h"
#include "framework/logger.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/tilemap/tile.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battlemappart.h"

using namespace OpenApoc;

namespace
{

StateRef<BattleMapPartType> registerType(GameState &state, const UString &id,
                                         BattleMapPartType::Type type, bool floor, int movementCost,
                                         int height)
{
	auto mapPartType = mksp<BattleMapPartType>();
	mapPartType->type = type;
	mapPartType->floor = floor;
	mapPartType->movement_cost = movementCost;
	mapPartType->height = height;
	state.battleMapTiles[id] = mapPartType;
	return {&state, id};
}

void spawnPart(TileMap &map, StateRef<BattleMapPartType> type, Vec3<int> tile)
{
	auto part = mksp<BattleMapPart>();
	part->type = type;
	part->position = Vec3<float>(tile) + Vec3<float>{0.5f, 0.5f, 0.0f};
	map.addObjectToMap(part);
}

float entryCost(const BattleUnitTileHelper &helper, Tile *from, Tile *to)
{
	float cost = -1.0f;
	bool jumped = false;
	bool doorInTheWay = false;
	if (!helper.canEnterTile(from, to, false, jumped, cost, doorInTheWay))
	{
		return -1.0f;
	}
	return cost;
}

void expect(bool condition, const UString &message)
{
	if (!condition)
	{
		LogError("FAILED: {0}", message);
		exit(EXIT_FAILURE);
	}
}

} // namespace

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	auto state = mksp<GameState>();
	TileMap map{
	    {6, 1, 1}, {1, 1, 1}, {4, 4, 4}, {{TileObject::Type::Ground, TileObject::Type::Feature}}};

	auto floor = registerType(*state, "BATTLEMAPPART_TEST_FLOOR", BattleMapPartType::Type::Ground,
	                          true, 4, 3);
	auto roughFloor = registerType(*state, "BATTLEMAPPART_TEST_ROUGH_FLOOR",
	                               BattleMapPartType::Type::Ground, true, 6, 3);
	auto freeGround = registerType(*state, "BATTLEMAPPART_TEST_FREE_GROUND",
	                               BattleMapPartType::Type::Ground, false, 0, 0);
	auto freeFeature = registerType(*state, "BATTLEMAPPART_TEST_FREE_FEATURE",
	                                BattleMapPartType::Type::Feature, false, 0, 3);

	// x=0: plain floor, the origin of the moves into x=1.
	spawnPart(map, floor, {0, 0, 0});
	// x=1: ground sprite without a floor flag and with cost 0, the vanilla hill-side shape.
	spawnPart(map, freeGround, {1, 0, 0});
	// x=2: plain floor with a cost 0 overlay on top, must keep the floor's cost.
	spawnPart(map, floor, {2, 0, 0});
	spawnPart(map, freeGround, {2, 0, 0});
	// x=3: rough floor, must keep its higher cost.
	spawnPart(map, roughFloor, {3, 0, 0});
	// x=4: feature only, cost 0: standable, so walkers see it too.
	spawnPart(map, freeFeature, {4, 0, 0});
	// x=5: empty air.

	auto origin = map.getTile(Vec3<int>{0, 0, 0});
	auto freeGroundTile = map.getTile(Vec3<int>{1, 0, 0});
	auto overlayTile = map.getTile(Vec3<int>{2, 0, 0});
	auto roughTile = map.getTile(Vec3<int>{3, 0, 0});
	auto freeFeatureTile = map.getTile(Vec3<int>{4, 0, 0});
	auto airTile = map.getTile(Vec3<int>{5, 0, 0});

	expect(freeGroundTile->movementCostIn == 4,
	       "tile holding only a cost 0 ground part must cost the standard 4 to enter");
	expect(!freeGroundTile->canStand, "cost 0 ground part without a floor flag is not standable");
	expect(overlayTile->movementCostIn == 4,
	       "floor plus cost 0 overlay must keep the floor's cost");
	expect(roughTile->movementCostIn == 6, "rough floor must keep its own cost");
	expect(freeFeatureTile->movementCostIn == 4,
	       "tile holding only a cost 0 feature must cost the standard 4 to enter");
	expect(airTile->movementCostIn == 4, "empty tile must cost the standard 4 to enter");

	BattleUnitTileHelper flyer{map, BattleUnitType::SmallFlyer};
	BattleUnitTileHelper walker{map, BattleUnitType::SmallWalker};

	expect(entryCost(flyer, origin, freeGroundTile) == 4.0f,
	       "flyer must pay 4 TU to enter a cost 0 ground tile, not 0");
	expect(entryCost(walker, origin, freeGroundTile) < 0.0f,
	       "walker must still be refused a cost 0 ground tile with no floor");
	expect(entryCost(walker, overlayTile, roughTile) == 6.0f,
	       "walker pays the rough floor's own cost");
	expect(entryCost(walker, roughTile, freeFeatureTile) == 4.0f,
	       "walker must pay 4 TU to enter a cost 0 feature tile, not 0");
	expect(entryCost(flyer, freeFeatureTile, airTile) == 4.0f, "flyer pays 4 TU into empty air");

	printf("test_zero_cost_map_part: all checks passed\n");
	return EXIT_SUCCESS;
}
