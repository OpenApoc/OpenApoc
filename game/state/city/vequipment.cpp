#include "game/state/city/vequipment.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/base.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/city/citycommonimagelist.h"
#include "game/state/rules/city/vammotype.h"
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

bool VEquipment::fire(GameState &state, Vec3<float> targetPosition, Vec3<float> homingPosition,
                      StateRef<Vehicle> targetVehicle, bool manual)
{
	static const std::map<VEquipment::WeaponState, UString> WeaponStateMap = {
	    {WeaponState::Ready, "ready"},
	    {WeaponState::Disabled, "disabled"},
	    {WeaponState::Reloading, "reloading"},
	    {WeaponState::OutOfAmmo, "outofammo"},
	};

	auto muzzle = owner->getMuzzleLocation();
	if (!state.current_city->map->tileIsValid(muzzle))
	{
		return false;
	}
	if (this->type->type != EquipmentSlotType::VehicleWeapon)
	{
		LogError("fire() called on non-Weapon");
		return false;
	}
	auto vehicleTile = owner->tileObject;
	if (!vehicleTile)
	{
		LogError("Called on vehicle with no tile object?");
		return false;
	}
	if (this->weaponState != WeaponState::Ready)
	{
		UString stateName = "UNKNOWN";
		const auto it = WeaponStateMap.find(this->weaponState);
		if (it != WeaponStateMap.end())
			stateName = it->second;
		LogWarning("Trying to fire weapon in state %s", stateName);
		return false;
	}
	if (this->ammo <= 0 && this->type->max_ammo != 0)
	{
		LogWarning("Trying to fire weapon with no ammo");
		return false;
	}
	this->reloadTime = type->fire_delay;
	this->weaponState = WeaponState::Reloading;
	if (this->type->max_ammo != 0)
	{
		if (!config().getBool("OpenApoc.Cheat.InfiniteAmmo") ||
		    this->owner->owner != state.getPlayer())
		{
			this->ammo--;
		}
	}

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
	                            targetVehicle && targetVehicle->isCloaked());

	Vec3<float> velocity = toScaled - fromScaled;
	velocity = glm::normalize(velocity);
	// I believe this is the correct formula
	velocity *= type->speed * PROJECTILE_VELOCITY_MULTIPLIER;

	auto projectile = mksp<Projectile>(
	    type->guided ? Projectile::Type::Missile : Projectile::Type::Beam, owner, targetVehicle,
	    homingPosition, muzzle, velocity, type->turn_rate, type->ttl, type->damage, /*delay*/ 0,
	    /*depletion rate*/ 0, type->tail_size, type->projectile_sprites, type->impact_sfx,
	    type->explosion_graphic, state.city_common_image_list->projectileVoxelMap, type->stunTicks,
	    type->splitIntoTypes, manual);
	owner->tileObject->map.addObjectToMap(projectile);
	owner->city->projectiles.insert(projectile);

	return true;
}

bool VEquipment::fire(GameState &state, Vec3<float> targetPosition, StateRef<Vehicle> targetVehicle,
                      bool manual)
{
	return fire(state, targetPosition, targetPosition, targetVehicle, manual);
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

void VEquipment::noAmmoToReload(const GameState &state [[maybe_unused]],
                                const VEquipment *equipment) const
{
	switch (equipment->type->type)
	{
		case EquipmentSlotType::VehicleEngine:
			LogInfo("Failed to refuel engine: %s", owner->name);
			fw().pushEvent(new GameVehicleEvent(GameEventType::NotEnoughFuel, owner));
			break;
		case EquipmentSlotType::VehicleWeapon:
			LogInfo("Failed to rearm weapon: %s", owner->name);
			fw().pushEvent(new GameVehicleEvent(GameEventType::NotEnoughAmmo, owner));
			break;
		case EquipmentSlotType::VehicleGeneral:
			LogWarning("We should not try to reload VehicleGeneral Equipment");
			break;
		default:
			break;
	}
}

int VEquipment::reload(int ammoAvailable)
{
	// TODO implement proper reloading speed. Now it is instant
	int ammoRequired = this->type->max_ammo - this->ammo;
	int reloadAmount = std::min(ammoRequired, ammoAvailable);
	this->ammo += reloadAmount;
	return reloadAmount;
}

bool VEquipment::reload(GameState &state, StateRef<Base> base)
{
	if (base)
	{
		if (type->max_ammo == 0)
		{
			return false;
		}
		if (!type->ammo_type)
		{
			return reload(type->max_ammo) > 0;
		}
		int ammoAvailable = base->inventoryVehicleAmmo[type->ammo_type.id];
		auto ammoSpent = reload(ammoAvailable);
		base->inventoryVehicleAmmo[type->ammo_type.id] -= ammoSpent;
		int ammoAfterReload = base->inventoryVehicleAmmo[type->ammo_type.id];
		// If we run out of ammo/fuel
		if (ammoAfterReload == 0 && ammoSpent > 0)
		{
			noAmmoToReload(state, this);
			return false;
		}
		else
		{
			return ammoSpent > 0;
		}
	}
	else
	{
		auto ammoSpent = reload(type->max_ammo);
		return ammoSpent > 0;
	}
}

void VEquipment::equipFromBase(GameState &state, StateRef<Base> base)
{
	base->inventoryVehicleEquipment[type->id]--;
	reload(state, base);
}

void VEquipment::unequipToBase(GameState &state [[maybe_unused]], StateRef<Base> base)
{
	base->inventoryVehicleEquipment[type.id]++;
	if (ammo > 0 && type->ammo_type)
	{
		base->inventoryVehicleAmmo[type->ammo_type.id] += ammo;
	}
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
	return !disabled && this->weaponState == WeaponState::Ready;
}

sp<Image> VEquipment::getEquipmentArmorImage() const
{
	LogError("Vehicle equipment cannot have armor image");
	return 0;
}

sp<Image> VEquipment::getEquipmentImage() const { return this->type->equipscreen_sprite; }

Vec2<int> VEquipment::getEquipmentSlotSize() const { return this->type->equipscreen_size; }

} // namespace OpenApoc
