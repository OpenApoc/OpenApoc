#include "game/city/weapon.h"
#include "framework/logger.h"
#include "game/city/vehicle.h"

namespace OpenApoc
{

Weapon::Weapon(const WeaponDef &def, std::shared_ptr<Vehicle> owner, int initialAmmo, State initialState)
	: state(initialState), def(def), owner(owner), ammo(initialAmmo), reloadTime(0)
{

}
std::shared_ptr<Projectile>
Weapon::fire(Vec3<float> target)
{
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
	LogWarning("Bang!");
	this->reloadTime = this->def.firingDelay;
	this->state = State::Reloading;
	this->ammo--;
	return nullptr;
}

void
Weapon::update(int ticks)
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


}; //namespace OpenApoc
