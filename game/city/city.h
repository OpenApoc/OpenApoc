#pragma once

#include "framework/includes.h"

#include "game/tileview/tile.h"
#include "game/city/buildingtile.h"
#include "game/city/vehicle.h"

namespace OpenApoc {

#define CITY_TILE_X (64)
#define CITY_TILE_Y (32)
#define CITY_TILE_Z (16)

class Vehicle;

class City : public TileMap
{
	private:
	public:
		City(Framework &fw);
		~City();
	std::vector<std::shared_ptr<Vehicle>> vehicles;

	std::vector<Building> buildings;

};

}; //namespace OpenApoc
