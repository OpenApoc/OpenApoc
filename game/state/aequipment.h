#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunit.h"
#include "game/state/rules/aequipment_type.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class BattleItem;
// class Projectile;

class AEquipment
{
  public:
	AEquipment();
	~AEquipment() = default;

	StateRef<AEquipmentType> type;
	// Type of loaded ammunition
	StateRef<AEquipmentType> payloadType;
	StateRef<AEquipmentType> getPayloadType();

	Vec2<int> equippedPosition;
	StateRef<Agent> ownerAgent;
	// Ammunition for weapons, protection for armor, charge for items
	int ammo = 0;

	bool aiming = false;
	int weapon_fire_ticks_remaining = 0;

	int getAccuracy(AgentType::BodyState bodyState, BattleUnit::FireAimingMode fireMode);

	// Following members are not serialized, but rather are set in initBattle method

	wp<BattleItem> ownerItem;

	void update(int ticks);
	/*
	float getRange() const;
	bool canFire() const;
	void setReloadTime(int ticks);
	// Reload uses up to 'ammoAvailable' to reload the weapon. It returns the amount
	// actually used.
	int reload(int ammoAvailable);
	sp<Projectile> fire(Vec3<float> target);
	*/
};
} // namespace OpenApoc
