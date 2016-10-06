#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gametime.h"
#include "library/sp.h"
#include "library/vec.h"

// Items recharge their recharge rate of ammo every 4 seconds
#define TICKS_PER_RECHARGE 4 * TICKS_PER_SECOND

namespace OpenApoc
{

class BattleItem;
class Projectile;
class AEquipmentType;

class AEquipment
{
  public:
	AEquipment();
	~AEquipment() = default;

	StateRef<AEquipmentType> type;
	// Type of loaded ammunition
	StateRef<AEquipmentType> payloadType;
	// Function to simplify getting payload for weapons and grenades
	StateRef<AEquipmentType> getPayloadType();

	Vec2<int> equippedPosition;
	AgentEquipmentLayout::EquipmentSlotType equippedSlotType =
	    AgentEquipmentLayout::EquipmentSlotType::None;
	StateRef<Agent> ownerAgent;

	// Ammunition for weapons, protection for armor, charge for items
	int ammo = 0;

	unsigned int recharge_ticks_accumulated = 0;

	bool readyToFire = false;
	unsigned int weapon_fire_ticks_remaining = 0;
	BattleUnit::FireAimingMode aimingMode = BattleUnit::FireAimingMode::Aimed;

	int getAccuracy(AgentType::BodyState bodyState, BattleUnit::FireAimingMode fireMode);

	bool isFiring() { return weapon_fire_ticks_remaining > 0 || readyToFire; };
	bool canFire(float range = 0.0f);
	void stopFiring();
	void startFiring(BattleUnit::FireAimingMode fireMode);

	// Following members are not serialized, but rather are set in initBattle method

	wp<BattleItem> ownerItem;

	void update(unsigned int ticks);
	/*
	float getRange() const;
	bool canFire() const;
	void setReloadTime(int ticks);
	// Reload uses up to 'ammoAvailable' to reload the weapon. It returns the amount
	// actually used.
	int reload(int ammoAvailable);
	*/
	sp<Projectile> fire(Vec3<float> targetPosition, StateRef<BattleUnit> targetUnit = nullptr);
};
} // namespace OpenApoc
