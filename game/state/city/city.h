#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <set>

namespace OpenApoc
{

#define TILE_X_CITY (64)
#define TILE_Y_CITY (32)
#define TILE_Z_CITY (16)

#define VOXEL_X_CITY (32)
#define VOXEL_Y_CITY (32)
#define VOXEL_Z_CITY (16)

class Vehicle;
class VehicleType;
class GameState;
class Building;
class Projectile;
class Scenery;
class Doodad;
class DoodadType;
class SceneryTileType;
class Organisation;
class BaseLayout;
class Agent;
class ResearchTopic;
class TileMap;
class GroundVehicleTileHelper;

class RoadSegment
{
  public:
	std::vector<int> connections;
	std::vector<Vec3<int>> tilePosition;
	bool intact = false;
	std::vector<bool> tileIntact;
	Vec3<int> middle = {0, 0, 0};
	int length = 0;

	// Methods

	void notifyRoadChange(const Vec3<int> &position, bool newIntact);
	void finalizeStats();
	bool empty() const;

	// Getters

	const Vec3<int> &getFirst() const;
	const Vec3<int> &getLast() const;
	// Get road end, accepts 0 or 1, returns first or last
	const Vec3<int> &getByConnectID(int id) const;
	bool getIntactFirst() const;
	bool getIntactLast() const;
	bool getIntactByConnectID(int id) const;
	bool getIntactByTile(const Vec3<int> &position) const;

	// Constructors

	RoadSegment() = default;
	RoadSegment(Vec3<int> tile);
	RoadSegment(Vec3<int> tile, int connection);

	// Pathfinding
	std::list<Vec3<int>> findPath(Vec3<int> origin, Vec3<int> destination) const;
	std::list<Vec3<int>> findClosestPath(Vec3<int> origin, Vec3<int> destination) const;
	std::list<Vec3<int>> findPathThrough(int id) const;
};

class City : public StateObject<City>, public std::enable_shared_from_this<City>
{
  public:
	City() = default;
	~City() override;

	void initCity(GameState &state);

	UString id;
	Vec3<int> size = {0, 0, 0};

	StateRefMap<SceneryTileType> tile_types;
	std::map<Vec3<int>, StateRef<SceneryTileType>> initial_tiles;
	std::list<Vec3<int>> initial_portals;
	StateRefMap<Building> buildings;
	std::vector<sp<Scenery>> scenery;
	std::list<sp<Doodad>> doodads;
	std::vector<sp<Doodad>> portals;

	std::set<sp<Projectile>> projectiles;

	up<TileMap> map;

	// Economy: default civilian salary that setting their expectations
	int civilianSalary = 0;

	// Unlocks when visiting this
	std::list<StateRef<ResearchTopic>> researchUnlock;

	// Pathfinding

	std::vector<int> tileToRoadSegmentMap;
	std::vector<RoadSegment> roadSegments;
	int getRoadSegmentID(const Vec3<int> &position) const;
	const RoadSegment &getRoadSegment(const Vec3<int> &position) const;
	void notifyRoadChange(const Vec3<int> &position, bool intact);
	void fillRoadSegmentMap(GameState &state);

	// CityView and CityTileView settings, saved here so that we can return to them

	Vec3<float> cityViewScreenCenter = {0.0f, 0.0f, 0.0f};
	int cityViewPageIndex = 0;
	std::list<StateRef<Vehicle>> cityViewSelectedVehicles;
	std::list<StateRef<Agent>> cityViewSelectedAgents;
	StateRef<Organisation> cityViewSelectedOrganisation;
	int cityViewOrgButtonIndex = 0;

	void handleProjectileHit(GameState &state, sp<Projectile> projectile, bool displayDoodad,
	                         bool playSound, bool expired);

	void update(GameState &state, unsigned int ticks);
	void hourlyLoop(GameState &state);
	void dailyLoop(GameState &state);

	void generatePortals(GameState &state);
	void updateInfiltration(GameState &state);
	void repairVehicles(GameState &state);
	void repairScenery(GameState &state);

	void initialSceneryLinkUp();

	sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);
	sp<Vehicle> createVehicle(GameState &state, StateRef<VehicleType> type,
	                          StateRef<Organisation> owner);
	sp<Vehicle> createVehicle(GameState &state, StateRef<VehicleType> type,
	                          StateRef<Organisation> owner, StateRef<Building> building);
	sp<Vehicle> placeVehicle(GameState &state, StateRef<VehicleType> type,
	                         StateRef<Organisation> owner);
	sp<Vehicle> placeVehicle(GameState &state, StateRef<VehicleType> type,
	                         StateRef<Organisation> owner, StateRef<Building> building);
	sp<Vehicle> placeVehicle(GameState &state, StateRef<VehicleType> type,
	                         StateRef<Organisation> owner, Vec3<float> position,
	                         float facing = 0.0f);

	// Pathfinding functions

	// Find shortest path, using road segments as a guide if going far
	std::list<Vec3<int>> findShortestPath(Vec3<int> origin, Vec3<int> destination,
	                                      const GroundVehicleTileHelper &canEnterTile,
	                                      bool approachOnly = false, bool ignoreStaticUnits = false,
	                                      bool ignoreMovingUnits = true,
	                                      bool ignoreAllUnits = false);

	// Move a group of vehicles in formation
	void groupMove(GameState &state, std::list<StateRef<Vehicle>> &selectedVehicles,
	               Vec3<int> targetLocation, bool useTeleporter = false);

	static void accuracyAlgorithmCity(GameState &state, Vec3<float> firePosition,
	                                  Vec3<float> &target, int accuracy, bool cloaked);

	// Following members are not serialized, but rather are set in initCity method
	std::list<StateRef<Building>> spaceports;
	int populationUnemployed = 0;
	int populationWorking = 0;
	int averageWage = 0;
};

}; // namespace OpenApoc
