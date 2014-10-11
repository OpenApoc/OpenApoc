#pragma once

#include "framework/includes.h"

#include "game/tileview/tile.h"
#include "buildingtile.h"

namespace OpenApoc {

#define CITY_TILE_X (64)
#define CITY_TILE_Y (32)
#define CITY_TILE_Z (16)

class Building;
class Organisation;
class Tile;
class BuildingTile;

class City : public TileMap
{
	private:
		std::vector<Building> buildings;
		std::vector<Organisation> organisations;
		std::vector<CityTile> cityTiles;
	public:
		City(Framework &fw, std::string mapName);
		~City();

};

}; //namespace OpenApoc
