#include "game/city/city.h"
#include "game/city/building.h"
#include "game/organisation.h"
#include "game/city/buildingtile.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include <random>

namespace OpenApoc {

City::City(Framework &fw)
	: TileMap(fw, fw.rules->getCitySize())
{
	for (auto &def : fw.rules->getBuildingDefs())
	{
		auto &owner = fw.state->organisations[def.getOwnerIdx()];
		this->buildings.emplace_back(def, owner);
	}

	for (int z = 0; z < this->size.z; z++)
	{
		for (int y = 0; y < this->size.y; y++)
		{
			for (int x = 0; x < this->size.x; x++)
			{
				auto tileID = fw.rules->getBuildingTileAt(Vec3<int>{x,y,z});
				if (tileID == "")
					continue;
				Building *bld = nullptr;

				for (auto &b: this->buildings)
				{
					if (b.def.getBounds().within(Vec2<int>{x,y}))
					{
						if (bld)
						{
							LogError("Multiple buildings on tile at %d,%d,%d", x, y, z);
						}
						bld = &b;
					}
				}


				auto &cityTileDef = fw.rules->getBuildingTileDef(tileID);
				auto tile = std::make_shared<BuildingTile>(*this, cityTileDef, Vec3<int>{x,y,z}, bld);
				this->addObject(std::dynamic_pointer_cast<TileObject>(tile));
			}
		}
	}
}

City::~City()
{

}

}; //namespace OpenApoc
