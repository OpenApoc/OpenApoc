#include "game/state/battle/battleitem.h"
#include "game/state/aequipment.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/agent.h"
#include "game/state/city/projectile.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "library/sp.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <list>

namespace OpenApoc
{

AEquipment::AEquipment() : equippedPosition(0, 0), ammo(0) {}

int AEquipment::getAccuracy(AgentType::BodyState bodyState, AgentType::MovementState movementState,
                            BattleUnit::FireAimingMode fireMode, bool thrown)
{
	if (!ownerAgent)
	{
		LogError("getAccuracy called on item not in agent's inventory!");
		return 0;
	}
	StateRef<AEquipmentType> payload = getPayloadType();
	if (!payload)
	{
		payload = *type->ammo_types.begin();
	}

	if (this->type->type != AEquipmentType::Type::Weapon && !thrown)
	{
		LogError("getAccuracy (non-thrown) called on non-weapon");
		return 0;
	}

	// Accuracy calculation algorithm

	// Take agent and weapon's accuracy

	auto agentAccuracy = (float)ownerAgent->modified_stats.accuracy;
	auto payloadAccuracy =
	    type->type == AEquipmentType::Type::Weapon ? (float)payload->accuracy : 100.0f;

	// Calculate dispersion, the inverse of accuracy, and scale values to 0-1 for simplicity

	float agentDispersion = 1.0f - agentAccuracy / 100.0f;
	float weaponDispersion = 1.0f - payloadAccuracy / 100.0f;

	if (thrown)
	{
		return (int)agentAccuracy;
		// Throwing accuracy is unaffected by movement, stance or mode of fire
	}
	else
	{
		// Snap and Auto increase agent's dispersion by 2x and 4x respectively
		agentDispersion *= (float)fireMode;

		// Moving also increase it: Moving or flying by 1,35x running by 1,70x
		agentDispersion *=
		    movementState == AgentType::MovementState::None &&
		            bodyState != AgentType::BodyState::Flying
		        ? 1.00f
		        : (movementState == AgentType::MovementState::Running ? 1.70f : 1.35f);

		// Having both hands busy also increases it by another 1,5x
		if (ownerAgent->getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::LeftHand) &&
		    ownerAgent->getFirstItemInSlot(AgentEquipmentLayout::EquipmentSlotType::RightHand))
		{
			agentDispersion *= 1.5f;
		}

		// Kneeling decreases agent's and weapon's dispersion, multiplying it by 0,8x
		if (bodyState == AgentType::BodyState::Kneeling)
		{
			agentDispersion *= 0.8f;
			weaponDispersion *= 0.8f;
		}
	}

	// Calculate total dispersion based on agent and weapon dispersion
	// Let:
	// * qrt(x) = qube root of x
	// * aD = sqrt(agentDispersion / 5)
	// * pD = qrt(weaponDispersion / 10)
	//
	// Then formula of total dispersion would be:
	//   tD = qrt(ad^3 + pd^3)
	// which can be further simplified to:
	//   tD = qrt((agentDispersion / 5) ^ 3/2 + weaponDispersion / 10)
	float totalDispersion =
	    powf(powf(agentDispersion / 5.0f, 3.0f / 2.0f) + weaponDispersion / 10.0f, 1.0f / 3.0f);

	return std::max(0, (int)(100.0f - totalDispersion * 100.0f));
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

void AEquipment::loadAmmo(GameState &state, sp<AEquipment> ammoItem)
{
	// Cannot load if this is using itself for payload or is not a weapon
	if (type->type != AEquipmentType::Type::Weapon || getPayloadType() == type)
	{
		return;
	}
	// If no ammoItem is supplied then look in the agent's inventory
	if (!ammoItem)
	{
		if (equippedSlotType == AgentEquipmentLayout::EquipmentSlotType::None)
		{
			LogError("Trying to reload a weapon not in agent inventory!?");
			return;
		}
		auto it = type->ammo_types.rbegin();
		while (it != type->ammo_types.rend())
		{
			ammoItem = ownerAgent->getFirstItemByType(*it);
			if (ammoItem)
			{
				break;
			}
			it++;
		}
	}
	// Cannot load non-ammo or if no ammo was found in the inventory
	if (!ammoItem || ammoItem->type->type != AEquipmentType::Type::Ammo)
	{
		return;
	}
	// Cannot load if inappropriate type
	if (std::find(type->ammo_types.begin(), type->ammo_types.end(), ammoItem->type) == type->ammo_types.end())
	{
		return;
	}
	
	// If this has ammo then swap
	if (payloadType)
	{
		auto ejectedType = payloadType;
		auto ejectedAmmo = ammo;
		payloadType = ammoItem->type;
		ammo = ammoItem->ammo;
		ammoItem->type = ejectedType;
		ammoItem->ammo = ejectedAmmo;
	}
	else
	{
		payloadType = ammoItem->type;
		ammo = ammoItem->ammo;
		// Remove item from battle/agent
		auto ownerItem = ammoItem->ownerItem.lock();
		if (ownerItem)
		{
			ownerItem->die(state, false);
		}
		else if (ownerAgent && equippedSlotType != AgentEquipmentLayout::EquipmentSlotType::None)
		{
			ownerAgent->removeEquipment(ammoItem);
		}
	}
}

void AEquipment::update(unsigned int ticks)
{
	// Recharge update
	auto payload = getPayloadType();
	if (payload && payload->recharge > 0 && ammo < payload->max_ammo)
	{
		recharge_ticks_accumulated += ticks;
		if (recharge_ticks_accumulated > TICKS_PER_RECHARGE)
		{
			recharge_ticks_accumulated = 0;
			ammo += payload->recharge;
			ammo = std::min(payload->max_ammo, ammo);
		}
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

	auto unit = ownerAgent->unit;
	auto payload = getPayloadType();

	if (payload->damage_type->launcher)
	{
		LogError("fire() called on launcher Weapon");
		return nullptr;
	}

	readyToFire = false;
	ammo--;
	if (ammo == 0 && payload != type)
	{
		payloadType.clear();
	}

	if (payload->fire_sfx)
	{
		fw().soundBackend->playSample(payload->fire_sfx, unit->position);
	}

	auto unitPos = unit->getMuzzleLocation();
	Vec3<float> velocity = targetPosition - unitPos;
	velocity = glm::normalize(velocity);
	velocity *= payload->speed * PROJECTILE_VELOCITY_MULTIPLIER;
	return mksp<Projectile>(payload->guided ? Projectile::Type::Missile : Projectile::Type::Beam,
	                        unit, targetUnit, unitPos, velocity, payload->turn_rate,
	                        payload->ttl * TICKS_MULTIPLIER, payload->damage, payload->tail_size,
	                        payload->projectile_sprites, payload->impact_sfx,
	                        payload->explosion_graphic, payload->damage_type);
}

void AEquipment::launch(Vec3<float> &targetPosition, Vec3<float> &velocity)
{
	if (this->type->type != AEquipmentType::Type::Weapon)
	{
		LogError("fire() called on non-Weapon");
		return;
	}
	if (!readyToFire)
	{
		LogError("fire() called on non-ready Weapon");
		return;
	}

	auto unit = ownerAgent->unit;
	auto payload = getPayloadType();

	if (!payload->damage_type->launcher)
	{
		LogError("launch() called on non-launcher Weapon");
		return;
	}

	readyToFire = false;
	ammo--;

	if (payload->fire_sfx)
	{
		fw().soundBackend->playSample(payload->fire_sfx, unit->position);
	}

	LogError("Implement launchers!");
}

bool AEquipment::isLauncher()
{
	if (this->type->type != AEquipmentType::Type::Weapon)
	{
		LogError("isLauncher() called on non-Weapon");
		return false;
	}
	if (!readyToFire)
	{
		LogError("isLauncher() called on non-ready Weapon");
		return false;
	}
	return getPayloadType()->damage_type->launcher;
}

StateRef<AEquipmentType> AEquipment::getPayloadType()
{
	if (type->type == AEquipmentType::Type::Weapon && type->ammo_types.size() > 0)
	{
		return payloadType;
	}
	return type;
}

bool AEquipment::canFire(float range)
{
	return type->type == AEquipmentType::Type::Weapon && ammo > 0 &&
	       getPayloadType()->getRange() > range;
};

bool AEquipment::needsReload()
{
	return type->type == AEquipmentType::Type::Weapon && ammo == 0 && !type->ammo_types.empty();
}

} // namespace OpenApoc
