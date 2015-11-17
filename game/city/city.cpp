#include "game/city/city.h"
#include "game/city/building.h"
#include "game/organisation.h"
#include "game/city/scenery.h"
#include "game/city/doodad.h"
#include "game/city/vehicle.h"
#include "game/city/vehiclemission.h"
#include "game/city/weapon.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/resources/gamecore.h"
#include "game/city/projectile.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/tileview/tileobject_projectile.h"
#include "game/tileview/voxel.h"

#include <limits>
#include <functional>
#include <future>

namespace OpenApoc
{

// An ordered list of the types drawn in each layer
// Within the same layer these are ordered by a calculated z based on the 'center' position
static std::vector<std::set<TileObject::Type>> layerMap = {
    // Draw all scenery first, then put stuff on top of that
    {TileObject::Type::Scenery, TileObject::Type::Doodad},
    {TileObject::Type::Projectile, TileObject::Type::Vehicle, TileObject::Type::Shadow},
};

City::City(Framework &fw, GameState &state) : fw(fw), map(fw, fw.rules->getCitySize(), layerMap)
{
	Trace::start("City::buildings");
	for (auto &def : fw.rules->getBuildingDefs())
	{
		this->buildings.emplace_back(
		    std::make_shared<Building>(def, state.getOrganisation(def.getOwnerName())));
	}
	Trace::end("City::buildings");

	Trace::start("City::scenery");
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
				map.addObjectToMap(scenery);
				if (cityTileDef.getOverlaySprite())
				{
					// FIXME: Bit of a hack to make the overlay always be at the 'top' of the tile -
					// as getPosition() returns the /center/ add half a tile
					scenery->overlayDoodad = std::make_shared<StaticDoodad>(
					    cityTileDef.getOverlaySprite(), scenery->getPosition(),
					    cityTileDef.getImageOffset());
					map.addObjectToMap(scenery->overlayDoodad);
				}
				this->scenery.insert(scenery);
			}
		}
	}
	Trace::end("City::scenery");

	Trace::start("City::scenery::support");
	for (auto &s : this->scenery)
	{
		if (s->pos.z == 0)
		{
			continue;
		}
		auto pos = s->pos;
		pos.z -= 1;

		bool supported = false;

		auto *t = map.getTile(pos);
		for (auto &obj : t->ownedObjects)
		{
			switch (obj->getType())
			{
				case TileObject::Type::Scenery:
				{
					auto supportingSceneryTile = std::static_pointer_cast<TileObjectScenery>(obj);
					auto supportingSceneryObject = supportingSceneryTile->getOwner();
					supportingSceneryObject->supports.insert(s);
					s->supportedBy.insert(supportingSceneryObject);
					supported = true;
				}
				default:
					break;
			}
		}

		if (!supported)
		{

			std::vector<Vec3<int>> dirs = {
			    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0},
			};
			for (auto &d : dirs)
			{
				pos = s->pos;
				pos += d;

				if (pos.x < 0 || pos.x >= map.size.x || pos.y < 0 || pos.y >= map.size.y)
				{
					continue;
				}

				auto *t = map.getTile(pos);
				for (auto &obj : t->ownedObjects)
				{
					switch (obj->getType())
					{
						case TileObject::Type::Scenery:
						{
							auto supportingSceneryTile =
							    std::static_pointer_cast<TileObjectScenery>(obj);
							auto supportingSceneryObject = supportingSceneryTile->getOwner();
							supportingSceneryObject->supports.insert(s);
							s->supportedBy.insert(supportingSceneryObject);
							supported = true;
						}
						default:
							break;
					}
				}
			}
		}
		if (!supported)
		{
			LogWarning("Scenery tile at {%d,%d,%d} has no support", s->pos.x, s->pos.y, s->pos.z);
		}
	}
	Trace::end("City::scenery::support");

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
	Trace::start("City::baseBuildings");
	for (auto b : this->buildings)
	{
		if (!b->def.getBaseCorridors().empty())
		{
			b->base = std::make_shared<Base>(b, fw);
			this->baseBuildings.emplace_back(b);
		}
	}
	Trace::end("City::baseBuildings");
}

City::~City()
{
	TRACE_FN;
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
	// FIXME: Due to tiles possibly being cross-supported we need to clear that sp<> to avoid leaks
	// Should this be pushed into a weak_ptr<> or some other ref?
	for (auto s : this->scenery)
	{
		s->supports.clear();
		s->supportedBy.clear();
	}
	this->scenery.clear();
	this->buildings.clear();
	this->baseBuildings.clear();
}

void City::update(GameState &state, unsigned int ticks)
{
	TRACE_FN_ARGS1("ticks", Strings::FromInteger((int)ticks));
	/* FIXME: Temporary 'get something working' HACK
	 * Every now and then give a landed vehicle a new 'goto random building' mission, so there's
	 * some activity in the city*/
	std::uniform_int_distribution<int> bld_distribution(0, this->buildings.size() - 1);

	// Need to use a 'safe' iterator method (IE keep the next it before calling ->update)
	// as update() calls can erase it's object from the lists

	Trace::start("City::update::buildings->landed_vehicles");
	for (auto it = this->buildings.begin(); it != this->buildings.end();)
	{
		auto b = *it++;
		for (auto &v : b->landed_vehicles)
		{
			for (auto &w : v->weapons)
			{
				w->reload(std::numeric_limits<int>::max());
			}
			if (v->missions.empty())
			{
				auto &dest = this->buildings[bld_distribution(state.rng)];
				v->missions.emplace_back(VehicleMission::gotoBuilding(*v, this->map, dest));
				v->missions.front()->start();
			}
		}
	}
	Trace::end("City::update::buildings->landed_vehicles");
	Trace::start("City::update::vehices->update");
	for (auto it = this->vehicles.begin(); it != this->vehicles.end();)
	{
		auto v = *it++;
		v->update(state, ticks);
	}
	Trace::end("City::update::vehices->update");
	Trace::start("City::update::projectiles->update");
	std::list<std::future<Collision>> collisions;
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	for (auto &p : this->projectiles)
	{
		auto func = std::bind(&Projectile::checkProjectileCollision, p, std::placeholders::_1);
		collisions.emplace_back(fw.threadPool->enqueue(func, std::ref(map)));
	}
	for (auto &future : collisions)
	{
		// Make sure every user of the TileMap is finished before processing (as the tileobject/map
		// lists are not locked, so probably OK for read-only...)
		future.wait();
	}
	for (auto &future : collisions)
	{

		auto c = future.get();
		if (c)
		{
			// FIXME: Handle collision
			this->projectiles.erase(c.projectile);
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
					sceneryTile->getOwner()->handleCollision(state, c);
					break;
				}
				default:
					LogError("Collision with non-collidable object");
			}
		}
	}
	Trace::end("City::update::projectiles->update");
	Trace::start("City::update::fallingScenery->update");
	for (auto it = this->fallingScenery.begin(); it != this->fallingScenery.end();)
	{
		auto s = *it++;
		s->update(fw, state, ticks);
	}
	Trace::end("City::update::fallingScenery->update");
	Trace::start("City::update::doodads->update");
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}
	Trace::end("City::update::doodads->update");
}

sp<Doodad> City::placeDoodad(DoodadDef &def, Vec3<float> position)
{
	auto doodad = std::make_shared<AnimatedDoodad>(def, position);
	map.addObjectToMap(doodad);
	this->doodads.insert(doodad);
	return doodad;
}

} // namespace OpenApoc
