#pragma once

#include "building.h"
#include "vehicle.h"
#include "organisation.h"

#include "framework/includes.h"

namespace OpenApoc {

class CityTile
{
	private:
	public:
		CityTile (int id = 0, Building *building = NULL);

		int id;
		Building *building;
		std::list<std::shared_ptr<Vehicle> > vehiclesOnTile;
};

class City
{
	private:
	public:
	City (std::string mapName);
	~City();

		int sizeX;
		int sizeY;
		int sizeZ;
		//tiles in [z][y][x] order
		std::vector < std::vector < std::vector < CityTile > > > tiles;
		std::list<Building> buildings;
		std::vector<Organisation> organisations;
};

}; //namespace OpenApoc
