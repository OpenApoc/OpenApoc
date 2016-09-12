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

#define BATTLE_VOXEL_X (24)
#define BATTLE_VOXEL_Y (24)
#define BATTLE_VOXEL_Z (20)

#define BATTLE_STRAT_TILE_X 8
#define BATTLE_STRAT_TILE_Y 8

class GameState;
class TileMap;
class BattleMapPart;
class BattleUnit;
class BattleItem;
class Projectile;
class Doodad;
class DoodadType;

class Battle : public std::enable_shared_from_this<Battle>
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

	void initBattle();
	void initMap();

	Vec3<int> size;

	StateRef<BattleMapPartType> destroyed_ground_tile;
	std::vector<StateRef<BattleMapPartType>> rubble_left_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_right_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_feature;

	MissionType mission_type = MissionType::AlienExtermination;
	UString mission_location_id;

	StateRef<Vehicle> player_craft;

	std::list<sp<BattleMapPart>> map_parts;
	std::list<sp<BattleItem>> items;
	std::list<sp<BattleUnit>> units;
	std::list<sp<Doodad>> doodads;

	std::set<sp<Projectile>> projectiles;

	up<TileMap> map;

	void update(GameState &state, unsigned int ticks);
	sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);
};

}; // namespace OpenApoc
