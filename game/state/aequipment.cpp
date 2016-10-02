#include "game/state/aequipment.h"
#include "framework/framework.h"
#include "game/state/city/projectile.h"
#include "framework/logger.h"
#include "game/state/agent.h"
#include "game/state/rules/aequipment_type.h"
#include "library/sp.h"
#include "game/state/tileview/tileobject_battleunit.h"

namespace OpenApoc
{

AEquipment::AEquipment() : equippedPosition(0, 0), ammo(0) {}

int AEquipment::getAccuracy(AgentType::BodyState bodyState, BattleUnit::FireAimingMode fireMode)
{
	if (!ownerAgent)
	{
		LogError("getAccuracy called on item not in agent's inventory!");
		return 0;
	}
	if (type->type != AEquipmentType::Type::Weapon)
	{
		LogError("getAccuracy called on non-weapon!");
		return 0;
	}
	// FIXME: Proper algorithm for calcualting weapon's accuracy
	StateRef<AEquipmentType> payload = getPayloadType();
	if (!payload)
	{
		payload = *type->ammo_types.begin();
	}
	return ownerAgent->modified_stats.accuracy * payload->accuracy *
	       (bodyState == AgentType::BodyState::Flying
	            ? 90
	            : (bodyState == AgentType::BodyState::Kneeling ? 110 : 100)) /
	       100 / 100  * (int)fireMode / 4;
}

void AEquipment::stopFiring()
{
	weapon_fire_ticks_remaining = 0;
	readyToFire = false;
}

void  AEquipment::startFiring(BattleUnit::FireAimingMode fireMode)
{
	if (ammo == 0)
		return;
	weapon_fire_ticks_remaining = getPayloadType()->fire_delay * (int)fireMode;
	readyToFire = false;
	aimingMode = fireMode;
}

void AEquipment::update(unsigned int ticks)
{
	// Recharge update
	auto payload = getPayloadType();
	if (payload->recharge > 0 && ammo < payload->max_ammo);
	{
		recharge_ticks_accumulated += ticks;
	}
	if (recharge_ticks_accumulated > TICKS_PER_RECHARGE)
	{
		recharge_ticks_accumulated = 0;
		ammo += payload->recharge;
		ammo = std::min(payload->max_ammo, ammo);
	}

	// Firing update
	if (weapon_fire_ticks_remaining > 0)
	{
		if (weapon_fire_ticks_remaining > ticks)
		{
			weapon_fire_ticks_remaining -= ticks;
		}
		else
		{
			weapon_fire_ticks_remaining = 0;
			readyToFire = true;
		}
	}
	
	// If firing - confirm we're still in business
	if (isFiring())
	{
		switch (equippedSlotType)
		{
		// Check if we're still firing
		case AgentEquipmentLayout::EquipmentSlotType::LeftHand:
			if (ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringBothHands
				&&ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringLeftHand)
			{
				stopFiring();
			}
			else if (ownerAgent->unit->fire_aiming_mode != aimingMode)
			{
				startFiring(ownerAgent->unit->fire_aiming_mode);
			}
			break;
		case AgentEquipmentLayout::EquipmentSlotType::RightHand:
			if (ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringBothHands
				&&ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringRightHand)
			{
				stopFiring();
			}
			else if (ownerAgent->unit->fire_aiming_mode != aimingMode)
			{
				startFiring(ownerAgent->unit->fire_aiming_mode);
			}
			break;
		// If weapon was dropped, we should stop firing
		case AgentEquipmentLayout::EquipmentSlotType::None:
		// If weapon was moved to any other slot from hands, stop firing
		default:
			stopFiring();
			break;
		}
	}

	// FIXME: Update equipment (grenades etc.)
}

sp<Projectile> AEquipment::fire(Vec3<float> target)
{
	if (this->type->type != AEquipmentType::Type::Weapon)
	{
		LogError("fire() called on non-Weapon");
		return nullptr;
	}
	if (!readyToFire)
	{
		LogError("fire() called on non-ready Weapon");
		return nullptr;
	}
	
	readyToFire = false;
	auto unit = ownerAgent->unit;
	auto payload = getPayloadType();

	ammo--;

	if (payload->fire_sfx)
	{
		fw().soundBackend->playSample(payload->fire_sfx, unit->position);
	}

	// FIXME: Apply accuracy
	Vec3<float> velocity = target - unit->tileObject->getCenter();
	velocity = glm::normalize(velocity) * VELOCITY_SCALE_BATTLE;
	velocity *= payload->speed;

	return mksp<Projectile>(unit, unit->tileObject->getCenter(), velocity,
		payload->ttl * 4, payload->damage, payload->tail_size, payload->projectile_sprites);
}

bool AEquipment::canFire(float range)
{ 
	return type->type == AEquipmentType::Type::Weapon 
		&& ammo > 0 
		&& getPayloadType()->getRange() > range; 
};

} // namespace OpenApoc
