#include "game/city/city.h"
#include "game/city/building.h"
#include "game/city/organisation.h"
#include "game/city/buildingtile.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include <random>

namespace OpenApoc {

City::City(Framework &fw, UString mapName)
	: TileMap(fw, Vec3<int>{100, 100, 10}), organisations(Organisation::defaultOrganisations)
{
	auto file = fw.data->load_file("xcom3/ufodata/" + mapName);
	if (!file)
	{
		LogError("Failed to open city map \"%s\"", mapName.str().c_str());
		return;
	}

	this->buildings = loadBuildingsFromBld(fw, mapName + ".bld", this->organisations, Building::defaultNames);
	this->cityTiles = CityTile::loadTilesFromFile(fw);

	for (int z = 0; z < this->size.z; z++)
	{
		for (int y = 0; y < this->size.y; y++)
		{
			for (int x = 0; x < this->size.x; x++)
			{
				uint16_t tileID;
				Tile &tile = this->getTile(x,y,z);
				if (!file.readule16(tileID))
				{
					LogError("Unexpected EOF reading citymap at %d,%d,%d",x,y,z);
					tileID = 0;
				}
				if (tileID)
				{
					Building *bld = nullptr;
					for (auto &b : this->buildings)
					{
						if (b.bounds.within(Vec2<int>{x,y}))
						{
							if (bld)
							{
								LogError("Multiple buildings on tile at %d,%d,%d", x, y, z);
							}
							bld = &b;
						}
					}
					if (tileID >= this->cityTiles.size())
					{
						LogError("Invalid tile IDX %u at %d,%d,%d", tileID, x, y, z);
					}
					else
					{
						tile.objects.push_back(std::make_shared<BuildingSection>(&tile, this->cityTiles[tileID], Vec3<int>{x,y,z}, bld));
					}
				}
			}
		}
	}

	std::default_random_engine generator;
	std::uniform_int_distribution<int> xydistribution(0,99);
	std::uniform_int_distribution<int> zdistribution(0,9);
	//Place 1000 random cars
	LogInfo("Starting placing cars");
	for (int i = 0; i < 100; i++)
	{
		int x = 0;
		int y = 0;
		int z = 0;
		
		do {
			x = xydistribution(generator);
			y = xydistribution(generator);
			z = zdistribution(generator);
		} while (!this->getTile(x,y,z).objects.empty());

		Tile &tile = this->getTile(x,y,z);

		std::shared_ptr<Vehicle> testVehicle(fw.gamecore->vehicleFactory.create("POLICE_HOVERCAR"));
		this->vehicles.push_back(testVehicle);
		std::shared_ptr<FlyingVehicle> testVehicleObject(new FlyingVehicle(*testVehicle, &tile));
		testVehicle->tileObject = testVehicleObject;
		tile.objects.push_back(testVehicleObject);
		//Vehicles are active
		this->activeObjects.push_back(testVehicleObject);
	}
	LogInfo("Finished placing cars");
}

City::~City()
{

}

}; //namespace OpenApoc
