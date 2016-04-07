#include "game/state/city/projectile.h"
#include "framework/logger.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/voxel.h"
#include "library/sp.h"

namespace OpenApoc
{

const std::map<Projectile::Type, UString> Projectile::TypeMap = {
    {Projectile::Type::Beam, "beam"}, {Projectile::Type::Missile, "missile"},
};

Projectile::Projectile(StateRef<Vehicle> firer, Vec3<float> position, Vec3<float> velocity,
                       unsigned int lifetime, const Colour &colour, float beamLength,
                       float beamWidth)
    : type(Type::Beam), position(position), velocity(velocity), age(0), lifetime(lifetime),
      firer(firer), previousPosition(position), colour(colour), beamLength(beamLength),
      beamWidth(beamWidth)
{
}

Projectile::Projectile()
    : type(Type::Beam), position(0, 0, 0), velocity(0, 0, 0), age(0), lifetime(0),
      previousPosition(0, 0, 0), colour(0, 0, 0), beamLength(0), beamWidth(0)
{
}

void Projectile::update(GameState &state, unsigned int ticks)
{
	this->age += ticks;
	this->previousPosition = this->position;
	auto newPosition = this->position +
	                   ((static_cast<float>(ticks) / TICK_SCALE) * this->velocity) / VELOCITY_SCALE;

	auto mapSize = this->tileObject->map.size;

	// Remove projectile if it's ran out of life or fell off the end of the world
	if (newPosition.x < 0 || newPosition.x >= mapSize.x || newPosition.y < 0 ||
	    newPosition.y >= mapSize.y || newPosition.z < 0 || newPosition.z >= mapSize.z ||
	    this->age >= this->lifetime)
	{
		auto this_shared = shared_from_this();
		for (auto &city : state.cities)
			city.second->projectiles.erase(std::dynamic_pointer_cast<Projectile>(this_shared));
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
