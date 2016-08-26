#pragma once
#include "framework/includes.h"
#include "game/state/battlemappart_type.h"
#include "game/state/battletile.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include "library/voxel.h"

namespace OpenApoc
{
#define BATTLE_TILE_X (48)
#define BATTLE_TILE_Y (24)
#define BATTLE_TILE_Z (40)

#define BATTLE_STRAT_TILE_X 8
#define BATTLE_STRAT_TILE_Y 8

class GameState;
class BattleTileMap;
class BattleMapPart;

class Battle
{
  public:
	Battle() = default;
	~Battle();

	void start();
	void initMap();

	Vec3<int> size;
	std::map<UString, sp<BattleMapPartType>> map_part_types;

	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_grounds;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_left_walls;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_right_walls;
	std::map<Vec3<int>, StateRef<BattleMapPartType>> initial_scenery;

	std::set<sp<BattleMapPart>> map_parts;

	up<BattleTileMap> map;

	void update(GameState &state, unsigned int ticks);
};

}; // namespace OpenApoc
