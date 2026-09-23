// Verifies that a tile with a gravlift part never ends up with solidGround
// set, even when vanilla map data also drops a plain (non-gravlift) floor
// into the same tile as the lift shaft. Uses synthetic map part types (no
// extracted tileset/gamestate data required), so this runs under normal CI
// like test_gravlift_lineoffire.cpp.
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
                                         BattleMapPartType::Type type, bool floor, bool gravlift,
                                         int height)
{
	auto mapPartType = mksp<BattleMapPartType>();
	mapPartType->type = type;
	mapPartType->floor = floor;
	mapPartType->gravlift = gravlift;
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
	// x=0 is the lift shaft column, x=1 and x=2 are single-tile controls.
	TileMap map{
	    {3, 1, 4}, {1, 1, 1}, {4, 4, 4}, {{TileObject::Type::Ground, TileObject::Type::Feature}}};

	auto liftFeature = registerType(*state, "BATTLEMAPPART_TEST_LIFT_FEATURE",
	                                BattleMapPartType::Type::Feature, false, true, 8);
	auto liftFloorPad = registerType(*state, "BATTLEMAPPART_TEST_LIFT_FLOOR_PAD",
	                                 BattleMapPartType::Type::Ground, true, true, 8);
	auto plainFloor = registerType(*state, "BATTLEMAPPART_TEST_PLAIN_FLOOR",
	                               BattleMapPartType::Type::Ground, true, false, 3);

	// Shaft column: lift feature running the full height, a lift floor pad at
	// the bottom, and a plain (non-gravlift) floor sharing the tile at z=2 -
	// the vanilla data shape that used to block the shaft.
	for (int z = 0; z <= 3; z++)
	{
		spawnPart(map, liftFeature, {0, 0, z});
	}
	spawnPart(map, liftFloorPad, {0, 0, 0});
	spawnPart(map, plainFloor, {0, 0, 2});

	// Control A: a plain floor on its own must still produce solid ground.
	spawnPart(map, plainFloor, {1, 0, 0});

	// Control B: a lift floor pad on its own must never produce solid ground.
	spawnPart(map, liftFloorPad, {2, 0, 0});

	auto shaftZ0 = map.getTile(Vec3<int>{0, 0, 0});
	auto shaftZ1 = map.getTile(Vec3<int>{0, 0, 1});
	auto shaftZ2 = map.getTile(Vec3<int>{0, 0, 2});
	auto controlA = map.getTile(Vec3<int>{1, 0, 0});
	auto controlB = map.getTile(Vec3<int>{2, 0, 0});

	expect(shaftZ2->hasLift && shaftZ2->canStand && !shaftZ2->solidGround,
	       "shaft z=2 (lift + plain floor overlap) must have a lift, be standable, and never be "
	       "solid ground");
	expect(shaftZ0->hasLift && !shaftZ0->solidGround,
	       "shaft z=0 (lift + lift floor pad) must never be solid ground");
	expect(shaftZ1->hasLift && !shaftZ1->solidGround,
	       "shaft z=1 (lift only) must never be solid ground");

	expect(controlA->solidGround && !controlA->hasLift && controlA->canStand,
	       "control A (plain floor only) must remain solid ground");
	expect(controlB->hasLift && !controlB->solidGround,
	       "control B (lift floor pad only) must never be solid ground");

	// Pathing: a small, non-flying unit (no live BattleUnit/Agent needed) must
	// be able to ride the lift straight through the overlap tile in both
	// directions.
	BattleUnitTileHelper smallWalker{map, BattleUnitType::SmallWalker};
	expect(smallWalker.canEnterTile(shaftZ1, shaftZ2),
	       "small unit must be able to ascend through the lift/floor overlap tile");
	expect(smallWalker.canEnterTile(shaftZ2, shaftZ1),
	       "small unit must be able to descend through the lift/floor overlap tile");

	printf("test_gravlift_floor_overlap: all checks passed\n");
	return EXIT_SUCCESS;
}
