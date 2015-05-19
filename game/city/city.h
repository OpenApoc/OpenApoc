#pragma once

#include "framework/includes.h"

#include "game/tileview/tile.h"
#include "buildingtile.h"
#include "game/city/vehicle.h"

namespace OpenApoc {

#define CITY_TILE_X (64)
#define CITY_TILE_Y (32)
#define CITY_TILE_Z (16)

class Building;
class Organisation;
class Tile;
class BuildingTile;
class Vehicle;

class City : public TileMap
{
	private:
		std::vector<Building> buildings;
		std::vector<Organisation> organisations;
		std::vector<CityTile> cityTiles;
		std::vector<std::shared_ptr<Vehicle>> vehicles;
	public:
		City(Framework &fw, UString mapName);
		~City();

};

}; //namespace OpenApoc
