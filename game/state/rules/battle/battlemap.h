#pragma once

#include "game/state/battle/battle.h"
#include "game/state/stateobject.h"
#include "game/state/tilemap/tilemap.h"
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
	// Different ways to alter map size for generation
	enum class GenerationSize
	{
		// Subtract 1 from larger map side
		Small = 0,
		// Standard size
		Normal = 1,
		// Add 1 to random map side
		Large = 2,
		// Add 2 to random map side
		VeryLarge = 3,
		// Add 2 to smaller map side
		Maximum = 4
	};

	BattleMap() = default;
	~BattleMap() override = default;

	UString id;

	Vec3<int> chunk_size = {0, 0, 0};
	Vec3<int> max_battle_size = {0, 0, 0};

	std::map<MapDirection, bool> allow_entrance;
	std::map<MapDirection, bool> allow_exit;
	int entrance_level_min = 0;
	int entrance_level_max = 0;
	int exit_level_min = 0;
	int exit_level_max = 0;

	std::set<Vec3<int>> exitsX;
	std::set<Vec3<int>> exitsY;

	int reinforcementsInterval = 0;

	std::list<UString> tilesets;
	StateRef<BattleMapPartType> destroyed_ground_tile;
	std::vector<StateRef<BattleMapPartType>> rubble_left_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_right_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_feature;
	std::vector<StateRef<BattleMapPartType>> exit_grounds;

	StateRefMap<BattleMapSector> sectors;

  private:
	static sp<Battle> createBattle(GameState &state, StateRef<Organisation> opponent,
	                               std::list<StateRef<Agent>> &player_agents,
	                               const std::map<StateRef<AgentType>, int> *aliens,
	                               StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft);

	static sp<Battle> createBattle(GameState &state, StateRef<Organisation> opponent,
	                               std::list<StateRef<Agent>> &player_agents,
	                               const std::map<StateRef<AgentType>, int> *aliens,
	                               const int *guards, const int *civilians,
	                               StateRef<Vehicle> player_craft,
	                               StateRef<Building> target_building);

	sp<Battle> createBattle(GameState &state, StateRef<Organisation> propertyOwner,
	                        StateRef<Organisation> opponent, std::list<StateRef<Agent>> &agents,
	                        StateRef<Vehicle> player_craft, Battle::MissionType mission_type,
	                        UString mission_location_id);

	bool generateMap(std::vector<sp<BattleMapSector>> &sec_map, Vec3<int> &size, GameState &state,
	                 GenerationSize genSize);

	bool generateBase(std::vector<sp<BattleMapSector>> &sec_map, Vec3<int> &size, GameState &state,
	                  UString mission_location_id);

	sp<Battle> fillMap(std::vector<std::list<std::pair<Vec3<int>, sp<BattleMapPart>>>> &doors,
	                   bool &spawnCivilians, std::vector<sp<BattleMapSector>> sec_map,
	                   Vec3<int> size, GameState &state, StateRef<Organisation> propertyOwner,
	                   StateRef<Organisation> target_organisation,
	                   std::list<StateRef<Agent>> &agents, StateRef<Vehicle> player_craft,
	                   Battle::MissionType mission_type, UString mission_location_id);

	void linkDoors(sp<Battle> b,
	               std::vector<std::list<std::pair<Vec3<int>, sp<BattleMapPart>>>> doors,
	               GameState &state);

	void fillSquads(sp<Battle> b, bool spawnCivilians, GameState &state,
	                std::list<StateRef<Agent>> &agents);

	void initNewMap(sp<Battle> b);

	void unloadTiles();

	void loadTilesets(GameState &state) const;
	static void unloadTilesets(GameState &state);

	friend class Battle;
};
} // namespace OpenApoc
