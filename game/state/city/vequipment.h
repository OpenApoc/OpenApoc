#pragma once

#include "game/state/shared/equipment.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <map>

// Alexey Andronov (Istrebitel):
// It has been observed that vehicle weapons reload too quickly when compared to vanilla
// Introducing a multiplier of 2 to their reload time seems to bring them to
// comparable times. However, this may be wrong.
#define VEQUIPMENT_RELOAD_TIME_MULTIPLIER 2

namespace OpenApoc
{

class VEquipmentType;
class VGeneralEquipmentType;
class VWeaponType;
class VEngineType;
class Vehicle;
class Projectile;

class VEquipment : public Equipment
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
	void fire(GameState &state, Vec3<float> targetPosition,
	          StateRef<Vehicle> targetVehicle = nullptr, bool manual = false);

	sp<Image> getEquipmentArmorImage() const override;
	sp<Image> getEquipmentImage() const override;
	Vec2<int> getEquipmentSlotSize() const override;
};
} // namespace OpenApoc
