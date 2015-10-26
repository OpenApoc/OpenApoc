#pragma once
#include "library/sp.h"

#include "framework/includes.h"

#include "game/tileview/tile.h"

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

class City
{
  private:
  public:
	City(Framework &fw, GameState &state);
	~City();
	std::vector<sp<Vehicle>> vehicles;
	std::set<sp<Projectile>> projectiles;
	std::vector<sp<Building>> buildings;
	std::vector<sp<Building>> baseBuildings;
	std::set<sp<Scenery>> scenery;

	TileMap map;

	void update(GameState &state, unsigned int ticks);
};

}; // namespace OpenApoc
