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
class GameState;
class Building;
class Projectile;
class Scenery;
class Doodad;
class DoodadType;
class SceneryTileType;
class BaseLayout;
class TileMap;

class City : public StateObject
{
	STATE_OBJECT(City)
  public:
	City() = default;
	~City() override;

	void initMap();

	Vec3<int> size;

	StateRefMap<SceneryTileType> tile_types;
	std::map<Vec3<int>, StateRef<SceneryTileType>> initial_tiles;
	StateRefMap<Building> buildings;
	std::vector<sp<Scenery>> scenery;
	std::list<sp<Doodad>> doodads;
	std::vector<sp<Doodad>> portals;

	std::set<sp<Projectile>> projectiles;

	up<TileMap> map;

	void handleProjectileHit(GameState &state, sp<Projectile> projectile, bool displayDoodad,
	                         bool playSound);

	void update(GameState &state, unsigned int ticks);
	void hourlyLoop(GameState &state);
	void dailyLoop(GameState &state);

	void generatePortals(GameState &state);
	void updateInfiltration(GameState &state);

	sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);

	static void accuracyAlgorithmCity(GameState &state, Vec3<float> firePosition,
	                                  Vec3<float> &target, int accuracy, bool cloaked);
};

}; // namespace OpenApoc
