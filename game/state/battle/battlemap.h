#pragma once

#include "game/state/tileview/tile.h"
#include "game/state/battle/battle.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include <list>
#include <map>
#include <set>
#include <vector>

namespace OpenApoc
{

class Building;
class Agent;
class Organisation;
class Vehicle;
class BattleMapPartType;
class BattleMapSector;

class BattleMap : public StateObject<BattleMap>
{
  public:
	BattleMap();
	~BattleMap() = default;

	UString id;

	Vec3<int> chunk_size;
	Vec3<int> max_battle_size;

	std::map<MapDirection, bool> allow_entrance;
	std::map<MapDirection, bool> allow_exit;
	int entrance_level_min = 0;
	int entrance_level_max = 0;
	int exit_level_min = 0;
	int exit_level_max = 0;

	std::set<Vec3<int>> exitsX;
	std::set<Vec3<int>> exitsY;

	std::list<UString> tilesets;
	StateRef<BattleMapPartType> destroyed_ground_tile;
	std::vector<StateRef<BattleMapPartType>> rubble_left_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_right_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_feature;
	std::vector<StateRef<BattleMapPartType>> exit_grounds;

	StateRefMap<BattleMapSector> sectors;

  private:
	static sp<Battle> createBattle(GameState &state, StateRef<Organisation> target_organisation,
	                               std::list<StateRef<Agent>> &player_agents,
	                               StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft);

	static sp<Battle> createBattle(GameState &state, StateRef<Organisation> target_organisation,
	                               std::list<StateRef<Agent>> &player_agents,
	                               StateRef<Vehicle> player_craft,
	                               StateRef<Building> target_building);

	sp<Battle> createBattle(GameState &state, StateRef<Organisation> target_organisation,
	                        std::list<StateRef<Agent>> &agents, StateRef<Vehicle> player_craft,
	                        Battle::MissionType mission_type, UString mission_location_id);

	void loadTilesets(GameState &state) const;
	static void unloadTilesets(GameState &state);

	friend class Battle;
};
}
