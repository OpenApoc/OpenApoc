#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Building;
class Organisation;
class Framework;
class Vehicle;

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
		City (Framework &fw, std::string mapName);
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
