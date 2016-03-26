#pragma once
#include "game/rules/vequipment.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class VEquipmentType;
class VGeneralEquipmentType;
class VWeaponType;
class VEngineType;
class Vehicle;
class Projectile;

class VEquipment
{
  public:
	VEquipment();
	~VEquipment() = default;

	StateRef<VEquipmentType> type;

	// All equipment state
	Vec2<int> equippedPosition;
	// General equipment state
	// Engine equipemnt state
	// Weapon equipment state
	enum class WeaponState
	{
		Ready,
		Disabled,
		Reloading,
		OutOfAmmo,
	};
	static const std::map<WeaponState, UString> WeaponStateMap;
	WeaponState weaponState;
	StateRef<Vehicle> owner;
	int ammo;
	int reloadTime;

	// All equipment methods
	// General equipment methods
	// Engine equipment methods
	// Weapon equipment methods
	float getRange() const;
	bool canFire() const;
	void update(int ticks);
	void setReloadTime(int ticks);
	// Reload uses up to 'ammoAvailable' to reload the weapon. It returns the amount
	// actually used.
	int reload(int ammoAvailable);
	sp<Projectile> fire(Vec3<float> target);
};
} // namespace OpenApoc
