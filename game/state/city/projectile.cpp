#include "game/state/city/projectile.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "game/state/tileview/tileobject_battleunit.h"

namespace OpenApoc
{

const std::map<Projectile::Type, UString> Projectile::TypeMap = {
    {Type::Beam, "beam"}, {Type::Missile, "missile"},
};

Projectile::Projectile(StateRef<Vehicle> firer, Vec3<float> position, Vec3<float> velocity,
                       unsigned int lifetime, int damage, unsigned int tail_length,
                       std::list<sp<Image>> projectile_sprites, sp<Sample> impactSfx)
    : type(Type::Beam), position(position), velocity(velocity), age(0), lifetime(lifetime),
      damage(damage), firerVehicle(firer), previousPosition(position), tail_length(tail_length),
      projectile_sprites(projectile_sprites), impactSfx(impactSfx), velocityScale(VELOCITY_SCALE_CITY)
{
	// 36 / (velocity length) = enough ticks to pass 1 whole tile
	ownerInvulnerableTicks = (int)ceilf(36.0f / glm::length(velocity / velocityScale)) + 1;
}
// FIXME: Properly add unit projectiles and shit
Projectile::Projectile(StateRef<BattleUnit> firer, Vec3<float> position, Vec3<float> velocity,
                       unsigned int lifetime, int damage, unsigned int tail_length,
                       std::list<sp<Image>> projectile_sprites, sp<Sample> impactSfx)
    : type(Type::Beam), position(position), velocity(velocity), age(0), lifetime(lifetime),
      damage(damage), firerUnit(firer), previousPosition(position), tail_length(tail_length),
      projectile_sprites(projectile_sprites), impactSfx(impactSfx), velocityScale(VELOCITY_SCALE_BATTLE)
{
	// 36 / (velocity length) = enough ticks to pass 1 whole tile
	ownerInvulnerableTicks = (int)ceilf(36.0f / glm::length(velocity / velocityScale)) + 1;
}

Projectile::Projectile()
    : type(Type::Beam), position(0, 0, 0), velocity(0, 0, 0), age(0), lifetime(0), damage(0),
      previousPosition(0, 0, 0), tail_length(0), velocityScale(1, 1, 1)
{ }

void Projectile::update(GameState &state, unsigned int ticks)
{
	if (ownerInvulnerableTicks > 0)
		ownerInvulnerableTicks -= ticks;
	this->age += ticks;
	this->previousPosition = this->position;
	auto newPosition = this->position +
	                   ((static_cast<float>(ticks) / TICK_SCALE) * this->velocity) / velocityScale;

	auto mapSize = this->tileObject->map.size;

	// Remove projectile if it's ran out of life or fell off the end of the world
	if (newPosition.x < 0 || newPosition.x >= mapSize.x || newPosition.y < 0 ||
	    newPosition.y >= mapSize.y || newPosition.z < 0 || newPosition.z >= mapSize.z ||
	    this->age >= this->lifetime)
	{
		auto this_shared = shared_from_this();
		if (firerVehicle)
		{
			for (auto &city : state.cities)
				city.second->projectiles.erase(std::dynamic_pointer_cast<Projectile>(this_shared));
		}
		else
		{
			state.current_battle->projectiles.erase(std::dynamic_pointer_cast<Projectile>(this_shared));
		}
		this->tileObject->removeFromMap();
		this->tileObject.reset();
	}
	else
	{
		this->position = newPosition;
		this->tileObject->setPosition(newPosition);
	}
}

Collision Projectile::checkProjectileCollision(TileMap &map)
{
	if (!this->tileObject)
	{
		// It's possible the projectile reached the end of it's lifetime this frame
		// so ignore stuff without a tile
		return {};
	}

	Collision c = map.findCollision(this->previousPosition, this->position);
	if (c && ownerInvulnerableTicks > 0 && ((c.obj->getType() == TileObject::Type::Vehicle &&
	    this->firerVehicle == std::static_pointer_cast<TileObjectVehicle>(c.obj)->getVehicle())
		|| (c.obj->getType() == TileObject::Type::Unit &&
			this->firerUnit == std::static_pointer_cast<TileObjectBattleUnit>(c.obj)->getUnit())))
	{
		return {}; 
	}

	c.projectile = shared_from_this();
	return c;
}

Projectile::~Projectile()
{
	if (this->tileObject)
	{
		this->tileObject->removeFromMap();
	}
}

}; // namespace OpenApoc
