#pragma once
#include "game/state/agent.h"
#include "framework/includes.h"
#include "game/state/battle/battleforces.h"
#include "game/state/battle/battlemapsector.h"
#include "game/state/battle/battlemappart_type.h"
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
class BattleMap;
class Vehicle;
class Building;

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

	void initBattle(GameState &state);
	void initMap();

	Vec3<int> size;

	StateRef<BattleMap> battle_map;

	std::vector<sp<BattleMapSector::LineOfSightBlock>> los_blocks;

	MissionType mission_type = MissionType::AlienExtermination;
	UString mission_location_id;

	StateRef<Vehicle> player_craft;

	std::list<sp<BattleMapPart>> map_parts;
	std::list<sp<BattleItem>> items;
	std::list<sp<BattleUnit>> units;
	std::list<sp<Doodad>> doodads;
	std::set<sp<Projectile>> projectiles;

	up<TileMap> map;

	std::set<StateRef<Organisation>> participants;

	// Following members are not serialized, but rather are set in initBattle method

	std::map<StateRef<Organisation>, BattleForces> forces;

	void update(GameState &state, unsigned int ticks);
	sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);

	// To be called when battle must be created, before showing battle briefing screen
	static void EnterBattle(GameState &state, StateRef<Organisation> target_organisation,
		std::list<StateRef<Agent>> &player_agents,
		StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft);
	
	// To be called when battle must be created, before showing battle briefing screen
	static void EnterBattle(GameState &state, StateRef<Organisation> target_organisation,
		std::list<StateRef<Agent>> &player_agents,
		StateRef<Vehicle> player_craft,
		StateRef<Building> target_building);
	
	// To be called when battle must be started, after briefing screen
	static void BeginBattle(GameState &state);

	// To be called when battle must be finished, before showing score screen
	static void FinishBattle(GameState &state);

	// To be called after battle was finished and before returning to cityscape
	static void ExitBattle(GameState &state);

  private:

	void LoadResources(GameState &state);
	void UnloadResources(GameState &state);

	void LoadImagePacks(GameState &state);
	void UnloadImagePacks(GameState &state);

	void LoadAnimationPacks(GameState &state);
	void UnloadAnimationPacks(GameState &state);

	friend class BattleMap;
};

}; // namespace OpenApoc
