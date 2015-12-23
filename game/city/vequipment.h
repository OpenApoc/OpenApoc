#pragma once
#include "library/vec.h"
#include "library/sp.h"

namespace OpenApoc
{

class VEquipmentType;
class VGeneralEquipmentType;
class VWeaponType;
class VEngineType;
class Vehicle;
class Projectile;
class Framework;

class VEquipment
{
  public:
	const VEquipmentType &type;
	virtual ~VEquipment() = default;

	Vec2<int> equippedPosition;

  protected:
	VEquipment(const VEquipmentType &type);
};

class VGeneralEquipment : public VEquipment
{
  public:
	VGeneralEquipment(const VGeneralEquipmentType &type);
	~VGeneralEquipment() override = default;
};

class VEngine : public VEquipment
{
  public:
	VEngine(const VEngineType &type);
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
	std::weak_ptr<Vehicle> owner;
	int ammo;
	int reloadTime;

  public:
	VWeapon(const VWeaponType &type, sp<Vehicle> owner, int initialAmmo,
	        State initialState = State::Ready);
	~VWeapon() override = default;

	float getRange() const;
	bool canFire() const { return state == State::Ready; }
	void update(int ticks);
	void setReloadTime(int ticks);
	// Reload uses up to 'ammoAvailable' to reload the weapon. It returns the amount
	// actually used.
	int reload(int ammoAvailable);
	sp<Projectile> fire(Framework &fw, Vec3<float> target);
};
} // namespace OpenApoc
