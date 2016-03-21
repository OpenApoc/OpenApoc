#pragma once
#include "library/vec.h"
#include "library/sp.h"
#include "game/rules/vequipment.h"

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
	StateRef<VEquipmentType> type;
	virtual ~VEquipment() = default;

	Vec2<int> equippedPosition;

  protected:
	VEquipment(StateRef<VEquipmentType> type);
};

class VGeneralEquipment : public VEquipment
{
  public:
	VGeneralEquipment(StateRef<VEquipmentType> type);
	~VGeneralEquipment() override = default;
};

class VEngine : public VEquipment
{
  public:
	VEngine(StateRef<VEquipmentType> type);
	~VEngine() override = default;
};

class VWeapon : public VEquipment
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
	StateRef<Vehicle> owner;
	int ammo;
	int reloadTime;

  public:
	VWeapon(StateRef<VEquipmentType>, StateRef<Vehicle> owner, int initialAmmo,
	        State initialState = State::Ready);
	~VWeapon() override = default;

	float getRange() const;
	bool canFire() const { return state == State::Ready; }
	void update(int ticks);
	void setReloadTime(int ticks);
	// Reload uses up to 'ammoAvailable' to reload the weapon. It returns the amount
	// actually used.
	int reload(int ammoAvailable);
	sp<Projectile> fire(Vec3<float> target);
};
} // namespace OpenApoc
