#include "game/state/aequipment.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "game/state/agent.h"
#include "game/state/city/projectile.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "library/sp.h"
#include <glm/glm.hpp>

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
	       100 / 100 / (int)fireMode;
}

void AEquipment::stopFiring()
{
	weapon_fire_ticks_remaining = 0;
	readyToFire = false;
}

void AEquipment::startFiring(BattleUnit::FireAimingMode fireMode)
{
	if (ammo == 0)
		return;
	weapon_fire_ticks_remaining = getPayloadType()->fire_delay * 4 / (int)fireMode;
	readyToFire = false;
	aimingMode = fireMode;
}

void AEquipment::update(unsigned int ticks)
{
	// Recharge update
	auto payload = getPayloadType();
	if (payload->recharge > 0 && ammo < payload->max_ammo)
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

	// If firing - confirm we're still aiming
	if (isFiring())
	{
		switch (ownerAgent->unit->target_hand_state)
		{
			// If aiming, changing into aiming, or firing, then we're fine
			case AgentType::HandState::Firing:
			case AgentType::HandState::Aiming:
				break;
			// Otherwise stop firing
			default:
				stopFiring();
		}
	}

	// If firing - confirm we're still attacking with it and it's in right place
	if (isFiring())
	{
		switch (equippedSlotType)
		{
			// Check if we're still firing
			case AgentEquipmentLayout::EquipmentSlotType::LeftHand:
				if (ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringBothHands &&
				    ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringLeftHand)
				{
					stopFiring();
				}
				else if (ownerAgent->unit->fire_aiming_mode != aimingMode)
				{
					startFiring(ownerAgent->unit->fire_aiming_mode);
				}
				break;
			case AgentEquipmentLayout::EquipmentSlotType::RightHand:
				if (ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringBothHands &&
				    ownerAgent->unit->weaponStatus != BattleUnit::WeaponStatus::FiringRightHand)
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

sp<Projectile> AEquipment::fire(Vec3<float> targetPosition, StateRef<BattleUnit> targetUnit)
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
	auto unitPos =
	    unit->position + Vec3<float>{0.0f, 0.0f, (float)unit->getCurrentHeight() / 40.0f};
	Vec3<float> velocity = targetPosition - unitPos;
	velocity = glm::normalize(velocity);
	velocity *= payload->speed * TICK_SCALE / 4; // I believe this is the correct formula
	return mksp<Projectile>(payload->guided ? Projectile::Type::Missile : Projectile::Type::Beam,
	                        unit, targetUnit, unitPos, velocity, payload->turn_rate,
	                        payload->ttl * 4, payload->damage, payload->tail_size,
	                        payload->projectile_sprites, payload->impact_sfx,
	                        payload->explosion_graphic, payload->damage_type);
}

bool AEquipment::canFire(float range)
{
	return type->type == AEquipmentType::Type::Weapon && ammo > 0 &&
	       getPayloadType()->getRange() > range;
};

} // namespace OpenApoc
