// Verifies the GravliftLineOfFire option gate on
// TileObjectBattleMapPart::getVoxelMap. Uses synthetic map part types (no
// extracted tileset/gamestate data required), so this runs under normal CI
// unlike the manual full-battle harnesses for issue #1330.
#include "framework/configfile.h"
#include "framework/logger.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "library/voxel.h"

using namespace OpenApoc;

namespace
{

sp<VoxelMap> makeSolidVoxelMap()
{
	auto slice = mksp<VoxelSlice>(Vec2<int>{4, 4});
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++)
			slice->setBit({x, y}, true);
	auto map = mksp<VoxelMap>(Vec3<int>{4, 4, 4});
	for (int z = 0; z < 4; z++)
		map->setSlice(z, slice);
	return map;
}

StateRef<BattleMapPartType> registerType(GameState &state, const UString &id, bool floor,
                                         bool gravlift, sp<VoxelMap> voxelMap)
{
	auto type = mksp<BattleMapPartType>();
	type->type = BattleMapPartType::Type::Ground;
	type->floor = floor;
	type->gravlift = gravlift;
	type->voxelMapLOF = voxelMap;
	type->voxelMapLOS = voxelMap;
	state.battleMapTiles[id] = type;
	return {&state, id};
}

sp<TileObjectBattleMapPart> spawnPart(TileMap &map, StateRef<BattleMapPartType> type,
                                      Vec3<float> position)
{
	auto part = mksp<BattleMapPart>();
	part->type = type;
	part->position = position;
	map.addObjectToMap(part);
	return part->tileObject;
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
	TileMap map{{4, 4, 4}, {1, 1, 1}, {4, 4, 4}, {{TileObject::Type::Ground}}};
	auto voxelMap = makeSolidVoxelMap();

	auto gravliftFloorType =
	    registerType(*state, "BATTLEMAPPART_TEST_GRAVLIFT_FLOOR", true, true, voxelMap);
	auto plainFloorType =
	    registerType(*state, "BATTLEMAPPART_TEST_PLAIN_FLOOR", true, false, voxelMap);
	auto gravliftWallType =
	    registerType(*state, "BATTLEMAPPART_TEST_GRAVLIFT_WALL", false, true, voxelMap);

	auto gravliftFloor = spawnPart(map, gravliftFloorType, {0.5f, 0.5f, 0.5f});
	auto plainFloor = spawnPart(map, plainFloorType, {1.5f, 0.5f, 0.5f});
	auto gravliftWall = spawnPart(map, gravliftWallType, {2.5f, 0.5f, 0.5f});

	// Flag OFF must be byte-identical to master: every piece blocks LOS/LOF
	// regardless of floor/gravlift combination.
	config().set("OpenApoc.NewFeature.GravliftLineOfFire", false);
	expect(gravliftFloor->getVoxelMap({}, true) == voxelMap,
	       "flag off: gravlift floor pad must still block LOS");
	expect(gravliftFloor->getVoxelMap({}, false) == voxelMap,
	       "flag off: gravlift floor pad must still block LOF");
	expect(plainFloor->getVoxelMap({}, true) == voxelMap, "flag off: plain floor must block LOS");
	expect(gravliftWall->getVoxelMap({}, false) == voxelMap,
	       "flag off: gravlift wall (non-floor) must block LOF");

	// Flag ON exempts only pieces that are BOTH a floor AND a gravlift.
	config().set("OpenApoc.NewFeature.GravliftLineOfFire", true);
	expect(gravliftFloor->getVoxelMap({}, true) == nullptr,
	       "flag on: gravlift floor pad must not block LOS");
	expect(gravliftFloor->getVoxelMap({}, false) == nullptr,
	       "flag on: gravlift floor pad must not block LOF");
	expect(plainFloor->getVoxelMap({}, true) == voxelMap,
	       "flag on: non-gravlift floor must still block LOS");
	expect(plainFloor->getVoxelMap({}, false) == voxelMap,
	       "flag on: non-gravlift floor must still block LOF");
	expect(gravliftWall->getVoxelMap({}, true) == voxelMap,
	       "flag on: gravlift wall (not a floor) must still block LOS");
	expect(gravliftWall->getVoxelMap({}, false) == voxelMap,
	       "flag on: gravlift wall (not a floor) must still block LOF");

	// The pre-existing "falling map parts don't break LOS" exemption must
	// still work independently of this flag.
	config().set("OpenApoc.NewFeature.GravliftLineOfFire", false);
	plainFloor->getOwner()->falling = true;
	expect(plainFloor->getVoxelMap({}, true) == nullptr,
	       "falling map part must not block LOS regardless of the flag");
	expect(plainFloor->getVoxelMap({}, false) == voxelMap,
	       "falling map part must still block LOF (falling only exempts LOS)");

	printf("test_gravlift_lineoffire: all checks passed\n");
	return EXIT_SUCCESS;
}
