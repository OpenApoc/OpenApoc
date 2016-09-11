#pragma once
#include "game/state/agent.h"
#include "game/state/agent.h"
#include "game/state/battle.h"
#include "game/state/battlemappart_type.h"
#include "game/state/battlemapsector.h"
#include "game/state/city/building.h"
#include "game/state/organisation.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include <map>

namespace OpenApoc
{

class Building;

class BattleMap : public StateObject<BattleMap>
{
  public:
	BattleMap();
	~BattleMap() = default;

	UString id;

	Vec3<int> chunk_size;
	Vec3<int> max_battle_size;

	std::map<Battle::MapBorder, bool> allow_entrance;
	std::map<Battle::MapBorder, bool> allow_exit;
	int entrance_level_min = 0;
	int entrance_level_max = 0;
	int exit_level_min = 0;
	int exit_level_max = 0;

	std::list<Vec3<int>> exits;

	std::list<UString> tilesets;
	StateRef<BattleMapPartType> destroyed_ground_tile;
	std::vector<StateRef<BattleMapPartType>> rubble_left_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_right_wall;
	std::vector<StateRef<BattleMapPartType>> rubble_feature;

	StateRefMap<BattleMapSector> sectors;

	static sp<Battle> CreateBattle(GameState &state, StateRef<Organisation> target_organisation,
	                               const std::list<StateRef<Agent>> &player_agents,
	                               StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft);
	static sp<Battle> CreateBattle(GameState &state, StateRef<Organisation> target_organisation,
	                               const std::list<StateRef<Agent>> &player_agents,
	                               StateRef<Vehicle> player_craft,
	                               StateRef<Building> target_building);

  private:
	sp<Battle> CreateBattle(GameState &state, StateRef<Organisation> target_organisation,
	                        const std::list<StateRef<Agent>> &player_agents,
	                        StateRef<Vehicle> player_craft, Battle::MissionType mission_type,
	                        UString mission_location_id,
	                        const std::list<StateRef<Agent>> &target_agents);
};
}
