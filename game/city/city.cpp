#include "city.h"
#include "../../framework/data.h"

#include <iostream>

std::unique_ptr<City> City::city;

City::City(std::string mapName)
	: sizeX(100), sizeY(100), sizeZ(100)
{
	auto file = DATA->load_file("ufodata/" + mapName, "r");
	if (!file)
	{
		std::cerr << "Failed to open city map:" << mapName << "\n";
		return;
	}

	tiles.resize(sizeZ);

	for (int z = 0; z < sizeZ; z++)
	{
		tiles[z].resize(sizeY);
		for (int y = 0; y < sizeY; y++)
		{
			tiles[z][y].resize(sizeX);
			for (int x = 0; x < sizeX; x++)
			{
				int16_t tileID = al_fread16le(file);
				tiles[z][y][x] = CityTile(tileID);
			}
		}
	}

	al_fclose(file);
}

City::~City()
{

}

CityTile::CityTile(int id)
	: id(id)
{
}
