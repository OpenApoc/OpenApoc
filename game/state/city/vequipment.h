#pragma once

#include "game/state/shared/equipment.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <map>

namespace OpenApoc
{

class VEquipmentType;
class VGeneralEquipmentType;
class VWeaponType;
class VEngineType;
class Vehicle;
class Projectile;
class Base;

class VEquipment : public Equipment
{
  public:
	VEquipment();
	~VEquipment() = default;

	StateRef<VEquipmentType> type;

	// All equipment state
	Vec2<int> equippedPosition;
	// General equipment state
	// Engine equipment state
	// Weapon equipment state
	enum class WeaponState
	{
		Ready,
		Disabled,
		Reloading,
		OutOfAmmo,
	};
	WeaponState weaponState;
	StateRef<Vehicle> owner;
	int ammo;
	int reloadTime;
	bool disabled = false;

	// All equipment methods
	// General equipment methods
	// Engine equipment methods
	// Weapon equipment methods
	float getRange() const;
	bool canFire() const;
	void update(int ticks);
	void setReloadTime(int ticks);
	// This sends alerts when not enough ammo to reload weapon or engine
	void noAmmoToReload(const GameState &state, const VEquipment *equipment) const;
	// Reload uses up to 'ammoAvailable' to reload the weapon. It returns the amount
	// actually used.
	int reload(int ammoAvailable);
	// Reload from base stores, return true if reloaded
	// Non-players reload from null base for free
	bool reload(GameState &state, StateRef<Base> base);
	// Subtract item from base stores and load ammo
	void equipFromBase(GameState &state, StateRef<Base> base);
	// Add item to base stores and unload ammo
	void unequipToBase(GameState &state, StateRef<Base> base);
	bool fire(GameState &state, Vec3<float> targetPosition, Vec3<float> homingPosition,
	          StateRef<Vehicle> targetVehicle = nullptr, bool manual = false);
	bool fire(GameState &state, Vec3<float> targetPosition,
	          StateRef<Vehicle> targetVehicle = nullptr, bool manual = false);

	sp<Image> getEquipmentArmorImage() const override;
	sp<Image> getEquipmentImage() const override;
	Vec2<int> getEquipmentSlotSize() const override;
};
} // namespace OpenApoc
