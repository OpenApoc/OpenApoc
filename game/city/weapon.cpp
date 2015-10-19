#include "library/sp.h"
#include "game/city/weapon.h"
#include "framework/logger.h"
#include "game/city/vehicle.h"
#include "game/tileview/beam_projectile.h"

namespace OpenApoc
{

Weapon::Weapon(const WeaponDef &def, sp<Vehicle> owner, int initialAmmo, State initialState)
    : state(initialState), def(def), owner(owner), ammo(initialAmmo), reloadTime(0)
{
}
sp<Projectile> Weapon::fire(Vec3<float> target)
{
	auto owner = this->owner.lock();
	if (!owner)
	{
		LogError("Called on weapon with no owner?");
	}
	auto vehicleTile = owner->tileObject.lock();
	if (!vehicleTile)
	{
		LogError("Called on vehicle with no tile object?");
	}
	if (this->state != State::Ready)
	{
		LogWarning("Trying to fire weapon in state %d", this->state);
		return nullptr;
	}
	if (this->ammo <= 0)
	{
		LogWarning("Trying to fire weapon with no ammo");
		return nullptr;
	}
	this->reloadTime = this->def.firingDelay;
	this->state = State::Reloading;
	this->ammo--;

	if (this->ammo == 0)
	{
		this->state = State::OutOfAmmo;
	}

	switch (this->def.projectileType)
	{
		case WeaponDef::ProjectileType::Beam:
		{
			Vec3<float> velocity = target - vehicleTile->getPosition();
			velocity = glm::normalize(velocity);
			velocity *= this->def.projectileSpeed;
			auto &map = vehicleTile->getOwningTile()->map;
			return std::make_shared<BeamProjectile>(
			    map, owner, vehicleTile->getPosition(), velocity,
			    static_cast<int>(def.range / this->def.projectileSpeed), def.beamColour,
			    def.projectileTailLength, def.beamWidth);
		}
		default:
			LogWarning("Unknown projectile type");
	}

	return nullptr;
}

void Weapon::update(int ticks)
{
	if (this->reloadTime != 0)
	{
		if (ticks >= this->reloadTime)
			this->reloadTime = 0;
		else
			this->reloadTime -= ticks;
	}
	switch (this->state)
	{
		case State::Reloading:
			if (this->reloadTime == 0)
				this->state = State::Ready;
			return;
		default:
			return;
	}
}

}; // namespace OpenApoc
