#pragma once

#include "game/rules/weapondef.h"

#include "library/vec.h"

namespace OpenApoc
{
class Vehicle;
class Projectile;
class Weapon
{
  public:
	enum class State
	{
		Ready,
		Disabled,
		Reloading,
		OutOfAmmo,
	};

  private:
	State state;
	const WeaponDef &def;
	std::weak_ptr<Vehicle> owner;
	int ammo;
	int reloadTime;

  public:
	Weapon(const WeaponDef &def, std::shared_ptr<Vehicle> owner, int initialAmmo,
	       State initialState = State::Ready);

	const WeaponDef &getWeaponDef() const { return def; }
	bool canFire() const { return state == State::Ready; }
	void update(int ticks);
	std::shared_ptr<Projectile> fire(Vec3<float> target);
};

}; // namespace OpenApoc
