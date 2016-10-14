#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battle.h"
#include "game/state/gametime.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{
// Items recharge their recharge rate of ammo every 4 seconds (or fully recharge every turn in TB)
static const unsigned TICKS_PER_RECHARGE = TICKS_PER_TURN;

class BattleItem;
class BattleUnit;
class Projectile;
class AEquipmentType;
enum class TriggerType;
enum class WeaponAimingMode;

class AEquipment : public std::enable_shared_from_this<AEquipment>
{
  public:
	AEquipment();
	~AEquipment() = default;

	StateRef<AEquipmentType> type;
	// Type of loaded ammunition
	StateRef<AEquipmentType> payloadType;
	// Function to simplify getting payload for weapons and grenades
	StateRef<AEquipmentType> getPayloadType() const;

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
	TriggerType triggerType;

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
	WeaponAimingMode aimingMode;

	int getAccuracy(BodyState bodyState, MovementState movementState, WeaponAimingMode fireMode,
	                bool thrown = false);

	bool isFiring() const { return weapon_fire_ticks_remaining > 0 || readyToFire; };
	bool canFire() const;
	bool canFire(Vec3<float> to) const;
	bool needsReload() const;
	void stopFiring();
	void startFiring(WeaponAimingMode fireMode);

	// Support nullptr ammoItem for auto-reloading
	void loadAmmo(GameState &state, sp<AEquipment> ammoItem = nullptr);

	// Following members are not serialized, but rather are set in initBattle method

	wp<BattleItem> ownerItem;

	void update(GameState &state, unsigned int ticks);

	// Wether this weapon works like brainsucker launcher, throwing it's ammunition instead of
	// firing a projectile
	bool isLauncher();
	void fire(GameState &state, Vec3<float> targetPosition,
	          StateRef<BattleUnit> targetUnit = nullptr);
	void throwItem(GameState &state, Vec3<int> targetPosition, float velocityXY, float velocityZ,
	               bool launch = false);

	bool getVelocityForThrow(const sp<BattleUnit> unit, Vec3<int> target, float &velocityXY,
	                         float &velocityZ) const;
	bool getVelocityForLaunch(const sp<BattleUnit> unit, Vec3<int> target, float &velocityXY,
	                          float &velocityZ) const;

  private:
	static float getMaxThrowDistance(int weight, int strength, int heightDifference);
	static bool calculateNextVelocityForThrow(float distanceXY, float diffZ, float &velocityXY,
	                                          float &velocityZ);
	static bool getVelocityForThrowLaunch(const sp<BattleUnit> unit, int weight,
	                                      Vec3<float> startPos, Vec3<int> target, float &velocityXY,
	                                      float &velocityZ);
};
} // namespace OpenApoc
