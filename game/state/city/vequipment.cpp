#include "game/state/city/city.h"
#include "game/state/city/vequipment.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/projectile.h"
#include "game/state/city/vehicle.h"
#include "game/state/rules/vequipment_type.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "library/sp.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

VEquipment::VEquipment()
    : equippedPosition(0, 0), weaponState(WeaponState::Ready), ammo(0), reloadTime(0)
{
}

sp<Projectile> VEquipment::fire(GameState &state, Vec3<float> targetPosition, StateRef<Vehicle> targetVehicle)
{
	static const std::map<VEquipment::WeaponState, UString> WeaponStateMap = {
	    {WeaponState::Ready, "ready"},
	    {WeaponState::Disabled, "disabled"},
	    {WeaponState::Reloading, "reloading"},
	    {WeaponState::OutOfAmmo, "outofammo"},
	};

	if (this->type->type != EquipmentSlotType::VehicleWeapon)
	{
		LogError("fire() called on non-Weapon");
		return nullptr;
	}
	auto vehicleTile = owner->tileObject;
	if (!vehicleTile)
	{
		LogError("Called on vehicle with no tile object?");
	}
	if (this->weaponState != WeaponState::Ready)
	{
		UString stateName = "UNKNOWN";
		const auto it = WeaponStateMap.find(this->weaponState);
		if (it != WeaponStateMap.end())
			stateName = it->second;
		LogWarning("Trying to fire weapon in state %s", stateName);
		return nullptr;
	}
	if (this->ammo <= 0 && this->type->max_ammo != 0)
	{
		LogWarning("Trying to fire weapon with no ammo");
		return nullptr;
	}
	this->reloadTime = type->fire_delay * VEQUIPMENT_RELOAD_TIME_MULTIPLIER * TICKS_MULTIPLIER;
	this->weaponState = WeaponState::Reloading;
	if (this->type->max_ammo != 0)
		this->ammo--;

	if (type->fire_sfx)
	{
		fw().soundBackend->playSample(type->fire_sfx, vehicleTile->getPosition());
	}

	if (this->ammo == 0 && this->type->max_ammo != 0)
	{
		this->weaponState = WeaponState::OutOfAmmo;
	}
	auto vehicleMuzzle = owner->getMuzzleLocation();

	auto fromScaled = vehicleMuzzle * VELOCITY_SCALE_CITY;
	auto toScaled = targetPosition * VELOCITY_SCALE_CITY;
	// FIXME: Account for target's cloak!
	City::accuracyAlgorithmCity(state, fromScaled, toScaled,
		type->accuracy + owner->getAccuracy(), false);
	
	Vec3<float> velocity = toScaled - fromScaled;
	velocity = glm::normalize(velocity);
	// I believe this is the correct formula
	velocity *= type->speed * PROJECTILE_VELOCITY_MULTIPLIER;
	
	return mksp<Projectile>(type->guided ? Projectile::Type::Missile : Projectile::Type::Beam,
	                        owner, targetVehicle, vehicleMuzzle, velocity, type->turn_rate,
	                        static_cast<int>(this->getRange() / type->speed * TICKS_MULTIPLIER),
	                        type->damage, /*delay*/ 0, type->tail_size, type->projectile_sprites,
	                        type->impact_sfx, type->explosion_graphic);
}

void VEquipment::update(int ticks)
{
	if (this->type->type != EquipmentSlotType::VehicleWeapon)
	{
		LogError("update() called on non-Weapon");
		return;
	}
	if (this->reloadTime != 0)
	{
		if (ticks >= this->reloadTime)
			this->reloadTime = 0;
		else
			this->reloadTime -= ticks;
	}
	switch (this->weaponState)
	{
		case WeaponState::Reloading:
			if (this->reloadTime == 0)
				this->weaponState = WeaponState::Ready;
			return;
		case WeaponState::OutOfAmmo:
			if (this->ammo > 0)
				this->weaponState = WeaponState::Ready;
			return;
		default:
			return;
	}
}

int VEquipment::reload(int ammoAvailable)
{
	if (this->type->type != EquipmentSlotType::VehicleWeapon)
	{
		LogError("reload() called on non-Weapon");
		return 0;
	}
	int ammoRequired = this->type->max_ammo - this->ammo;
	int reloadAmount = std::min(ammoRequired, ammoAvailable);
	this->ammo += reloadAmount;
	return reloadAmount;
}

float VEquipment::getRange() const
{
	if (this->type->type != EquipmentSlotType::VehicleWeapon)
	{
		LogError("getRange() called on non-Weapon");
		return 0;
	}
	auto &type = this->type;
	return type->range;
}

void VEquipment::setReloadTime(int ticks)
{
	if (this->type->type != EquipmentSlotType::VehicleWeapon)
	{
		LogError("setReloadTime() called on non-Weapon");
		return;
	}
	if (ticks <= 0)
	{
		this->reloadTime = 0;
	}
	else
	{
		this->weaponState = WeaponState::Reloading;
		this->reloadTime = ticks;
	}
}

bool VEquipment::canFire() const
{
	if (this->type->type != EquipmentSlotType::VehicleWeapon)
	{
		LogError("canFire() called on non-Weapon");
		return false;
	}
	return this->weaponState == WeaponState::Ready;
}

sp<Image> VEquipment::getEquipmentArmorImage() const
{
	LogError("Vehicle equipment cannot have armor image");
	return 0;
}

sp<Image> VEquipment::getEquipmentImage() const { return this->type->equipscreen_sprite; }

Vec2<int> VEquipment::getEquipmentSlotSize() const { return this->type->equipscreen_size; }

} // namespace OpenApoc
