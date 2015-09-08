#pragma once
#include "game/tileview/tile.h"

namespace OpenApoc
{

class Vehicle;
class Weapon;

class Projectile : virtual public TileObject
{
  protected:
	Vec3<float> velocity;
	unsigned int age;
	unsigned int lifetime;
	std::shared_ptr<Vehicle> firer;

	Vec3<float> previousPosition;
	friend class Weapon;

  public:
	Projectile(TileMap &map, std::shared_ptr<Vehicle> firer, Vec3<float> position,
	           Vec3<float> velocity, unsigned int lifetime);
	virtual void checkProjectileCollision() override;
	virtual void update(unsigned int ticks) override;

	virtual Vec3<float> getVelocity() const { return this->velocity; }
	virtual unsigned int getLifetime() const { return this->lifetime; }
	virtual unsigned int getAge() const { return this->age; }
	virtual std::shared_ptr<Vehicle> getFiredBy() const { return this->firer; }

	virtual void drawProjectile(TileView &v, Renderer &r, Vec2<int> screenPosition) override = 0;

	virtual ~Projectile() = default;
};
}; // namespace OpenApoc
