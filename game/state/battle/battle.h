#pragma once

#include "game/state/battle/ai/aitype.h"
#include "game/state/battle/ai/tacticalai.h"
#include "game/state/battle/battleforces.h"
#include "game/state/gametime.h"
#include "game/state/rules/agenttype.h"
#include "game/state/rules/battle/battlemapsector.h"
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

static const int MAX_UNITS_PER_SIDE = 36;
static const float MAX_HEARING_DISTANCE = 10.0f;

static const unsigned TICKS_PER_TURN = TICKS_PER_SECOND * 4;
// Amount of ticks that must pass without any action in order for turn end to trigger
static const unsigned TICKS_END_TURN = TICKS_PER_SECOND * 2;
// Amount of ticks that must pass for interrupt to begin
static const unsigned TICKS_BEGIN_INTERRUPT = TICKS_PER_SECOND / 2;
// If vehicle is this close it can receive loot from UFO crash
static const float VEHICLE_NEARBY_THRESHOLD = 5.0f;

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
class BattleScanner;
enum class BattleUnitType;
class Agent;
class BattleUnitTileHelper;

class BattleScore
{
  public:
	int combatRating = 0;
	int casualtyPenalty = 0;
	int getLeadershipBonus();
	int friendlyFire = 0;
	int liveAlienCaptured = 0;
	int equipmentCaptured = 0;
	int equipmentLost = 0;
	int getTotal();
	UString getText();
};

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

	Vec3<int> size;

	StateRef<BattleMap> battle_map;

	std::vector<sp<BattleMapSector::LineOfSightBlock>> losBlocks;
	// Map of vectors of bools, one bool for every tile, denotes visible tiles (same indexing)
	std::map<StateRef<Organisation>, std::vector<bool>> visibleTiles;
	// Map of vectors of bools, one bool for every los block, denotes visible blocks
	std::map<StateRef<Organisation>, std::vector<bool>> visibleBlocks;
	std::map<StateRef<Organisation>, std::set<StateRef<BattleUnit>>> visibleUnits;
	std::map<StateRef<Organisation>, std::set<StateRef<BattleUnit>>> visibleEnemies;

	// Map of vector of flags, whether los block is available for pathfinding for this type
	std::map<BattleUnitType, std::vector<bool>> blockAvailable;
	// Map of vector of positions, those are center positions of a LOS block
	// (for pathfinding calculations) for every kind of unit and every block
	std::map<BattleUnitType, std::vector<Vec3<int>>> blockCenterPos;
	// Vector which contains a 2d array of bools, which denotes whether
	// a connection between two los blocks exists (whether they are adjacent)
	std::vector<bool> linkAvailable;
	// Map of vector which contains a 2d array of ints, which denotes
	// cost of a link (with -1 being no link present)
	std::map<BattleUnitType, std::vector<int>> linkCost;
	// Vector of flags, one for each los block, which denotes whether
	// los block had changed and needs update
	std::vector<bool> blockNeedsUpdate;
	// Vector which contains a 2d array of bools, which denotes whether
	// a connection between two los blocks needs an update
	// Note: technically, this is a 2d array, but we only fill half of it
	// We fill only values for 2nd index which is bigger than 1st
	// Example: If there's a link update required between ids 2 and 3,
	// we will set only [2 + 3 * size] to true
	std::vector<bool> linkNeedsUpdate;

	// Tiles that have something changed inside them and require to re-calculate vision
	// of every soldier who has them in LOS. Triggers include:
	// - Map part changing tiles
	// - Map part dying or getting damaged
	// - Map part which is door opening/closing
	std::set<Vec3<int>> tilesChangedForVision;
	MissionType mission_type = MissionType::AlienExtermination;
	UString mission_location_id;
	Mode mode = Mode::RealTime;
	BattleScore score = {};
	unsigned missionEndTimer = 0;
	bool buildingCanBeDisabled = false;
	bool buildingDisabled = false;
	bool playerWon = false;
	StateRef<Organisation> locationOwner;
	bool loserHasRetreated = false;
	bool winnerHasRetreated = false;

	StateRef<Vehicle> player_craft;

	std::list<sp<BattleMapPart>> map_parts;
	std::list<sp<BattleItem>> items;
	StateRefMap<BattleUnit> units;
	StateRefMap<BattleScanner> scanners;
	std::list<sp<Doodad>> doodads;
	std::set<sp<Projectile>> projectiles;
	StateRefMap<BattleDoor> doors;
	std::set<sp<BattleExplosion>> explosions;
	std::set<sp<BattleHazard>> hazards;

	up<TileMap> map;

	std::list<StateRef<Organisation>> participants;
	std::map<StateRef<Organisation>, int> leadershipBonus;

	AIBlockTactical aiBlock;

	// Current player in control of the interface (will only change if we're going multiplayer)
	StateRef<Organisation> currentPlayer;
	// Who's turn is it
	StateRef<Organisation> currentActiveOrganisation;
	bool hotseat = false;
	// Turn number
	unsigned int currentTurn = 0;
	// Ticks without action in TB
	unsigned int ticksWithoutAction = 0;
	// Ticks without action as ween by org
	std::map<StateRef<Organisation>, unsigned> ticksWithoutSeenAction;
	// Last action location seen by org
	std::map<StateRef<Organisation>, Vec3<int>> lastSeenActionLocation;
	// RF interval (how frequently RFs spawn, in ticks)
	int reinforcementsInterval = 0;
	// RF timer
	int ticksUntilNextReinforcement = 0;

	// Turn end allowed by current org
	bool turnEndAllowed = false;
	// Low morale units were processed at turn start
	bool lowMoraleProcessed = false;
	// Units queued to get interrupt after this update
	std::map<StateRef<BattleUnit>, int> interruptQueue;
	// Units currently interrupting
	std::map<StateRef<BattleUnit>, int> interruptUnits;

	bool skirmish = false;
	std::map<StateRef<Organisation>, float> relationshipsBeforeSkirmish;
	int scoreBeforeSkirmish = 0;

	// BattleView and BattleTileView settings, saved here so that we can return to them

	// Need separate one because float does not work
	int battleViewZLevel = 0;
	Vec3<float> battleViewScreenCenter;
	bool battleViewGroupMove = false;
	int battleViewSquadIndex = -1;
	std::list<StateRef<BattleUnit>> battleViewSelectedUnits;

	// Methods

	Battle() = default;
	~Battle();

	void initBattle(GameState &state, bool first = false);
	void initMap();
	bool initialMapCheck(GameState &state, std::list<StateRef<Agent>> agents);
	void initialMapPartRemoval(GameState &state);
	void initialMapPartLinkUp();

	void initialUnitSpawn(GameState &state);

	void setMode(Mode mode);

	// Queue relevant los blocks for refresh
	void queuePathfindingRefresh(Vec3<int> tile);

	// Move a group of units in formation
	void groupMove(GameState &state, std::list<StateRef<BattleUnit>> &selectedUnits,
	               Vec3<int> targetLocation, int facingDelta = 0, bool demandGiveWay = false,
	               bool useTeleporter = false);

	int getLosBlockID(int x, int y, int z) const;
	bool getVisible(StateRef<Organisation> org, int x, int y, int z) const;
	void setVisible(StateRef<Organisation> org, int x, int y, int z, bool val = true);

	// Queue tile for vision update
	void queueVisionRefresh(Vec3<int> tile);

	// Notify scanners about movement at position
	void notifyScanners(Vec3<int> position);

	// Notify about action happening
	void notifyAction(Vec3<int> location = {-1, -1, -1}, StateRef<BattleUnit> actorUnit = nullptr);

	int killStrandedUnits(GameState &state, StateRef<Organisation> org, bool preview = false);
	void abortMission(GameState &state);
	void checkMissionEnd(GameState &state, bool retreated, bool forceReCheck = false);
	void checkIfBuildingDisabled(GameState &state);
	bool tryDisableBuilding();
	void refreshLeadershipBonus(StateRef<Organisation> org);
	void spawnReinforcements(GameState &state);

	void handleProjectileHit(GameState &state, sp<Projectile> projectile, bool displayDoodad,
	                         bool playSound, bool expired);

	void update(GameState &state, unsigned int ticks);
	void updateTB(GameState &state);
	void updateRT(GameState &state, unsigned int ticks);
	void updateTBBegin(GameState &state);
	void updateTBEnd(GameState &state);

	void updateProjectiles(GameState &state, unsigned int ticks);
	void updateVision(GameState &state);
	void updatePathfinding(GameState &state, unsigned int ticks);

	// Adding objects to battle

	sp<BattleExplosion> addExplosion(GameState &state, Vec3<int> position,
	                                 StateRef<DoodadType> doodadType,
	                                 StateRef<DamageType> damageType, int power, int depletionRate,
	                                 StateRef<Organisation> ownerOrg,
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
	sp<BattleHazard> placeHazard(GameState &state, StateRef<Organisation> owner,
	                             StateRef<BattleUnit> unit, StateRef<DamageType> type,
	                             Vec3<int> position, int ttl, int power,
	                             int initialAgeTTLDivizor = 1, bool delayVisibility = true);
	sp<BattleScanner> addScanner(GameState &state, AEquipment &item);
	void removeScanner(GameState &state, AEquipment &item);

	static void accuracyAlgorithmBattle(GameState &state, Vec3<float> firePosition,
	                                    Vec3<float> &target, int accuracy, bool cloaked,
	                                    bool thrown = false);

	// Turn based functions

	// Begin new org's turn
	void beginTurn(GameState &state);
	// End current org's turn
	void endTurn(GameState &state);
	// Give interrupt chance to hostile units that see this unit
	void giveInterruptChanceToUnits(GameState &state, StateRef<BattleUnit> giver,
	                                int reactionValue);
	// Give interrupt chance to a unit
	void giveInterruptChanceToUnit(GameState &state, StateRef<BattleUnit> giver,
	                               StateRef<BattleUnit> receiver, int reactionValue);

	// Battle Start Functions

	// To be called when battle in a ufo must be created, before showing battle briefing screen
	static void beginBattle(GameState &state, bool hotseat, StateRef<Organisation> opponent,
	                        std::list<StateRef<Agent>> &player_agents,
	                        const std::map<StateRef<AgentType>, int> *aliens,
	                        StateRef<Vehicle> player_craft, StateRef<Vehicle> target_craft);

	// To be called when battle in a building must be created, before showing battle briefing screen
	static void beginBattle(GameState &state, bool hotseat, StateRef<Organisation> opponent,
	                        std::list<StateRef<Agent>> &player_agents,
	                        const std::map<StateRef<AgentType>, int> *aliens, const int *guards,
	                        const int *civilians, StateRef<Vehicle> player_craft,
	                        StateRef<Building> target_building);

	// To be called when battle must be started, after briefing and squad assign screen
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
	                                      bool ignoreMovingUnits = true,
	                                      int iterationLimitDirect = 0, bool forceDirect = false,
	                                      bool ignoreAllUnits = false, float *cost = nullptr,
	                                      float maxCost = 0.0f);

	// Find path over the graph of los blocks
	std::list<int> findLosBlockPath(int origin, int destination, BattleUnitType type,
	                                int iterationLimit = 1000);

  private:
	// The part of findShortestPath that uses LBs
	std::list<Vec3<int>> findShortestPathUsingLB(
	    Vec3<int> origin, Vec3<int> destination, const BattleUnitTileHelper &canEnterTile,
	    bool approachOnly = false, bool ignoreStaticUnits = false, bool ignoreMovingUnits = true,
	    bool ignoreAllUnits = false, float *cost = nullptr, float maxCost = 0.0f);

  private:
	void loadResources(GameState &state);
	void unloadResources(GameState &state);

	void loadImagePacks(GameState &state);
	void unloadImagePacks(GameState &state);

	void loadAnimationPacks(GameState &state);
	void unloadAnimationPacks(GameState &state);

	friend class BattleMap;

  public:
	// Following members are not serialized

	// Timestamp when we last saw a hostile unit
	// We are not saving it because when player loads he may have no recollection of which
	// enemies are hiding where, and so we need to pop notifications again for every enemy
	std::map<StateRef<Organisation>, std::map<StateRef<BattleUnit>, uint64_t>> lastVisibleTime;

	// List of locations for priority spawning of specific agent types
	// we cannot save game in briefing screen so this is not saved
	std::map<StateRef<AgentType>, std::list<Vec3<int>>> spawnLocations;

	// We cannot save game in debriefing screen so these debriefing parameters are not saved

	// List of promoted units
	std::list<StateRef<BattleUnit>> unitsPromoted;
	// List of bio-containment loot
	std::map<StateRef<AEquipmentType>, int> bioLoot;
	// List of cargo bay loot
	std::map<StateRef<AEquipmentType>, int> cargoLoot;

	// Following members are not serialized, but rather are set in initBattle method

	std::set<Vec3<int>> exits;

	sp<BattleCommonImageList> common_image_list;
	sp<BattleCommonSampleList> common_sample_list;

	std::map<StateRef<Organisation>, BattleForces> forces;

	// Vector of indexes to los blocks, each block appears multiple times according to its priority
	std::vector<int> losBlockRandomizer;
	// Vector of indexes to los blocks, for each tile (index is like tile's location in tilemap)
	std::vector<int> tileToLosBlock;
};

}; // namespace OpenApoc
