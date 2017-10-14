#include "game/state/city/vequipment.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/citycommonimagelist.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include "library/sp.h"
#include <glm/glm.hpp>

namespace OpenApoc
{

VEquipment::VEquipment()
    : equippedPosition(0, 0), weaponState(WeaponState::Ready), ammo(0), reloadTime(0)
{
}

void VEquipment::fire(GameState &state, Vec3<float> targetPosition, StateRef<Vehicle> targetVehicle,
                      bool manual)
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
		return;
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
		return;
	}
	if (this->ammo <= 0 && this->type->max_ammo != 0)
	{
		LogWarning("Trying to fire weapon with no ammo");
		return;
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
	City::accuracyAlgorithmCity(state, fromScaled, toScaled, type->accuracy + owner->getAccuracy(),
	                            targetVehicle && targetVehicle->hasCloak());

	Vec3<float> velocity = toScaled - fromScaled;
	velocity = glm::normalize(velocity);
	// I believe this is the correct formula
	velocity *= type->speed * PROJECTILE_VELOCITY_MULTIPLIER;

	if (state.current_city->map->tileIsValid(vehicleMuzzle))
	{
		auto projectile = mksp<Projectile>(
		    type->guided ? Projectile::Type::Missile : Projectile::Type::Beam, owner, targetVehicle,
		    targetPosition, vehicleMuzzle, velocity, type->turn_rate, type->ttl * TICKS_MULTIPLIER,
		    type->damage,
		    /*delay*/ 0, type->tail_size, type->projectile_sprites, type->impact_sfx,
		    type->explosion_graphic,
		    type->guided ? state.city_common_image_list->projectileVoxelMap : nullptr, manual);
		vehicleTile->map.addObjectToMap(projectile);
		state.current_city->projectiles.insert(projectile);
	}
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
