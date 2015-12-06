#include "library/sp.h"
#include "game/city/vequipment.h"
#include "game/rules/vequipment.h"
#include "framework/logger.h"
#include "framework/framework.h"
#include "game/city/vehicle.h"
#include "game/city/projectile.h"
#include "game/tileview/tileobject_vehicle.h"

namespace OpenApoc
{

VWeapon::VWeapon(const VWeaponType &type, sp<Vehicle> owner, int initialAmmo, State initialState)
    : VEquipment(type), state(initialState), owner(owner), ammo(initialAmmo), reloadTime(0)
{
}
sp<Projectile> VWeapon::fire(Framework &fw, Vec3<float> target)
{
	auto &weaponType = static_cast<const VWeaponType &>(this->type);
	auto owner = this->owner.lock();
	if (!owner)
	{
		LogError("Called on weapon with no owner?");
	}
	auto vehicleTile = owner->tileObject;
	if (!vehicleTile)
	{
		LogError("Called on vehicle with no tile object?");
	}
	if (this->state != State::Ready)
	{
		LogWarning("Trying to fire weapon in state %d", this->state);
		return nullptr;
	}
	if (this->ammo <= 0 && this->type.max_ammo != 0)
	{
		LogWarning("Trying to fire weapon with no ammo");
		return nullptr;
	}
	this->reloadTime = weaponType.fire_delay * TICK_SCALE;
	this->state = State::Reloading;
	if (this->type.max_ammo != 0)
		this->ammo--;

	if (weaponType.fire_sfx)
	{
		fw.soundBackend->playSample(weaponType.fire_sfx);
	}

	if (this->ammo == 0 && this->type.max_ammo != 0)
	{
		this->state = State::OutOfAmmo;
	}

	Vec3<float> velocity = target - vehicleTile->getPosition();
	velocity = glm::normalize(velocity);
	velocity *= weaponType.speed;

	Colour c;
	// FIXME: Implement 'proper' weapon projectile images
	// For now just change the colour so you can at least see they're different
	switch (weaponType.projectile_image)
	{
		case 0: // Missile ("GLM array" "janitor" "prophet")
		case 1: // Bigger missile ("GLM air defence" "justice" "retribution"
			c = {255, 0, 0, 255}; // Red missile
			break;
		case 2: // Bullet ("40mm auto cannon/turret" "airguard" "rumble cannon")
			c = {139, 69, 19, 255}; // Brown
			break;
		case 3: // Small laser ("laser defence array")
		case 4: // Medium Laser ("bolter")
		case 5: // Big laser ("lancer")
			c = {255, 255, 0, 255}; // Yellow
			break;
		case 6: // Small plasma ("plasma defence array")
		case 7: // Medium plasma ("rendor" "plasma multi-system")
		case 8: // Big plasma ("lineage" "plasma turret cannon")
			c = {255, 255, 255, 255}; // White
			break;
		case 9: // Small disruptor ("light disruptor beam")
		case 10: // Medium disruptor ("medium disruptor beam")
		case 11: // Big disruptor ("heavy disruptor beam")
			c = {255, 0, 255, 255}; // Magenta
			break;
		case 12: // disruptor bomb
		case 13: // stasis bomb
		case 14: // multi-bomb
			c = {255, 128, 255, 255}; // Pink
			break;
		default:
			LogError("Unknown projectile_image \"%d\" for type \"%s\"", weaponType.projectile_image,
			         weaponType.id.c_str());
	}

	return std::make_shared<Projectile>(
	    owner, vehicleTile->getPosition(), velocity,
	    static_cast<int>(this->getRange() / weaponType.speed * TICK_SCALE), c, weaponType.tail_size,
	    2.0f);

	return nullptr;
}

void VWeapon::update(int ticks)
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
		case State::OutOfAmmo:
			if (this->ammo > 0)
				this->state = State::Ready;
			return;
		default:
			return;
	}
}

int VWeapon::reload(int ammoAvailable)
{
	int ammoRequired = this->type.max_ammo - this->ammo;
	int reloadAmount = std::min(ammoRequired, ammoAvailable);
	this->ammo += reloadAmount;
	return reloadAmount;
}

float VWeapon::getRange() const
{
	auto &weaponType = static_cast<const VWeaponType &>(this->type);
	return weaponType.range;
}

}; // namespace OpenApoc
