#pragma once

#include "../../framework/includes.h"
#include <memory>

class CityTile
{
	private:
	public:
	CityTile (int id = 0);

	int id;
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

	std::vector < std::vector < std::vector < CityTile > > > tiles;



	static std::unique_ptr<City> city;
};

#define CITY City::city
