#pragma once
#include "framework/includes.h"
#include "game/state/agent.h"
#include "game/state/battle/battleforces.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/battle/battlemapsector.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include "library/voxel.h"

namespace OpenApoc
{
#define TILE_X_BATTLE (48)
#define TILE_Y_BATTLE (24)
#define TILE_Z_BATTLE (40)

#define VOXEL_X_BATTLE (24)
#define VOXEL_Y_BATTLE (24)
#define VOXEL_Z_BATTLE (20)

class BattleCommonImageList;
class BattleCommonSampleList;
class GameState;
class TileMap;
class BattleMapPart;
class BattleUnit;
class BattleDoor;
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

	enum class Mode
	{
		RealTime,
		TurnBased
	};

	Battle() = default;
	~Battle();

	void initBattle(GameState &state);
	void initMap();

	Vec3<int> size;

	StateRef<BattleMap> battle_map;

	std::list<sp<BattleMapSector::LineOfSightBlock>> los_blocks;

	MissionType mission_type = MissionType::AlienExtermination;
	UString mission_location_id;
	Mode mode = Mode::RealTime;

	StateRef<Vehicle> player_craft;

	std::list<sp<BattleMapPart>> map_parts;
	std::list<sp<BattleItem>> items;
	StateRefMap<BattleUnit> units;
	std::list<sp<Doodad>> doodads;
	std::set<sp<Projectile>> projectiles;
	StateRefMap<BattleDoor> doors;

	up<TileMap> map;

	std::set<StateRef<Organisation>> participants;

	// Current player in control of the interface (will only change if we're going multiplayer)
	StateRef<Organisation> currentPlayer;

	// Who's turn is it
	StateRef<Organisation> currentActiveOrganisation;

	// Turn number
	int currentTurn = 0;

	// Store information about last screen center location, so that when we load a save
	// we could restore it
	int battleviewZLevel;
	Vec3<float> battleviewScreenCenter;

	// Contains height at which to spawn units, or -1 if spawning is not possible
	// No need to serialize this, as we cannot save/load during briefing
	std::vector<std::vector<std::vector<int>>> spawnMap;

	// Following members are not serialized, but rather are set in initBattle method

	sp<BattleCommonImageList> common_image_list;
	sp<BattleCommonSampleList> common_sample_list;

	std::map<StateRef<Organisation>, BattleForces> forces;

	void update(GameState &state, unsigned int ticks);
	sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);

	// Adds unit, setting it's id
	UString addUnit();

	// Adds door, setting it's id
	UString addDoor();

	// Turn based functions

	// Called when new organisation's turn is starting
	void beginTurn();

	// Called when current active organisation decides to end their turn
	void endTurn();

	// Battle Start Functions

	// To be called when battle must be created, before showing battle briefing screen
	static void beginBattle(GameState &state, StateRef<Organisation> target_organisation,
	                        std::list<StateRef<Agent>> &player_agents,
	                        StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft);

	// To be called when battle must be created, before showing battle briefing screen
	static void beginBattle(GameState &state, StateRef<Organisation> target_organisation,
	                        std::list<StateRef<Agent>> &player_agents,
	                        StateRef<Vehicle> player_craft, StateRef<Building> target_building);

	// To be called when battle must be started, after briefing screen
	static void enterBattle(GameState &state);

	// Battle End Functions

	// To be called when battle must be finished, before showing score screen
	static void finishBattle(GameState &state);

	// To be called after battle was finished and before returning to cityscape
	static void exitBattle(GameState &state);

  private:
	void loadResources(GameState &state);
	void unloadResources(GameState &state);

	void loadImagePacks(GameState &state);
	void unloadImagePacks(GameState &state);

	void loadAnimationPacks(GameState &state);
	void unloadAnimationPacks(GameState &state);

	friend class BattleMap;
};

}; // namespace OpenApoc
