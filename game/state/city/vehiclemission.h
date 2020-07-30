#pragma once

#include "game/state/rules/city/vehicletype.h"
#include "game/state/stateobject.h"

#include "game/state/tilemap/tilemap.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <math.h>

namespace OpenApoc
{

static const int TELEPORTER_SPREAD = 10;
static const int DIMENSION_GATE_DELAY = TICKS_PER_SECOND / 2;
static const int FOLLOW_BOUNDS_XY = 4;
static const int FOLLOW_BOUNDS_Z = 1;
static const int TARGET_BUILDING_DISTANCE_LIMIT = 30;
static const float FOLLOW_RANGE =
    sqrtf(2 * FOLLOW_BOUNDS_XY * FOLLOW_BOUNDS_XY + FOLLOW_BOUNDS_Z * FOLLOW_BOUNDS_Z);

class Vehicle;
class Tile;
class TileMap;
class Building;
class City;

class FlyingVehicleTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	const Vehicle &v;
	Vec2<int> size;
	bool large;
	int altitude;

  public:
	FlyingVehicleTileHelper(TileMap &map, const Vehicle &v);

	bool canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits = false,
	                  bool ignoreMovingUnits = true, bool ignoreAllUnits = false) const override;

	float pathOverheadAlloawnce() const override;

	// Support 'from' being nullptr for if a vehicle is being spawned in the map
	bool canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &, bool, bool,
	                  bool) const override;

	float adjustCost(Vec3<int> nextPosition, int z) const override;

	float getDistance(Vec3<float> from, Vec3<float> to) const override;
	float getDistance(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd) const override;

	static float getDistanceStatic(Vec3<float> from, Vec3<float> to);
	static float getDistanceStatic(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd);

	bool canLandOnTile(Tile *to) const;

	Vec3<int> findTileToLandOn(GameState &, sp<TileObjectVehicle> vTile) const;

	Vec3<float> findSidestep(GameState &state, sp<TileObjectVehicle> vTile,
	                         sp<TileObjectVehicle> targetTile, float distancePref) const;
};

class GroundVehicleTileHelper : public CanEnterTileHelper
{
  private:
	TileMap &map;
	VehicleType::Type type;

  public:
	GroundVehicleTileHelper(TileMap &map, Vehicle &v);
	GroundVehicleTileHelper(TileMap &map, VehicleType::Type type);

	bool canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits = false,
	                  bool ignoreMovingUnits = true, bool ignoreAllUnits = false) const override;

	float pathOverheadAlloawnce() const override;

	// Support 'from' being nullptr for if a vehicle is being spawned in the map
	bool canEnterTile(Tile *from, Tile *to, bool, bool &, float &cost, bool &, bool, bool,
	                  bool) const override;

	float getDistance(Vec3<float> from, Vec3<float> to) const override;
	float getDistance(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd) const override;

	static float getDistanceStatic(Vec3<float> from, Vec3<float> to);
	static float getDistanceStatic(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd);

	// Convert vector direction into index for tube array
	int convertDirection(Vec3<int> dir) const;

	bool isMoveAllowedRoad(Scenery &scenery, int dir) const;
	bool isMoveAllowedATV(Scenery &scenery, int dir) const;
};

class VehicleTargetHelper
{
  public:
	// Is a given tile reachable by a given vehicle type?
	enum class Reachability
	{
		Reachable,
		BlockedByVehicle, // We only check for this if VehicleAvoidance is not Ignore.
		BlockedByScenery,
	};

	// Desired target adjustment behavior when the given target tile is BlockedByVehicle.
	enum class VehicleAvoidance
	{
		Ignore,          // Use given target anyway.
		PickNearbyPoint, // Go to a nearby point instead and allow mission to finish.
		Sidestep,        // Go to a nearby point and try to re-route to the original target.
	};

	struct AdjustTargetResult
	{
		// Original target point's reachability.
		Reachability reachability;

		// Whether we were able to find a suitable target point. This is false only in extreme cases
		// where there was no room in the sky, no roads in the city, etc.
		bool foundSuitableTarget;
	};

	// Potentially modifies the given target location such that it is reachable for the given
	// vehicle. Args:
	//   state: Game state.
	//   vehicle: Vehicle, only used for looking up the type.
	//   target: Reference to target location, may be modified by this method.
	//   vehicleAvoidance: Vehicle avoidance strategy. Only relevant for flyers and UFOs. See enum
	//   above. adjustForFlying: If false, don't do any checks or adjustment for flyers and UFOs.
	// Returns: AdjustTargetResult, see above for details.
	static AdjustTargetResult adjustTargetToClosest(GameState &state, Vehicle &v, Vec3<int> &target,
	                                                const VehicleAvoidance vehicleAvoidance,
	                                                bool adjustForFlying);

	// Checks reachability of the target location for the given vehicle.
	// Args:
	//   vehicle: Vehicle, only used for looking up the type.
	//   target: Location to be checked for reachability.
	// Returns: Reachability, see above for details.
	static Reachability isReachableTarget(const Vehicle &v, Vec3<int> target);

  private:
	static AdjustTargetResult adjustTargetToClosestFlying(GameState &state, Vehicle &v,
	                                                      Vec3<int> &target,
	                                                      const VehicleAvoidance vehicleAvoidance);
	static AdjustTargetResult adjustTargetToClosestRoad(Vehicle &v, Vec3<int> &target);
	static AdjustTargetResult adjustTargetToClosestGround(Vehicle &v, Vec3<int> &target);

	static Reachability isReachableTargetFlying(const Vehicle &v, Vec3<int> target);
	static Reachability isReachableTargetRoad(const Vehicle &v, Vec3<int> target);
	static Reachability isReachableTargetGround(const Vehicle &v, Vec3<int> target);
};

class VehicleMission
{
  private:
	// INTERNAL: Not to be used directly (Only works when in building)
	static VehicleMission *takeOff(Vehicle &v);
	// INTERNAL: Not to be used directly (Only works if directly above a pad)
	static VehicleMission *land(Vehicle &v, StateRef<Building> b);
	// INTERNAL: This checks if mission is actually finished. Called by isFinished.
	// If it is finished, update() is called by isFinished so that any remaining work could be done
	bool isFinishedInternal(GameState &state, Vehicle &v);

	bool takeOffCheck(GameState &state, Vehicle &v);
	bool teleportCheck(GameState &state, Vehicle &v);

  public:
	VehicleMission() = default;

	// Methods used in pathfinding etc.
	bool getNextDestination(GameState &state, Vehicle &v, Vec3<float> &destPos, float &destFacing,
	                        int &turboTiles);
	void update(GameState &state, Vehicle &v, unsigned int ticks, bool finished = false);
	bool isFinished(GameState &state, Vehicle &v, bool callUpdateIfFinished = true);
	void start(GameState &state, Vehicle &v);
	void setPathTo(GameState &state, Vehicle &v, Vec3<int> target, int maxIterations,
	               bool checkValidity = true, bool giveUpIfInvalid = false);
	void setFollowPath(GameState &state, Vehicle &v);
	bool advanceAlongPath(GameState &state, Vehicle &v, Vec3<float> &destPos, float &destFacing,
	                      int &turboTiles);
	bool isTakingOff(Vehicle &v);
	int getDefaultIterationCount(Vehicle &v);
	static Vec3<float> getRandomMapEdgeCoordinates(GameState &state, StateRef<City> city);
	bool acquireTargetBuilding(GameState &state, Vehicle &v);
	void updateTimer(unsigned ticks);
	void takePositionNearPortal(GameState &state, Vehicle &v);
	static bool canRecoverVehicle(const GameState &state, const Vehicle &v, const Vehicle &target);

	// Methods to create new missions

	static VehicleMission *gotoLocation(GameState &state, Vehicle &v, Vec3<int> target,
	                                    bool allowTeleporter = false, bool pickNearest = false,
	                                    int attemptsToGiveUpAfter = 20);
	static VehicleMission *gotoPortal(GameState &state, Vehicle &v);
	static VehicleMission *gotoPortal(GameState &state, Vehicle &v, Vec3<int> target);
	static VehicleMission *departToSpace(GameState &state, Vehicle &v);
	// With now building goes home
	static VehicleMission *gotoBuilding(GameState &state, Vehicle &v,
	                                    StateRef<Building> target = nullptr,
	                                    bool allowTeleporter = false);
	static VehicleMission *infiltrateOrSubvertBuilding(GameState &state, Vehicle &v,
	                                                   bool subvert = false,
	                                                   StateRef<Building> target = nullptr);
	static VehicleMission *attackVehicle(GameState &state, Vehicle &v, StateRef<Vehicle> target);
	static VehicleMission *attackBuilding(GameState &state, Vehicle &v,
	                                      StateRef<Building> target = nullptr);
	static VehicleMission *followVehicle(GameState &state, Vehicle &v, StateRef<Vehicle> target);
	static VehicleMission *followVehicle(GameState &state, Vehicle &v,
	                                     std::list<StateRef<Vehicle>> &targets);
	static VehicleMission *recoverVehicle(GameState &state, Vehicle &v, StateRef<Vehicle> target);
	static VehicleMission *offerService(GameState &state, Vehicle &v,
	                                    StateRef<Building> target = nullptr);
	static VehicleMission *snooze(GameState &state, Vehicle &v, unsigned int ticks);
	static VehicleMission *selfDestruct(GameState &state, Vehicle &v);
	static VehicleMission *arriveFromDimensionGate(GameState &state, Vehicle &v, int ticks = 0);
	static VehicleMission *restartNextMission(GameState &state, Vehicle &v);
	static VehicleMission *crashLand(GameState &state, Vehicle &v);
	static VehicleMission *patrol(GameState &state, Vehicle &v, bool home = false,
	                              unsigned int counter = 10);
	static VehicleMission *teleport(GameState &state, Vehicle &v, Vec3<int> target = {-1, -1, -1});
	static VehicleMission *investigateBuilding(GameState &state, Vehicle &v,
	                                           StateRef<Building> target,
	                                           bool allowTeleporter = false);
	UString getName();

	enum class MissionType
	{
		GotoLocation,
		GotoBuilding,
		FollowVehicle,
		RecoverVehicle,
		AttackVehicle,
		AttackBuilding,
		RestartNextMission,
		Snooze,
		TakeOff,
		Land,
		Crash,
		Patrol,
		GotoPortal,
		InfiltrateSubvert,
		OfferService,
		Teleport,
		SelfDestruct,
		DepartToSpace,
		ArriveFromDimensionGate,
		InvestigateBuilding,
	};

	MissionType type = MissionType::GotoLocation;

	// GotoLocation InfiltrateSubvert TakeOff GotoPortal Patrol
	Vec3<int> targetLocation = {0, 0, 0};
	// GotoLocation GotoBuilding
	bool allowTeleporter = false;
	// How many times will vehicle try to re-route until it gives up
	int reRouteAttempts = 0;
	// GotoLocation - should it pick nearest point or random point if destination unreachable
	bool pickNearest = false;
	// Patrol - should patrol around home building only
	bool patrolHome = false;
	// GotoLocation - picked nearest (allows finishing mission without reaching destination)
	bool pickedNearest = false;
	// GotoBuilding AttackBuilding Land Infiltrate
	StateRef<Building> targetBuilding;
	// FollowVehicle AttackVehicle
	StateRef<Vehicle> targetVehicle;
	// FollowVehicle
	std::list<StateRef<Vehicle>> targets;
	// Snooze, SelfDestruct
	unsigned int timeToSnooze = 0;
	// RecoverVehicle, InfiltrateSubvert, Patrol: waypoints
	unsigned int missionCounter = 0;
	// InfiltrateSubvert: mode
	bool subvert = false;
	// AttackVehicle
	bool attackCrashed = false;

	bool cancelled = false;

	std::list<Vec3<int>> currentPlannedPath;
};
} // namespace OpenApoc
