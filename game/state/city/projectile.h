#pragma once
#include "game/state/city/vehicle.h"
#include "game/state/stateobject.h"
#include "library/colour.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class Weapon;
class GameState;
class TileObjectProjectile;
class TileMap;
class Collision;

class Projectile : public std::enable_shared_from_this<Projectile>
{
  public:
	enum class Type
	{
		Beam,
		Missile,
	};
	static const std::map<Type, UString> TypeMap;

	// Beams have a width & tail length
	// FIXME: Make this a non-constant colour?
	// FIXME: Width is currently just used for drawing - TODO What is "collision" size of beams?
	Projectile(StateRef<Vehicle> firer, Vec3<float> position, Vec3<float> velocity,
	           unsigned int lifetime, unsigned int tail_length,
	           std::list<sp<Image>> projectile_sprites);
	Projectile();
	virtual void update(GameState &state, unsigned int ticks);

	Vec3<float> getVelocity() const { return this->velocity; }
	unsigned int getLifetime() const { return this->lifetime; }
	unsigned int getAge() const { return this->age; }
	sp<Vehicle> getFiredBy() const { return this->firer; }
	Vec3<float> getPosition() const { return this->position; }

	Collision checkProjectileCollision(TileMap &map);

	virtual ~Projectile();

	sp<TileObjectProjectile> tileObject;

	Type type;
	Vec3<float> position;
	Vec3<float> velocity;
	unsigned int age;
	unsigned int lifetime;
	StateRef<Vehicle> firer;
	Vec3<float> previousPosition;

	unsigned int tail_length;
	std::list<sp<Image>> projectile_sprites;

	friend class Weapon;
	friend class TileObjectProjectile;
};
}; // namespace OpenApoc
