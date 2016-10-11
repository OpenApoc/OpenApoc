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

class AEquipment : public std::enable_shared_from_this<AEquipment>
{
  public:
	enum class TriggerType
	{
		None,
		Timed,
		Contact,
		Proximity,
		Boomeroid,
	};

	AEquipment();
	~AEquipment() = default;

	StateRef<AEquipmentType> type;
	// Type of loaded ammunition
	StateRef<AEquipmentType> payloadType;
	// Function to simplify getting payload for weapons and grenades
	StateRef<AEquipmentType> getPayloadType();

	Vec2<int> equippedPosition;
	AEquipmentSlotType equippedSlotType = AEquipmentSlotType::None;
	StateRef<Agent> ownerAgent;

	// Ammunition for weapons, protection for armor, charge for items
	int ammo = 0;

	// Explosives parameters

	// If set, will activate upon leaving agent inventory
	bool primed = false;
	// If set, will count down timer and go off when trigger condition is satisfied
	bool activated = false;
	// Delay until trigger is activated
	unsigned int triggerDelay = 0;
	// Range for proximity
	float triggerRange = 0.0f;
	// Type of trigger used
	TriggerType triggerType = TriggerType::None;

	void prime(bool onImpact = true, int triggerDelay = 0, float triggerRange = 0.0f);
	void explode(GameState &state);

	// Ticks until recharge rate is added to ammo for anything that recharges
	unsigned int recharge_ticks_accumulated = 0;

	// Weapon parameters

	// Weapon is ready to fire now
	bool readyToFire = false;
	// Ticks until weapon is ready to fire
	unsigned int weapon_fire_ticks_remaining = 0;
	// Aiming mode for the weapon
	BattleUnit::FireAimingMode aimingMode = BattleUnit::FireAimingMode::Aimed;

	int getAccuracy(BodyState bodyState, MovementState movementState,
	                BattleUnit::FireAimingMode fireMode, bool thrown = false);

	bool isFiring() { return weapon_fire_ticks_remaining > 0 || readyToFire; };
	bool canFire(float range = 0.0f);
	bool needsReload();
	void stopFiring();
	void startFiring(BattleUnit::FireAimingMode fireMode);

	// Support nullptr ammoItem for auto-reloading
	void loadAmmo(GameState &state, sp<AEquipment> ammoItem = nullptr);

	// Following members are not serialized, but rather are set in initBattle method

	wp<BattleItem> ownerItem;

	void update(GameState &state, unsigned int ticks);
	/*
	float getRange() const;
	bool canFire() const;
	void setReloadTime(int ticks);
	// Reload uses up to 'ammoAvailable' to reload the weapon. It returns the amount
	// actually used.
	int reload(int ammoAvailable);
	*/
	// Wether this weapon works like brainsucker launcher, throwing it's ammunition instead of
	// firing a projectile
	bool isLauncher();
	sp<Projectile> fire(GameState &state, Vec3<float> targetPosition, Vec3<float> originalTarget,
	                    StateRef<BattleUnit> targetUnit = nullptr);
	void launch(Vec3<float> &targetPosition, Vec3<float> &velocity);
};
} // namespace OpenApoc
