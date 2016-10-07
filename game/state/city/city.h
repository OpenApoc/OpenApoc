#pragma once
#include "framework/includes.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/stateobject.h"
#include "game/state/tileview/tile.h"
#include "library/sp.h"
#include "library/vec.h"

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

class City : public StateObject<City>
{
  public:
	City() = default;
	~City() override;

	void initMap();

	Vec3<int> size;

	StateRefMap<SceneryTileType> tile_types;
	std::map<Vec3<int>, StateRef<SceneryTileType>> initial_tiles;
	StateRefMap<Building> buildings;
	std::list<sp<Scenery>> scenery;
	std::list<sp<Doodad>> doodads;
	std::list<sp<Doodad>> portals;

	std::set<sp<Projectile>> projectiles;

	up<TileMap> map;

	void update(GameState &state, unsigned int ticks);
	void dailyLoop(GameState &state);

	void generatePortals(GameState &state);
	sp<Doodad> placeDoodad(StateRef<DoodadType> type, Vec3<float> position);

	static void accuracyAlgorithmCity(GameState &state, Vec3<float> firePosition, Vec3<float> &target, int accuracy);
};

}; // namespace OpenApoc
