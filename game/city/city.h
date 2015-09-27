#pragma once

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

class City : public TileMap
{
  private:
  public:
	City(Framework &fw, GameState &state);
	~City();
	std::vector<std::shared_ptr<Vehicle>> vehicles;
	std::vector<Building> buildings;

	void update(unsigned int ticks);
};

}; // namespace OpenApoc
