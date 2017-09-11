#include "game/state/city/city.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "game/state/city/building.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/rules/doodad_type.h"
#include "game/state/rules/scenery_tile_type.h"
#include "game/state/rules/vequipment_type.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include <functional>
#include <future>
#include <limits>

namespace OpenApoc
{

// An ordered list of the types drawn in each layer
// Within the same layer these are ordered by a calculated z based on the 'center' position
static std::vector<std::set<TileObject::Type>> layerMap = {
    // Draw all scenery first, then put stuff on top of that
    {TileObject::Type::Scenery, TileObject::Type::Doodad, TileObject::Type::Vehicle},
    {TileObject::Type::Projectile, TileObject::Type::Shadow},
};

City::~City()
{
	TRACE_FN;
	// Note due to backrefs to Tile*s etc. we need to destroy all tile objects
	// before the TileMap
	for (auto &p : this->projectiles)
	{
		if (p->tileObject)
			p->tileObject->removeFromMap();
		p->tileObject = nullptr;
	}
	this->projectiles.clear();
	for (auto &s : this->scenery)
	{
		if (s->tileObject)
			s->tileObject->removeFromMap();
		s->tileObject = nullptr;
	}
	// FIXME: Due to tiles possibly being cross-supported we need to clear that sp<> to avoid leaks
	// Should this be pushed into a weak_ptr<> or some other ref?
	for (auto &s : this->scenery)
	{
		s->supports.clear();
		s->supportedBy.clear();
	}

	for (auto &b : this->buildings)
	{
		b.second->landed_vehicles.clear();
	}
}

void City::initMap()
{
	if (this->map)
	{
		LogError("Called on city with existing map");
		return;
	}
	this->map.reset(new TileMap(this->size, VELOCITY_SCALE_CITY,
	                            {VOXEL_X_CITY, VOXEL_Y_CITY, VOXEL_Z_CITY}, layerMap));
	for (auto &s : this->scenery)
	{
		// FIXME: Should we really add all scenery to the map? What if it's destroyed?
		this->map->addObjectToMap(s);
		if (s->type->isLandingPad)
		{
			Vec2<int> pos = {s->initialPosition.x, s->initialPosition.y};

			for (auto &b : this->buildings)
			{
				if (b.second->bounds.within(pos))
				{
					b.second->landingPadLocations.push_back(s->initialPosition);
					LogInfo("Pad %s is within building %s bounds %s", pos, b.first,
					        b.second->bounds);
				}
			}
		}
	}
	for (auto &b : this->buildings)
	{
		if (b.second->landingPadLocations.empty())
		{
			LogError("Building %s has no landing pads", b.first);
		}
		LogInfo("Building %s has %u landing pads:", b.first,
		        (unsigned)b.second->landingPadLocations.size());

		for (auto &loc : b.second->landingPadLocations)
		{
			LogInfo("Pad: %s", loc);
		}
	}
	for (auto &p : this->projectiles)
	{
		this->map->addObjectToMap(p);
	}
	for (auto &d : this->doodads)
	{
		this->map->addObjectToMap(d);
	}
	for (auto &p : this->portals)
	{
		this->map->addObjectToMap(p);
	}
}

void City::update(GameState &state, unsigned int ticks)
{
	TRACE_FN_ARGS1("ticks", Strings::fromInteger(static_cast<int>(ticks)));
	/* FIXME: Temporary 'get something working' HACK
	 * Every now and then give a landed vehicle a new 'goto random building' mission, so there's
	 * some activity in the city*/
	std::uniform_int_distribution<int> bld_distribution(0, (int)this->buildings.size() - 1);

	// Need to use a 'safe' iterator method (IE keep the next it before calling ->update)
	// as update() calls can erase it's object from the lists

	Trace::start("City::update::buildings->landed_vehicles");
	for (auto it = this->buildings.begin(); it != this->buildings.end();)
	{
		auto b = it->second;
		it++;
		for (auto v : b->landed_vehicles)
		{
			for (auto &e : v->equipment)
			{
				if (e->type->type != EquipmentSlotType::VehicleWeapon)
					continue;
				e->reload(std::numeric_limits<int>::max());
			}
			if (v->owner == state.getPlayer())
				continue;

			if (v->missions.empty())
			{
				auto bldIt = this->buildings.begin();
				auto count = bld_distribution(state.rng);
				while (count--)
					bldIt++;
				StateRef<Building> dest = {&state, bldIt->first};
				v->missions.emplace_back(VehicleMission::gotoBuilding(state, *v, dest));
				v->missions.front()->start(state, *v);

				// FIXME: Make snoozetime bounds/distribution readable from serialized GameState
				std::uniform_int_distribution<unsigned int> snoozeTimeDist(10, 10000);
				v->missions.emplace_back(
				    VehicleMission::snooze(state, *v, snoozeTimeDist(state.rng)));
			}
		}
	}
	Trace::end("City::update::buildings->landed_vehicles");
	Trace::start("City::update::projectiles->update");
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	for (auto it = this->projectiles.begin(); it != this->projectiles.end();)
	{
		auto &p = *it++;
		auto c = p->checkProjectileCollision(*map);
		if (c)
		{
			// FIXME: Handle collision
			this->projectiles.erase(c.projectile);
			if (c.projectile->impactSfx)
			{
				fw().soundBackend->playSample(c.projectile->impactSfx, c.position);
			}

			auto doodadType = c.projectile->doodadType;
			if (doodadType)
			{
				auto doodad = this->placeDoodad(doodadType, c.position);
			}

			switch (c.obj->getType())
			{
				case TileObject::Type::Vehicle:
				{
					auto vehicle = std::static_pointer_cast<TileObjectVehicle>(c.obj)->getVehicle();
					vehicle->handleCollision(state, c);
					LogWarning("Vehicle collision");
					break;
				}
				case TileObject::Type::Scenery:
				{
					auto sceneryTile = std::static_pointer_cast<TileObjectScenery>(c.obj);
					// FIXME: Don't just explode scenery, but damaged tiles/falling stuff? Different
					// explosion doodads? Not all weapons instantly destory buildings too

					auto doodad =
					    this->placeDoodad({&state, "DOODAD_3_EXPLOSION"}, sceneryTile->getCenter());
					sceneryTile->getOwner()->handleCollision(state, c);
					break;
				}
				default:
					LogError("Collision with non-collidable object");
			}
		}
	}
	Trace::end("City::update::projectiles->update");
	Trace::start("City::update::scenery->update");
	for (auto &s : this->scenery)
	{
		s->update(state, ticks);
	}
	Trace::end("City::update::scenery->update");
	Trace::start("City::update::doodads->update");
	for (auto it = this->doodads.begin(); it != this->doodads.end();)
	{
		auto d = *it++;
		d->update(state, ticks);
	}

	for (auto it = this->portals.begin(); it != this->portals.end();)
	{
		auto p = *it++;
		p->update(state, ticks);
	}
	Trace::end("City::update::doodads->update");
}

void City::hourlyLoop(GameState &state) { updateInfiltration(state); }

void City::dailyLoop(GameState &state)
{
	// FIXME: Repair buildings, update stocks

	generatePortals(state);
}

void City::generatePortals(GameState &state)
{
	static const int iterLimit = 1000;
	for (auto &p : portals)
	{
		p->remove(state);
	}
	this->portals.clear();

	std::uniform_int_distribution<int> xyPos(10, 130);
	std::uniform_int_distribution<int> zPos(2, 8);
	for (int p = 0; p < 3; p++)
	{
		for (int i = 0; i < iterLimit; i++)
		{
			Vec3<float> pos(xyPos(state.rng), xyPos(state.rng), zPos(state.rng));

			if (map->tileIsValid(pos) && map->getTile(pos)->ownedObjects.empty())
			{
				auto doodad =
				    mksp<Doodad>(pos, StateRef<DoodadType>{&state, "DOODAD_6_DIMENSION_GATE"});
				map->addObjectToMap(doodad);
				this->portals.push_back(doodad);
				break;
			}
		}
	}
}

void City::updateInfiltration(GameState &state)
{
	for (auto &b : buildings)
	{
		b.second->alienGrowth(state);
	}
}

sp<Doodad> City::placeDoodad(StateRef<DoodadType> type, Vec3<float> position)
{
	auto doodad = mksp<Doodad>(position, type);
	map->addObjectToMap(doodad);
	this->doodads.push_back(doodad);
	return doodad;
}

sp<City> City::get(const GameState &state, const UString &id)
{
	auto it = state.cities.find(id);
	if (it == state.cities.end())
	{
		LogError("No citymap matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

const UString &City::getPrefix()
{
	static UString prefix = "CITYMAP_";
	return prefix;
}
const UString &City::getTypeName()
{
	static UString name = "City";
	return name;
}

const UString &City::getId(const GameState &state, const sp<City> ptr)
{
	static const UString emptyString = "";
	for (auto &c : state.cities)
	{
		if (c.second == ptr)
		{
			return c.first;
		}
	}
	LogError("No city matching pointer %p", ptr.get());
	return emptyString;
}

void City::accuracyAlgorithmCity(GameState &state, Vec3<float> firePosition, Vec3<float> &target,
                                 int accuracy, bool cloaked)
{
	// FIXME: Prettify this code
	int projx = firePosition.x;
	int projy = firePosition.y;
	int projz = firePosition.z;
	int vehx = target.x;
	int vehy = target.y;
	int vehz = target.z;
	int inverseAccuracy = 100 - accuracy;
	// Introduce minimal dispersion?
	inverseAccuracy = std::max(0, inverseAccuracy);

	float delta_x = (float)(vehx - projx) * inverseAccuracy / 1000.0f;
	float delta_y = (float)(vehy - projy) * inverseAccuracy / 1000.0f;
	float delta_z = (float)(vehz - projz) * inverseAccuracy / 1000.0f;
	if (delta_x == 0.0f && delta_y == 0.0f && delta_z == 0.0f)
	{
		return;
	}

	float length_vector =
	    1.0f / std::sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

	std::vector<float> rnd(3);
	while (true)
	{
		rnd[1] = (float)randBoundsExclusive(state.rng, 0, 100000) / 100000.0f;
		rnd[2] = (float)randBoundsExclusive(state.rng, 0, 100000) / 100000.0f;
		rnd[0] = rnd[1] * rnd[1] + rnd[2] * rnd[2];
		if (rnd[0] > 0.0f && rnd[0] < 1.0f)
		{
			break;
		}
	}

	// Misses can go both ways on the axis
	float k1 = (2 * randBoundsInclusive(state.rng, 0, 1) - 1) * rnd[1] *
	           std::sqrt(-2 * std::log(rnd[0]) / rnd[0]);
	float k2 = (2 * randBoundsInclusive(state.rng, 0, 1) - 1) * rnd[2] *
	           std::sqrt(-2 * std::log(rnd[0]) / rnd[0]);

	float x1 = length_vector * delta_x * delta_z * k1;
	float y1 = length_vector * delta_y * delta_z * k1;
	float z1 = -length_vector * (delta_x * delta_x + delta_y * delta_y) * k1;

	float x2 = -delta_y * k2;
	float y2 = delta_x * k2;
	float z2 = 0;

	float x3 = (x1 + x2);
	float y3 = (y1 + y2);
	float z3 = ((z1 + z2) / 3.0f);
	x3 = x3 < 0 ? ceilf(x3) : floorf(x3);
	y3 = y3 < 0 ? ceilf(y3) : floorf(y3);
	z3 = z3 < 0 ? ceilf(z3) : floorf(z3);

	target.x += x3;
	target.y += y3;
	target.z += z3;
}

} // namespace OpenApoc
