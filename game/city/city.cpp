#include "game/city/city.h"
#include "game/city/building.h"
#include "game/organisation.h"
#include "game/city/buildingtile.h"
#include "game/city/vehicle.h"
#include "game/city/vehiclemission.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"

namespace OpenApoc
{

City::City(Framework &fw, GameState &state) : TileMap(fw, fw.rules->getCitySize())
{
	for (auto &def : fw.rules->getBuildingDefs())
	{
		this->buildings.emplace_back(def, state.getOrganisation(def.getOwnerName()));
	}

	for (int z = 0; z < this->size.z; z++)
	{
		for (int y = 0; y < this->size.y; y++)
		{
			for (int x = 0; x < this->size.x; x++)
			{
				auto tileID = fw.rules->getBuildingTileAt(Vec3<int>{x, y, z});
				if (tileID == "")
					continue;
				Building *bld = nullptr;

				for (auto &b : this->buildings)
				{
					if (b.def.getBounds().withinInclusive(Vec2<int>{x, y}))
					{
						if (bld)
						{
							LogError("Multiple buildings on tile at %d,%d,%d", x, y, z);
						}
						bld = &b;
						for (auto &padID : fw.rules->getLandingPadTiles())
						{
							if (padID == tileID)
							{
								LogInfo("Building %s has landing pad at {%d,%d,%d}",
								        b.def.getName().c_str(), x, y, z);
								b.landingPadLocations.emplace_back(x, y, z);
								break;
							}
						}
					}
				}

				auto &cityTileDef = fw.rules->getBuildingTileDef(tileID);
				auto tile =
				    std::make_shared<BuildingTile>(*this, cityTileDef, Vec3<int>{x, y, z}, bld);
				this->addObject(std::dynamic_pointer_cast<TileObject>(tile));
				tile->setPosition(Vec3<int>{x, y, z});
			}
		}
	}
	/* Sanity check - all buildings should at have least one landing pad */
	for (auto &b : this->buildings)
	{
		if (b.landingPadLocations.empty())
		{
			LogError("Building \"%s\" has no landing pads", b.def.getName().c_str());
		}
	}
	/* Keep a cache of base locations (using pointers since you can't have a vector of references)
	 */
	for (auto &b : this->buildings)
	{
		if (!b.def.getBaseCorridors().empty())
		{
			b.base = std::make_shared<Base>(b, fw);
			this->baseBuildings.emplace_back(&b);
		}
	}
}

City::~City() {}

void City::update(unsigned int ticks)
{
	/* FIXME: Temporary 'get something working' HACK
	 * Every now and then give a landed vehicle a new 'goto random building' mission, so there's
	 * some activity in the city*/
	std::uniform_int_distribution<int> bld_distribution(0, this->buildings.size() - 1);
	for (auto &b : this->buildings)
	{
		for (auto &v : b.landed_vehicles)
		{
			if (v->missions.empty())
			{
				auto &b = this->buildings[bld_distribution(fw.state->rng)];
				v->missions.emplace_back(VehicleMission::gotoBuilding(*v, *this, b));
				v->missions.front()->start();
			}
		}
	}
	TileMap::update(ticks);
}

}; // namespace OpenApoc
