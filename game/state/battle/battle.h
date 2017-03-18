#pragma once

#include "game/state/agent.h"
#include "game/state/battle/ai/ai.h"
#include "game/state/battle/battleforces.h"
#include "game/state/battle/battlemapsector.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <set>
#include <vector>

namespace OpenApoc
{
#define TILE_X_BATTLE (48)
#define TILE_Y_BATTLE (24)
#define TILE_Z_BATTLE (40)

#define VOXEL_X_BATTLE (24)
#define VOXEL_Y_BATTLE (24)
#define VOXEL_Z_BATTLE (20)

static const unsigned TICKS_PER_TURN = TICKS_PER_SECOND * 4;

class BattleCommonImageList;
class BattleCommonSampleList;
class GameState;
class TileMap;
class BattleMapPart;
class BattleUnit;
class AEquipment;
class BattleDoor;
class BattleItem;
class BattleExplosion;
class BattleHazard;
class DamageType;
class Projectile;
class Doodad;
class DoodadType;
class BattleMap;
class Vehicle;
class Building;
class Agent;
enum class BattleUnitType;
class BattleUnitTileHelper;

class Battle : public std::enable_shared_from_this<Battle>
{
  public:
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

	void initBattle(GameState &state, bool first = false);
	void initMap();
	void initialMapPartLinkUp();

	Vec3<int> size;

	StateRef<BattleMap> battle_map;

	std::vector<sp<BattleMapSector::LineOfSightBlock>> losBlocks;
	// Map of vectors of bools, one bool for every tile, denotes visible tiles (same indexing)
	std::map<StateRef<Organisation>, std::vector<bool>> visibleTiles;
	// Map of vectors of bools, one bool for every los block, denotes visible blocks
	std::map<StateRef<Organisation>, std::vector<bool>> visibleBlocks;
	std::map<StateRef<Organisation>, std::set<StateRef<BattleUnit>>> visibleUnits;
	std::map<StateRef<Organisation>, std::set<StateRef<BattleUnit>>> visibleEnemies;

	// Map of vector of flags, wether los block is available for pathfinding for this type
	std::map<BattleUnitType, std::vector<bool>> blockAvailable;
	// Map of vector of positions, those are center positions of a LOS block
	// (for pathfinding calculations) for every kind of unit and every block
	std::map<BattleUnitType, std::vector<Vec3<int>>> blockCenterPos;
	// Vector which contains a 2d array of bools, which denotes wether
	// a connection between two los blocks exists (wether they are adjacent)
	std::vector<bool> linkAvailable;
	// Map of vector which contains a 2d array of ints, which denotes
	// cost of a link (with -1 being no link present)
	std::map<BattleUnitType, std::vector<int>> linkCost;
	// Vector of flags, one for each los block, which deontes wether
	// los block had changed and needs update
	std::vector<bool> blockNeedsUpdate;
	// Vector which contains a 2d array of bools, which denotes wether
	// a connection between two los blocks needs an update
	// Note: technically, this is a 2d array, but we only fill half of it
	// We fill only values for 2nd index which is bigger than 1st
	// Example: If there's a link update required between ids 2 and 3,
	// we will set only [2 + 3 * size] to true
	std::vector<bool> linkNeedsUpdate;
	// Queue relevant los blocks for refresh
	void queuePathfindingRefresh(Vec3<int> tile);

	// Move a group of units in formation
	static void groupMove(GameState &state, std::list<StateRef<BattleUnit>> &selectedUnits,
	                      Vec3<int> targetLocation, int facingDelta = 0, bool demandGiveWay = false,
	                      bool useTeleporter = false);

	int getLosBlockID(int x, int y, int z) const;
	bool getVisible(StateRef<Organisation> org, int x, int y, int z) const;
	void setVisible(StateRef<Organisation> org, int x, int y, int z, bool val = true);
	// Tiles that have something changed inside them and require to re-calculate vision
	// of every soldier who has them in LOS. Triggers include:
	// - Map part changing tiles
	// - Map part dying or getting damaged
	// - Map part which is door opening/closing
	std::set<Vec3<int>> tilesChangedForVision;
	// Queue tile for vision update
	void queueVisionRefresh(Vec3<int> tile);

	MissionType mission_type = MissionType::AlienExtermination;
	UString mission_location_id;
	Mode mode = Mode::RealTime;
	void setMode(Mode mode);

	StateRef<Vehicle> player_craft;

	std::list<sp<BattleMapPart>> map_parts;
	std::list<sp<BattleItem>> items;
	StateRefMap<BattleUnit> units;
	std::list<sp<Doodad>> doodads;
	std::set<sp<Projectile>> projectiles;
	StateRefMap<BattleDoor> doors;
	std::set<sp<BattleExplosion>> explosions;
	std::set<sp<BattleHazard>> hazards;

	up<TileMap> map;

	std::set<StateRef<Organisation>> participants;

	TacticalAIBlock aiBlock;

	// Current player in control of the interface (will only change if we're going multiplayer)
	StateRef<Organisation> currentPlayer;

	// Who's turn is it
	StateRef<Organisation> currentActiveOrganisation;

	// Turn number
	int currentTurn = 0;

	// BattleView and BattleTileView settings, saved here so that we can return to them

	int battleViewZLevel = 0;
	Vec3<float> battleViewScreenCenter;
	bool battleViewGroupMove = false;
	std::list<StateRef<BattleUnit>> battleViewSelectedUnits;

	void update(GameState &state, unsigned int ticks);

	void updateProjectiles(GameState &state, unsigned int ticks);
	void updateVision(GameState &state);
	void updatePathfinding(GameState &state);

	// Adding objects to battle

	sp<BattleExplosion> addExplosion(GameState &state, Vec3<int> position,
	                                 StateRef<DoodadType> doodadType,
	                                 StateRef<DamageType> damageType, int power, int depletionRate,
	                                 StateRef<BattleUnit> ownerUnit = nullptr);
	sp<BattleDoor> addDoor(GameState &state);
	sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);
	sp<BattleUnit> spawnUnit(GameState &state, StateRef<Organisation> owner,
	                         StateRef<AgentType> agentType, Vec3<float> position,
	                         Vec2<int> facing = {0, 0}, BodyState curState = BodyState::Standing,
	                         BodyState tarState = BodyState::Standing);
	sp<BattleUnit> placeUnit(GameState &state, StateRef<Agent> agent);
	sp<BattleUnit> placeUnit(GameState &state, StateRef<Agent> agent, Vec3<float> position);
	sp<BattleItem> placeItem(GameState &state, sp<AEquipment> item, Vec3<float> position);
	sp<BattleHazard> placeHazard(GameState &state, StateRef<DamageType> type, Vec3<int> position,
	                             int ttl, int power, int initialAgeTTLDivizor = 1,
	                             bool delayVisibility = true);

	static void accuracyAlgorithmBattle(GameState &state, Vec3<float> firePosition,
	                                    Vec3<float> &target, int accuracy, bool thrown = false);

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

	// Pathfinding functions

  public:
	// Find shortest path, using los blocks as a guide if going far
	std::list<Vec3<int>> findShortestPath(Vec3<int> origin, Vec3<int> destination,
	                                      const BattleUnitTileHelper &canEnterTile,
	                                      bool approachOnly = false, bool ignoreStaticUnits = false,
	                                      int iterationLimitDirect = 0, bool forceDirect = false,
	                                      bool ignoreAllUnits = false, float *cost = nullptr,
	                                      float maxCost = 0.0f);

	// Find path over the graph of los blocks
	std::list<int> findLosBlockPath(int origin, int destination, BattleUnitType type,
	                                int iterationLimit = 1000);

  private:
	// The part of findShortestPath that uses LBs
	std::list<Vec3<int>> findShortestPathUsingLB(Vec3<int> origin, Vec3<int> destination,
	                                             const BattleUnitTileHelper &canEnterTile,
	                                             bool approachOnly = false,
	                                             bool ignoreStaticUnits = false,
	                                             bool ignoreAllUnits = false, float *cost = nullptr,
	                                             float maxCost = 0.0f);

  private:
	void loadResources(GameState &state);
	void unloadResources(GameState &state);

	void loadImagePacks(GameState &state);
	void unloadImagePacks(GameState &state);

	void loadAnimationPacks(GameState &state);
	void unloadAnimationPacks(GameState &state);

	friend class BattleMap;

  public:
	// Following members are not serialized, but rather are set in initBattle method

	sp<BattleCommonImageList> common_image_list;
	sp<BattleCommonSampleList> common_sample_list;

	std::map<StateRef<Organisation>, BattleForces> forces;

	// Vector of indexes to los blocks, each block appears multiple times according to its priority
	std::vector<int> losBlockRandomizer;
	// Vector of indexes to los blocks, for each tile (index is like tile's location in tilemap)
	std::vector<int> tileToLosBlock;

	// Contains height at which to spawn units, or -1 if spawning is not possible
	// No need to serialize this, as we cannot save/load during briefing
	std::vector<std::vector<std::vector<int>>> spawnMap;
};

}; // namespace OpenApoc
