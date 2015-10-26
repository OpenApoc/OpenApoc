#include "game/city/city.h"
#include "game/city/building.h"
#include "game/organisation.h"
#include "game/city/scenery.h"
#include "game/city/doodad.h"
#include "game/city/vehicle.h"
#include "game/city/vehiclemission.h"
#include "framework/framework.h"
#include "game/resources/gamecore.h"
#include "game/city/projectile.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/tileview/tileobject_projectile.h"
#include "game/tileview/voxel.h"

namespace OpenApoc
{

City::City(Framework &fw, GameState &state) : fw(fw), map(fw, fw.rules->getCitySize())
{
	for (auto &def : fw.rules->getBuildingDefs())
	{
		this->buildings.emplace_back(
		    std::make_shared<Building>(def, state.getOrganisation(def.getOwnerName())));
	}

	for (int z = 0; z < this->map.size.z; z++)
	{
		for (int y = 0; y < this->map.size.y; y++)
		{
			for (int x = 0; x < this->map.size.x; x++)
			{
				auto tileID = fw.rules->getSceneryTileAt(Vec3<int>{x, y, z});
				if (tileID == "")
					continue;
				sp<Building> bld = nullptr;

				for (auto b : this->buildings)
				{
					if (b->def.getBounds().withinInclusive(Vec2<int>{x, y}))
					{
						if (bld)
						{
							LogError("Multiple buildings on tile at %d,%d,%d", x, y, z);
						}
						bld = b;
						for (auto &padID : fw.rules->getLandingPadTiles())
						{
							if (padID == tileID)
							{
								LogInfo("Building %s has landing pad at {%d,%d,%d}",
								        b->def.getName().c_str(), x, y, z);
								b->landingPadLocations.emplace_back(x, y, z);
								break;
							}
						}
					}
				}

				auto &cityTileDef = fw.rules->getSceneryTileDef(tileID);
				auto scenery = std::make_shared<Scenery>(cityTileDef, Vec3<int>{x, y, z}, bld);
				auto tile = map.addObjectToMap(scenery);
				this->scenery.insert(scenery);
			}
		}
	}
	/* Sanity check - all buildings should at have least one landing pad */
	for (auto b : this->buildings)
	{
		if (b->landingPadLocations.empty())
		{
			LogError("Building \"%s\" has no landing pads", b->def.getName().c_str());
		}
	}
	/* Keep a cache of base locations (using pointers since you can't have a vector of references)
	 */
	for (auto b : this->buildings)
	{
		if (!b->def.getBaseCorridors().empty())
		{
			b->base = std::make_shared<Base>(b, fw);
			this->baseBuildings.emplace_back(b);
		}
	}
}

City::~City()
{
	// Note due to backrefs to Tile*s etc. we need to destroy all tile objects
	// before the TileMap
	for (auto &v : this->vehicles)
	{
		if (v->tileObject)
			v->tileObject->removeFromMap();
	}
	this->vehicles.clear();
	for (auto &p : this->projectiles)
	{
		if (p->tileObject)
			p->tileObject->removeFromMap();
	}
	this->projectiles.clear();
	for (auto &s : this->scenery)
	{
		if (s->tileObject)
			s->tileObject->removeFromMap();
	}
	this->scenery.clear();
	this->buildings.clear();
	this->baseBuildings.clear();
}

void City::update(GameState &state, unsigned int ticks)
{
	/* FIXME: Temporary 'get something working' HACK
	 * Every now and then give a landed vehicle a new 'goto random building' mission, so there's
	 * some activity in the city*/
	std::uniform_int_distribution<int> bld_distribution(0, this->buildings.size() - 1);
	for (auto b : this->buildings)
	{
		for (auto &v : b->landed_vehicles)
		{
			if (v->missions.empty())
			{
				auto &b = this->buildings[bld_distribution(state.rng)];
				v->missions.emplace_back(VehicleMission::gotoBuilding(*v, this->map, b));
				v->missions.front()->start();
			}
		}
	}
	for (auto v : this->vehicles)
	{
		v->update(state, ticks);
	}
	for (auto p : this->projectiles)
	{
		p->update(state, ticks);
		Collision c = p->checkProjectileCollision(map);
		if (c)
		{
			// FIXME: Handle collision
			this->projectiles.erase(p);
			// FIXME: Get doodad from weapon definition?
			auto doodad =
			    this->placeDoodad(fw.rules->getDoodadDef("DOODAD_EXPLOSION_0"), c.position);

			switch (c.obj->getType())
			{
				case TileObject::Type::Vehicle:
				{
					LogWarning("Vehicle collision");
					break;
				}
				case TileObject::Type::Scenery:
				{
					auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(c.obj);
					// FIXME: Don't just explode scenery, but damaged tiles/falling stuff? Different
					// explosion doodads? Not all weapons instantly destory buildings too

					auto doodad = this->placeDoodad(fw.rules->getDoodadDef("DOODAD_EXPLOSION_2"),
					                                sceneryTile->getPosition());
					sceneryTile->getOwner()->handleCollision(c);
					break;
				}
				default:
					LogError("Collision with non-collidable object");
			}
		}
	}
	for (auto d : this->doodads)
	{
		d->update(state, ticks);
	}
}

sp<Doodad> City::placeDoodad(DoodadDef &def, Vec3<float> position)
{
	auto doodad = std::make_shared<Doodad>(def, position);
	auto doodadTileObject = map.addObjectToMap(doodad);
	this->doodads.insert(doodad);
	return doodad;
}

} // namespace OpenApoc
