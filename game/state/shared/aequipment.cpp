#include "game/state/shared/aequipment.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/options.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlescanner.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/rules/battle/damage.h"
#include "game/state/shared/agent.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "library/sp.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <list>

namespace OpenApoc
{

AEquipment::AEquipment()
    : equippedPosition(0, 0), ammo(0), triggerType(TriggerType::None),
      aimingMode(WeaponAimingMode::Aimed)
{
}

/*
// Alexey Andronov (Istrebitel):
// Just in case we need to check, here's old formula I have hand-calculated
// based on values tested from the game

int AEquipment::getAccuracy(BodyState bodyState, MovementState movementState,
                            WeaponAimingMode fireMode, bool thrown)
{
    float accuracy = 0.0f;
    auto agent = ownerAgent ? ownerAgent : (ownerUnit ? ownerUnit->agent : nullptr);
    if (agent)
    {
        accuracy = agent->modified_stats.accuracy;
    }

    if (thrown)
    {
        return (int)accuracy;
        // Throwing accuracy is unaffected by movement, stance or mode of fire
    }

    StateRef<AEquipmentType> payload = getPayloadType();
    if (!payload)
    {
        payload = *type->ammo_types.begin();
    }

    if (this->type->type != AEquipmentType::Type::Weapon)
    {
        LogError("getAccuracy (non-thrown) called on non-weapon");
        return 0;
    }

    // Accuracy calculation algorithm

    // Take agent and weapon's accuracy

    auto agentAccuracy = accuracy;
    auto payloadAccuracy =
        type->type == AEquipmentType::Type::Weapon ? (float)payload->accuracy : 100.0f;

    // Calculate dispersion, the inverse of accuracy, and scale values to 0-1 for simplicity

    float agentDispersion = 1.0f - agentAccuracy / 100.0f;
    float weaponDispersion = 1.0f - payloadAccuracy / 100.0f;


    // Snap and Auto increase agent's dispersion by 2x and 4x respectively
    agentDispersion *= (float)fireMode;

    // Moving also increase it: Moving or flying by 1,35x running by 1,70x
    agentDispersion *= movementState == MovementState::None && bodyState != BodyState::Flying
        ? 1.00f
        : (movementState == MovementState::Running ? 1.70f : 1.35f);

    // Having both hands busy also increases it by another 1,5x
    if (ownerAgent && (equippedSlotType == EquipmentSlotType::LeftHand ||
        equippedSlotType == EquipmentSlotType::RightHand) &&
        ownerAgent->getFirstItemInSlot(EquipmentSlotType::LeftHand) &&
        ownerAgent->getFirstItemInSlot(EquipmentSlotType::RightHand))
    {
        agentDispersion *= 1.5f;
    }

    // Kneeling decreases agent's and weapon's dispersion, multiplying it by 0,8x
    if (bodyState == BodyState::Kneeling)
    {
        agentDispersion *= 0.8f;
        weaponDispersion *= 0.8f;
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

*/

int AEquipment::getAccuracy(BodyState bodyState, MovementState movementState,
                            WeaponAimingMode fireMode, bool thrown)
{
	float accuracy = 0.0f;
	auto agent = ownerAgent ? ownerAgent : (ownerUnit ? ownerUnit->agent : nullptr);
	if (agent)
	{
		accuracy = agent->modified_stats.accuracy;
	}

	if (thrown)
	{
		return (int)accuracy;
		// Throwing accuracy is unaffected by movement, stance or mode of fire
	}

	StateRef<AEquipmentType> payload = getPayloadType();
	if (!payload)
	{
		payload = *type->ammo_types.begin();
	}

	if (this->type->type != AEquipmentType::Type::Weapon)
	{
		LogError("getAccuracy (non-thrown) called on non-weapon");
		return 0;
	}

	// Accuracy calculation algorithm

	// Take agent and weapon's accuracy

	auto agentAccuracy = accuracy;
	auto payloadAccuracy =
	    type->type == AEquipmentType::Type::Weapon ? (float)payload->accuracy : 100.0f;

	// Calculate dispersion, the inverse of accuracy, and scale values to 0-1 for simplicity

	float agentDispersion = 1.0f - agentAccuracy / 100.0f;
	float weaponDispersion = 1.0f - payloadAccuracy / 100.0f;

	// Weapon dispersion is half as important
	weaponDispersion *= 0.5f;

	// Firing mode: Auto increases dispersion and Aimed decreases by factors of 2
	agentDispersion *= 0.5f * (float)fireMode;

	// Moving affects dispersion
	switch (movementState)
	{
		case MovementState::None:
			agentDispersion *= 0.8f;
			break;
		case MovementState::Running:
			agentDispersion *= 1.25f;
			break;
		default:
			agentDispersion *= 0.95f;
			break;
	}

	// Body state affects dispersion
	switch (bodyState)
	{
		case BodyState::Flying:
			agentDispersion *= 1.15f;
			break;
		case BodyState::Kneeling:
			agentDispersion *= 0.8f;
			break;
		case BodyState::Prone:
			agentDispersion *= 0.6f;
			break;
		default:
			// no change
			break;
	}

	// Checks that apply only if weapon is in agent inventory
	float healthDispersion = 0.0f;
	float woundDispersion = 0.0f;
	float berserkDispersion = 0.0f;
	if (ownerAgent)
	{
		auto unit = ownerAgent->unit;

		// Checks that apply only if weapon is in hand
		if (equippedSlotType == EquipmentSlotType::LeftHand ||
		    equippedSlotType == EquipmentSlotType::RightHand)
		{
			// Big guns have a penalty if other hand is occupied : 1.4x dispersion
			if (type->equipscreen_size.x * type->equipscreen_size.y > 4 &&
			    ownerAgent->getFirstItemInSlot(EquipmentSlotType::LeftHand) &&
			    ownerAgent->getFirstItemInSlot(EquipmentSlotType::RightHand))
			{
				agentDispersion *= 1.4f;
			}
		}

		// 1.5 dispersion for every fatal wound in the arm used to hold the weapon
		// Assume right hand if not yet holding it in hands
		if (ownerAgent->type->inventory)
		{
			woundDispersion = 1.5f * (float)(equippedSlotType == EquipmentSlotType::LeftHand
			                                     ? unit->fatalWounds[BodyPart::LeftArm]
			                                     : unit->fatalWounds[BodyPart::RightArm]);
		}
		// Wound dispersion for aliens without inventory
		else
		{
			int totalWounds = 0;
			for (auto &entry : unit->fatalWounds)
			{
				totalWounds += entry.second;
			}
			woundDispersion = totalWounds;
		}

		// Health penalty
		healthDispersion =
		    (float)(ownerAgent->current_stats.health - ownerAgent->modified_stats.health) /
		    (float)ownerAgent->modified_stats.health;

		// Berserk penalty
		berserkDispersion = unit->moraleState == MoraleState::Berserk ? 2.0f : 0.0f;
	}

	float totalDispersion =
	    sqrtf(agentDispersion * agentDispersion + weaponDispersion * weaponDispersion +
	          healthDispersion * healthDispersion + woundDispersion * woundDispersion +
	          berserkDispersion * berserkDispersion);
	return std::max(0, (int)(100.0f - totalDispersion * 100.0f));
}

int AEquipment::getWeight() const
{
	return type->weight + ((payloadType && ammo > 0) ? payloadType->weight : 0);
}

int AEquipment::getFireCost(WeaponAimingMode fireMode)
{
	return getPayloadType()->fire_delay / TICKS_MULTIPLIER / (int)fireMode;
}

int AEquipment::getFireCost(WeaponAimingMode fireMode, int maxTU)
{
	return getFireCost(fireMode) * maxTU / 100;
}

void AEquipment::stopFiring()
{
	weapon_fire_ticks_remaining = 0;
	readyToFire = false;
}

void AEquipment::startFiring(WeaponAimingMode fireMode, bool instant)
{
	if (ammo == 0)
		return;
	if (instant)
	{
		weapon_fire_ticks_remaining = TICKS_PER_SECOND / 4;
	}
	else
	{
		weapon_fire_ticks_remaining = getPayloadType()->fire_delay / (int)fireMode;
	}
	readyToFire = false;
	aimingMode = fireMode;
}

static sp<AEquipment> getAutoreloadAmmoType(const AEquipment &weapon)
{
	LogAssert(weapon.type->type == AEquipmentType::Type::Weapon);
	LogAssert(weapon.getPayloadType() != weapon.type);

	// Check if the auto-reload option is disabled
	if (!Options::optionAutoReload.get())
	{
		return nullptr;
	}

	// Prefer loading the same type of ammo again
	if (weapon.lastLoadedAmmoType)
	{
		auto equippedAmmo = weapon.ownerAgent->getFirstItemByType(weapon.lastLoadedAmmoType);
		if (equippedAmmo)
		{
			LogAssert(equippedAmmo->type->type == AEquipmentType::Type::Ammo);
			return equippedAmmo;
		}
	}
	// Otherwise find any possible ammo on the agent
	for (const auto &ammoType : weapon.type->ammo_types)
	{
		auto equippedAmmo = weapon.ownerAgent->getFirstItemByType(ammoType);
		if (equippedAmmo)
		{
			LogAssert(equippedAmmo->type->type == AEquipmentType::Type::Ammo);
			return equippedAmmo;
		}
	}
	// No ammo found
	return nullptr;
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
		if (!ownerAgent)
		{
			LogError("Trying to auto-reload a weapon not in agent inventory!?");
			return;
		}
		ammoItem = getAutoreloadAmmoType(*this);
	}
	// Cannot load non-ammo or if no ammo was found in the inventory
	if (!ammoItem || ammoItem->type->type != AEquipmentType::Type::Ammo)
	{
		return;
	}
	// Cannot load if inappropriate type
	if (std::find(type->ammo_types.begin(), type->ammo_types.end(), ammoItem->type) ==
	    type->ammo_types.end())
	{
		return;
	}
	// Store the loaded ammo type for autoreload
	lastLoadedAmmoType = ammoItem->type;

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
		// Spend ammo
		ammoItem->ammo = 0;
		// Remove item from battle/agent
		auto ownerItem = ammoItem->ownerItem.lock();
		if (ownerItem)
		{
			ownerItem->die(state, false);
		}
		else if (ownerAgent)
		{
			ownerAgent->removeEquipment(state, ammoItem);
		}
	}
}

sp<AEquipment> AEquipment::unloadAmmo()
{
	if (!payloadType)
	{
		return nullptr;
	}
	auto clip = mksp<AEquipment>();
	clip->type = payloadType;
	clip->ammo = ammo;
	payloadType = nullptr;
	ammo = 0;
	return clip;
}

void AEquipment::updateInner(GameState &state, unsigned int ticks)
{
	// Recharge update
	auto payload = getPayloadType();
	if (payload && payload->recharge > 0 && ammo < payload->max_ammo)
	{
		if (state.current_battle->mode == Battle::Mode::TurnBased)
		{
			ammo += payload->rechargeTB;
			ammo = std::min(payload->max_ammo, ammo);
		}
		else
		{
			recharge_ticks_accumulated += ticks;
			if (recharge_ticks_accumulated > TICKS_PER_RECHARGE)
			{
				recharge_ticks_accumulated = 0;
				ammo += payload->recharge;
				ammo = std::min(payload->max_ammo, ammo);
			}
		}
	}

	// Process primed explosives
	if (primed && activated)
	{
		if (triggerDelay > ticks)
		{
			triggerDelay -= ticks;
		}
		else
		{
			triggerDelay = 0;
		}
	}
}

bool AEquipment::canBeUsed(GameState &state) const
{
	if (ownerAgent)
	{
		return type->canBeUsed(state, ownerAgent->owner);
	}
	return true;
}

void AEquipment::update(GameState &state, unsigned int ticks)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;

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

	if (isFiring())
	{
		// If firing - confirm we're still in inventory
		// If firing - confirm we're still aiming
		if (!ownerAgent || ownerAgent->unit->target_hand_state == HandState::AtEase)
		{
			stopFiring();
		}
		else
		{
			// If firing - confirm we're still attacking with it and it's in right place
			switch (equippedSlotType)
			{
				// Check if we're still firing
				case EquipmentSlotType::LeftHand:
					if (ownerAgent->unit->weaponStatus != WeaponStatus::FiringBothHands &&
					    ownerAgent->unit->weaponStatus != WeaponStatus::FiringLeftHand)
					{
						stopFiring();
					}
					else if (ownerAgent->unit->fire_aiming_mode != aimingMode)
					{
						startFiring(ownerAgent->unit->fire_aiming_mode, !realTime);
					}
					break;
				case EquipmentSlotType::RightHand:
					if (ownerAgent->unit->weaponStatus != WeaponStatus::FiringBothHands &&
					    ownerAgent->unit->weaponStatus != WeaponStatus::FiringRightHand)
					{
						stopFiring();
					}
					else if (ownerAgent->unit->fire_aiming_mode != aimingMode)
					{
						startFiring(ownerAgent->unit->fire_aiming_mode, !realTime);
					}
					break;
				// If weapon was moved to any other slot from hands, stop firing
				default:
					stopFiring();
					break;
			}
		}
	}

	if (inUse)
	{
		// If in use - confirm we're still in agent's inventory
		if (!ownerAgent)
		{
			inUse = false;
			if (battleScanner)
			{
				state.current_battle->removeScanner(state, *this);
			}
		}
		// If in use - confirm we're still in the right place
		else
		{
			switch (equippedSlotType)
			{
				case EquipmentSlotType::LeftHand:
				case EquipmentSlotType::RightHand:
					break;
				default:
					inUse = false;
					if (battleScanner)
					{
						state.current_battle->removeScanner(state, *this);
					}
					break;
			}
		}
	}

	if (primed && !activated && !ownerAgent)
	{
		activated = true;
	}

	if (realTime)
	{
		updateInner(state, ticks);
	}

	if (primed && activated && triggerDelay == 0)
	{
		auto payload = getPayloadType();
		switch (triggerType)
		{
			case TriggerType::None:
				LogError("Primed activated item with no trigger?");
				break;
			case TriggerType::Contact:
			{
				auto item = ownerItem.lock();
				if (item)
				{
					if (!item->falling)
					{
						item->die(state);
					}
				}
				else
				{
					// Contact trigger in inventory? Blow up!
					explode(state);
				}
				break;
			}
			case TriggerType::Proximity:
			case TriggerType::Boomeroid:
			{
				auto item = ownerItem.lock();
				if (item)
				{
					// Nothing, triggered by moving units
				}
				else
				{
					// Proxy trigger in inventory? Blow up!
					if (payload->damage_type->effectType != DamageType::EffectType::Brainsucker)
					{
						explode(state);
					}
				}
				break;
			}
			case TriggerType::Timed:
			{
				auto item = ownerItem.lock();
				if (item)
				{
					item->die(state);
				}
				else
				{
					explode(state);
				}
				break;
			}
		}
	}
}

void AEquipment::updateTB(GameState &state) { updateInner(state, TICKS_PER_TURN); }

void AEquipment::prime(bool onImpact, int triggerDelay, float triggerRange)
{
	if (type->type != AEquipmentType::Type::Grenade && type->type != AEquipmentType::Type::Ammo)
	{
		return;
	}

	if (onImpact)
	{
		triggerType = TriggerType::Contact;
	}
	else
	{
		triggerType = type->trigger_type;
	}
	this->triggerDelay = triggerDelay;
	this->triggerRange = triggerRange;
	this->primed = true;
}

void AEquipment::explode(GameState &state)
{
	Vec3<float> position;
	auto item = ownerItem.lock();
	if (item)
	{
		position = item->position;
	}
	else
	{
		position = ownerAgent->unit->position;
	}
	if (ownerAgent)
	{
		if (!ownerUnit)
		{
			ownerUnit = ownerAgent->unit;
		}
		ownerAgent->removeEquipment(state, shared_from_this());
	}
	switch (type->type)
	{
		case AEquipmentType::Type::Grenade:
			state.current_battle->addExplosion(
			    state, position, type->explosion_graphic, type->damage_type, type->damage,
			    type->explosion_depletion_rate,
			    ownerUnit ? ownerUnit->owner : (ownerAgent ? ownerAgent->owner : ownerOrganisation),
			    ownerUnit);
			break;
		case AEquipmentType::Type::Weapon:
		case AEquipmentType::Type::Ammo:
		{
			auto payload = getPayloadType();
			// If no payload or brainsucker then nothing
			if (!payload || payload->damage_type->effectType == DamageType::EffectType::Brainsucker)
			{
				break;
			}
			if (!config().getBool("OpenApoc.NewFeature.PayloadExplosion"))
			{
				break;
			}
			// If explosive just blow up
			if (payload->damage_type->explosive)
			{
				state.current_battle->addExplosion(
				    state, position, payload->explosion_graphic, payload->damage_type,
				    payload->damage, payload->explosion_depletion_rate,
				    ownerUnit ? ownerUnit->owner
				              : (ownerAgent ? ownerAgent->owner : ownerOrganisation),
				    ownerUnit);
				break;
			}
			// Otherwise shoot stray shots into air
			auto shots = std::min(ammo, (int)MAX_PAYLOAD_EXPLOSION_SHOTS);
			auto bItem = ownerItem.lock();
			if (!bItem)
			{
				LogError("WTF? Exploding ammo/weapon in agent inventory???");
				break;
			}
			while (shots-- > 0)
			{
				auto velocity =
				    Vec3<float>{(float)randBoundsInclusive(state.rng, -1000, 1000) / 1000.0f,
				                (float)randBoundsInclusive(state.rng, -1000, 1000) / 1000.0f,
				                (float)randBoundsInclusive(state.rng, 1, 1000) / 1000.0f};
				velocity = glm::normalize(velocity);
				velocity *= payload->speed * PROJECTILE_VELOCITY_MULTIPLIER;
				auto p = mksp<Projectile>(
				    payload->guided ? Projectile::Type::Missile : Projectile::Type::Beam, ownerUnit,
				    nullptr, Vec3<float>{0.0f, 0.0f, 0.0f},
				    position + Vec3<float>{0.0f, 0.0f, 0.33f}, velocity, 0, payload->ttl,
				    payload->damage, /*delay*/ 0, payload->explosion_depletion_rate,
				    payload->tail_size, payload->projectile_sprites, payload->impact_sfx,
				    payload->explosion_graphic, payload->damage_type);
				state.current_battle->map->addObjectToMap(p);
				state.current_battle->projectiles.insert(p);
			}
			break;
		}
		default:
			LogWarning("Implement blown up payload firing in all directions etc.");
			break;
	}
}

void AEquipment::fire(GameState &state, Vec3<float> targetPosition, StateRef<BattleUnit> targetUnit)
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
	Vec3<float> originalTarget = targetPosition;

	if (payload->fire_sfx)
	{
		fw().soundBackend->playSample(payload->fire_sfx, unit->position);
	}

	if (type->launcher)
	{
		auto item = mksp<AEquipment>();
		item->type = payload;
		item->ammo = 1;
		if (payload->damage_type->effectType == DamageType::EffectType::Brainsucker)
		{
			item->prime(false, TICKS_PER_TURN, 10.0f);
		}
		else
		{
			item->prime();
		}
		item->ownerUnit = ownerAgent->unit;
		float velocityXY = 0.0f;
		float velocityZ = 0.0f;
		if (!getVelocityForLaunch(*unit, targetPosition, velocityXY, velocityZ))
		{
			LogError("Firing a launcher with no valid trajectory?");
			return;
		}
		// Throw item (accuracy applied inside)
		item->throwItem(state, targetPosition, velocityXY, velocityZ, true);
	}
	else
	{
		auto unitPos = unit->getMuzzleLocation();

		auto fromPos = unitPos * VELOCITY_SCALE_BATTLE;
		auto toPos = targetPosition * VELOCITY_SCALE_BATTLE;
		// Apply accuracy algorithm
		Battle::accuracyAlgorithmBattle(state, fromPos, toPos,
		                                getAccuracy(unit->current_body_state,
		                                            unit->current_movement_state,
		                                            unit->fire_aiming_mode),
		                                targetUnit && targetUnit->isCloaked());
		// Fire
		Vec3<float> velocity = toPos - fromPos;
		velocity = glm::normalize(velocity);
		// Move projectile a little bit forward so that it does not shoot from inside our chest
		// We are protecting firer from collision for first frames anyway, so this is redundant
		// for all cases except when a unit fires with a brainsucker on it's head!
		// But this also looks better since it does visually fire from the muzzle, not from inside
		// the soldier
		unitPos += velocity * 3.5f / 8.0f;
		// Scale velocity according to speed
		velocity *= payload->speed * PROJECTILE_VELOCITY_MULTIPLIER;

		if (state.current_battle->map->tileIsValid(unitPos))
		{
			auto p = mksp<Projectile>(
			    payload->guided ? Projectile::Type::Missile : Projectile::Type::Beam, unit,
			    targetUnit, originalTarget, unitPos, velocity, payload->turn_rate, payload->ttl,
			    payload->damage, payload->projectile_delay, payload->explosion_depletion_rate,
			    payload->tail_size, payload->projectile_sprites, payload->impact_sfx,
			    payload->explosion_graphic, payload->damage_type);
			state.current_battle->map->addObjectToMap(p);
			state.current_battle->projectiles.insert(p);
		}
	}

	readyToFire = false;
	if (!config().getBool("OpenApoc.Cheat.InfiniteAmmo") ||
	    this->ownerAgent->owner != state.getPlayer())
	{
		ammo--;
	}
	if (ammo == 0 && payloadType)
	{
		payloadType.clear();
		loadAmmo(state);
		ownerAgent->updateSpeed();
	}
}

void AEquipment::throwItem(GameState &state, Vec3<int> targetPosition, float velocityXY,
                           float velocityZ, bool launch)
{
	auto &unit = *ownerUnit;
	Vec3<float> position = unit.getThrownItemLocation();
	if (state.battle_common_sample_list->throwSounds.size() > 0)
	{
		fw().soundBackend->playSample(
		    pickRandom(state.rng, state.battle_common_sample_list->throwSounds), position);
	}

	// This will be modified by the accuracy algorithm
	Vec3<float> targetLocationModified =
	    Vec3<float>(targetPosition) + Vec3<float>{0.5f, 0.5f, 0.0f};
	// This is proper target vector, stored to get difference later
	Vec3<float> targetVector = targetLocationModified - position;
	// Apply accuracy (if launching apply normal, narrower spread)
	Battle::accuracyAlgorithmBattle(state, position, targetLocationModified,
	                                getAccuracy(unit.current_body_state,
	                                            unit.current_movement_state, unit.fire_aiming_mode,
	                                            true),
	                                false, !launch);
	Vec3<float> targetVectorModified = targetLocationModified - position;
	// Calculate difference in lengths to modify velocity
	float targetVectorDifference = glm::length(targetVectorModified) / glm::length(targetVector);

	velocityXY *= targetVectorDifference;
	velocityZ *= targetVectorDifference;

	auto bi = state.current_battle->placeItem(state, shared_from_this(), position);

	bi->velocity =
	    (glm::normalize(Vec3<float>{targetVectorModified.x, targetVectorModified.y, 0.0f}) *
	         velocityXY +
	     Vec3<float>{0.0f, 0.0f, velocityZ}) *
	    VELOCITY_SCALE_BATTLE;
	bi->falling = true;
	// 36 / (velocity length) = enough ticks to pass 1 whole tile
	bi->ownerInvulnerableTicks =
	    (int)ceilf(36.0f / glm::length(bi->velocity / VELOCITY_SCALE_BATTLE)) + 1;
}

StateRef<AEquipmentType> AEquipment::getPayloadType() const
{
	if (type->type == AEquipmentType::Type::Weapon && type->ammo_types.size() > 0)
	{
		return payloadType;
	}
	return type;
}

bool AEquipment::canFire(GameState &state) const
{
	return (type->type == AEquipmentType::Type::Weapon && ammo > 0 && canBeUsed(state));
}

bool AEquipment::canFire(GameState &state, Vec3<float> to) const
{
	if (!canFire(state))
		return false;
	float distanceToTarget =
	    glm::length((ownerAgent->unit->getMuzzleLocation() - to) * VELOCITY_SCALE_BATTLE);
	if (getPayloadType()->range < distanceToTarget)
		return false;
	if (!type->launcher)
	{
		return true;
	}
	// Launchers need to additionally confirm a throw trajectory to target
	float ignore1, ignore2;
	return getVelocityForLaunch(*ownerAgent->unit, to, ignore1, ignore2);
};

bool AEquipment::needsReload() const
{
	return type->type == AEquipmentType::Type::Weapon && ammo == 0 && !type->ammo_types.empty();
}

// Alexey Andronov: Istrebitel
// Made up values calculated by trying several throws in game
// This formula closely resembles results I've gotten
// But it may be completely wrong
float AEquipment::getMaxThrowDistance(int weight, int strength, int heightDifference)
{
	static float max = 30.0f;
	if (weight <= 2)
	{
		return max;
	}
	int mod = heightDifference > 0 ? heightDifference : heightDifference * 2;
	return std::max(0.0f, std::min(max, (float)strength / ((float)weight - 1) - 2 + mod));
}

// Calculate starting velocity among xy and z to reach target
bool AEquipment::calculateNextVelocityForThrow(float distanceXY, float diffZ, float &velocityXY,
                                               float &velocityZ)
{
	static float dZ = 0.2f;

	// Initial setup
	// Start with X = 2.0f on first try, this is max speed item can have on XY
	if (velocityXY == 0.0f)
	{
		velocityXY = 2.0f;
	}
	else
	{
		velocityXY -= dZ;
	}

	// For simplicity assume moving only along X
	//
	// t = time, in ticks
	// VelocityZ(t) = VelocityZ0 - (Falling_Acceleration / VELOCITY_SCALE_Z) * t;
	// Let:	a = -Falling_acc / VELOCITY_SCALE_Z/ 2 / TICK_SCALE,
	//		b = a + VelocityZ / TICK_SCALE,
	//		c = diffZ
	// z(t) = a*t^2 + b*t + c
	//
	// x = coordinate on the tile grid, in tiles
	// x = t * VelocityX / TICK_SCALE
	// t = x * TICK_SCALE / VelocityX
	//
	// We need to find VelocityZ, given a, t, c, that will produce a desired throw
	// a*t^2 + b*t + c = 0 (hit the target at desired distance)
	// b*t = -c-a*t^2
	// b =  (-c-a*t^2)/t
	// VelocityZ = TICK_SCALE * ((-c -a * t^2) / t - a)
	//
	// However, item must fall from above to the target
	// Therefore, it's VelocityZ when arriving at target must be negative, and big enough
	// If it's not, we must reduce VelocityX

	float a = -FALLING_ACCELERATION_ITEM / VELOCITY_SCALE_BATTLE.z / 2.0f / TICK_SCALE;
	float c = diffZ;
	float t = 0.0f;

	// We will continue reducing velocityXY  until we find such a trajectory
	// that makes the item fall on top of the tile
	while (velocityXY > 0.0f)
	{
		// FIXME: Should we prevent very high Z-trajectories, unrealistic for heavy items?
		t = distanceXY * TICK_SCALE / (velocityXY);
		velocityZ = TICK_SCALE * ((-c - a * t * t) / t - a);
		if (velocityZ - (FALLING_ACCELERATION_ITEM / VELOCITY_SCALE_BATTLE.z) * t < -0.125f)
		{
			return true;
		}
		else
		{
			velocityXY -= dZ;
		}
	}
	return false;
}

bool AEquipment::getVelocityForThrowLaunch(const BattleUnit *unit, const TileMap &map, int strength,
                                           int weight, Vec3<float> startPos, Vec3<int> target,
                                           float &velocityXY, float &velocityZ)
{
	// Check distance to target
	Vec3<int> pos = startPos;
	Vec3<float> targetVectorXY = target - pos;
	targetVectorXY = {targetVectorXY.x, targetVectorXY.y, 0.0f};
	float distance = glm::length(targetVectorXY);
	if (distance >= getMaxThrowDistance(weight, strength, pos.z - target.z))
	{
		return false;
	}

	// Calculate trajectory
	bool valid = true;
	while (AEquipment::calculateNextVelocityForThrow(distance, startPos.z - target.z - 6.0f / 40.0f,
	                                                 velocityXY, velocityZ))
	{
		valid = map.checkThrowTrajectory(unit ? unit->tileObject : nullptr, startPos, target,
		                                 targetVectorXY, velocityXY, velocityZ);
		if (valid)
		{
			break;
		}
	}
	return valid;
}
bool AEquipment::getCanThrow(const TileMap &map, int strength, Vec3<float> startPos,
                             Vec3<int> target)
{
	float nothing1 = 0.0f;
	float nothing2 = 0.0f;
	return getVelocityForThrow(map, strength, startPos, target, nothing1, nothing2);
}

bool AEquipment::getVelocityForThrow(const TileMap &map, int strength, Vec3<float> startPos,
                                     Vec3<int> target, float &velocityXY, float &velocityZ) const
{
	return getVelocityForThrowLaunch(nullptr, map, strength, getWeight(), startPos, target,
	                                 velocityXY, velocityZ);
}

bool AEquipment::getCanThrow(const BattleUnit &unit, Vec3<int> target)
{
	float nothing1 = 0.0f;
	float nothing2 = 0.0f;
	return getVelocityForThrow(unit, target, nothing1, nothing2);
}

bool AEquipment::getVelocityForThrow(const BattleUnit &unit, Vec3<int> target, float &velocityXY,
                                     float &velocityZ) const
{
	return getVelocityForThrowLaunch(&unit, unit.tileObject->map,
	                                 unit.agent->modified_stats.strength, getWeight(),
	                                 unit.getThrownItemLocation(), target, velocityXY, velocityZ);
}

bool AEquipment::getVelocityForLaunch(const BattleUnit &unit, Vec3<int> target, float &velocityXY,
                                      float &velocityZ) const
{
	// Launchers use weapon's projectile speed as their XY speed
	// Item's velocityXY is later multiplied by velocity_scale
	// Projectile's speed is already in that scale, but is later multiplied by velocity_multiplier
	// Therefore we multiply by velocity_mult and divide by scale to convert proj speed to item
	// speed
	velocityXY =
	    (float)getPayloadType()->speed * PROJECTILE_VELOCITY_MULTIPLIER / VELOCITY_SCALE_BATTLE.x;
	return getVelocityForThrowLaunch(&unit, unit.tileObject->map,
	                                 unit.agent->modified_stats.strength, payloadType->weight,
	                                 unit.getThrownItemLocation(), target, velocityXY, velocityZ);
}

sp<Image> AEquipment::getEquipmentArmorImage() const { return this->type->body_sprite; }

sp<Image> AEquipment::getEquipmentImage() const { return this->type->equipscreen_sprite; }

Vec2<int> AEquipment::getEquipmentSlotSize() const { return this->type->equipscreen_size; }

} // namespace OpenApoc
