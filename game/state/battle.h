#pragma once
#include "framework/includes.h"
#include "game/state/battlemappart_type.h"
#include "game/state/stateobject.h"
#include "game/state/tileview/tile.h"
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
class TileMap;
class BattleMapPart;
class BattleUnit;
// class BattleProjectile;
// class BattleDoodad;

class Battle
{
  public:
	enum class MapBorder
	{
		North,
		East,
		South,
		West
	};

	enum class MissionType
	{
		AlienExtermination,
		RaidAliens,
		BaseDefense,
		RaidHumans,
		UfoRecovery,
	};

	Battle() = default;
	~Battle();

	void initMap();

	Vec3<int> size;

	StateRef<BattleMapPartType> destroyed_ground_tile;
	std::vector<StateRef<BattleMapPartType>> rubble_left_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_right_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_feature;

	MissionType mission_type = MissionType::AlienExtermination;
	UString mission_location_id;

	StateRef<Vehicle> player_craft;

	std::set<sp<BattleMapPart>> map_parts;
	// std::set<sp<BattleProjectile>> projectiles;
	// std::set<sp<BattleUnit>> units;
	// std::set<sp<BattleDoodad>> doodads;

	up<TileMap> map;

	void update(GameState &state, unsigned int ticks);
};

}; // namespace OpenApoc
