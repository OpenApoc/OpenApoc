#pragma once
#include "framework/includes.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/stateobject.h"
#include "game/state/tileview/tile.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

#define CITY_TILE_X (64)
#define CITY_TILE_Y (32)
#define CITY_TILE_Z (16)

#define CITY_STRAT_TILE_X 8
#define CITY_STRAT_TILE_Y 8

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
};

}; // namespace OpenApoc
