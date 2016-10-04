#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/tileview/tile.h"
#include "library/sp.h"
#include "library/strings.h"
#include <algorithm>

// How many in-game ticks are required to travel one in-game unit
#define TICKS_PER_UNIT_TRAVELLED 32
// How many in-game ticks are required to pass 1 animation frame
#define TICKS_PER_FRAME_UNIT 8
// Every this amount of units travelled, unit will emit an appropriate sound
#define UNITS_TRAVELLED_PER_SOUND 12
// If running, unit will only emit sound every this number of times it normally would
#define UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR 2
// This defines how fast a flying unit accelerates to full speed
#define FLYING_ACCELERATION_DIVISOR 2

#define FALLING_ACCELERATION_UNIT 0.16666667f // 1/6th

#define LOS_CHECK_INTERVAL_TRACKING 36

namespace OpenApoc
{

class TileObjectBattleUnit;
class TileObjectShadow;
class Battle;

class BattleUnit : public StateObject<BattleUnit>, public std::enable_shared_from_this<BattleUnit>
{
  public:
	// Enums for player selectable modes for the agent
	enum class BehaviorMode
	{
		Aggressive,
		Normal,
		Evasive
	};
	enum class FireAimingMode
	{
		Aimed = 1,
		Snap = 2,
		Auto = 4
	};
	enum class FirePermissionMode
	{
		AtWill,
		CeaseFire
	};
	enum class MovementMode
	{
		Running,
		Walking,
		Prone
	};
	enum class KneelingMode
	{
		None,
		Kneeling
	};

	// Enum for tracking unit's weapon state
	enum class WeaponStatus
	{
		NotFiring,
		FiringLeftHand,
		FiringRightHand,
		FiringBothHands
	};

	// Enum for tracking unit's targeting mode
	enum class TargetingMode
	{
		NoTarget,
		Unit,
		TileCenter,
		TileGround
	};

	UString id;

	StateRef<Agent> agent;

	StateRef<Organisation> owner;
	// Squad number, -1 = not assigned to any squad
	int squadNumber = 0;
	// Squad position, has no meaning if not in squad
	unsigned int squadPosition = 0;
	void removeFromSquad();
	bool assignToSquad(int squadNumber);
	void moveToSquadPosition(int squadPosition);

	// Weapon state and firing

	WeaponStatus weaponStatus = WeaponStatus::NotFiring;
	TargetingMode targetingMode = TargetingMode::NoTarget;
	// Tile we're targeting right now (or tile with unit we're targeting)
	Vec3<int> targetTile;
	// Unit we're targeting right now
	StateRef<BattleUnit> targetUnit;
	// Unit we're ordered to focus on (in real time)
	StateRef<BattleUnit> focusUnit;
	// Units focusing this unit
	// Ticks until we check if target is still valid, turn to it etc.
	unsigned int ticksTillNextTargetCheck = 0;

	void setFocus(StateRef<BattleUnit> unit);
	void startAttacking(StateRef<BattleUnit> unit,
	                    WeaponStatus status = WeaponStatus::FiringBothHands);
	void startAttacking(Vec3<int> tile, WeaponStatus status = WeaponStatus::FiringBothHands,
	                    bool atGround = false);
	void stopAttacking();

	// Stats

	// Accumulated xp points for each stat
	AgentStats experiencePoints;
	// Fatal wounds for each body part
	std::map<AgentType::BodyPart, int> fatalWounds;
	bool isFatallyWounded();
	// Which body part is medikit used on
	AgentType::BodyPart healingBodyPart = AgentType::BodyPart::Body;
	// Is using a medikit
	bool isHealing = false;
	// Ticks until next wound damage or medikit heal is applied
	int woundTicksAccumulated = 0;
	// Stun damage acquired
	int stunDamageInTicks = 0;
	int getStunDamage() const;
	void dealStunDamage(int damage);

	// User set modes

	BehaviorMode behavior_mode = BehaviorMode::Normal;
	FireAimingMode fire_aiming_mode = FireAimingMode::Snap;
	FirePermissionMode fire_permission_mode = FirePermissionMode::AtWill;
	MovementMode movement_mode = MovementMode::Walking;
	KneelingMode kneeling_mode = KneelingMode::None;

	// Animation frames and state

	// Time, in game ticks, until body animation is finished
	unsigned int body_animation_ticks_remaining = 0;
	unsigned int getBodyAnimationFrame() const
	{
		return (body_animation_ticks_remaining + TICKS_PER_FRAME_UNIT - 1) / TICKS_PER_FRAME_UNIT;
	}
	AgentType::BodyState current_body_state = AgentType::BodyState::Standing;
	AgentType::BodyState target_body_state = AgentType::BodyState::Standing;
	void setBodyState(AgentType::BodyState state);
	void beginBodyStateChange(AgentType::BodyState state);

	// Time, in game ticks, until hands animation is finished
	unsigned int hand_animation_ticks_remaining = 0;
	// Time, in game ticks, until unit will lower it's weapon
	unsigned int aiming_ticks_remaining = 0;
	// Time, in game ticks, until firing animation is finished
	unsigned int firing_animation_ticks_remaining = 0;
	// Get hand animation frame to draw
	unsigned int getHandAnimationFrame() const
	{
		return ((firing_animation_ticks_remaining > 0 ? firing_animation_ticks_remaining
		                                              : hand_animation_ticks_remaining) +
		        TICKS_PER_FRAME_UNIT - 1) /
		       TICKS_PER_FRAME_UNIT;
	}
	AgentType::HandState current_hand_state = AgentType::HandState::AtEase;
	AgentType::HandState target_hand_state = AgentType::HandState::AtEase;
	void setHandState(AgentType::HandState state);
	void beginHandStateChange(AgentType::HandState state);

	// Distance, in movement ticks, spent since starting to move
	unsigned int movement_ticks_passed = 0;
	unsigned int getDistanceTravelled() const
	{
		return movement_ticks_passed / TICKS_PER_UNIT_TRAVELLED;
	}

	// Movement sounds
	unsigned int movement_sounds_played = 0;
	bool shouldPlaySoundNow();
	unsigned int getWalkSoundIndex();
	AgentType::MovementState current_movement_state = AgentType::MovementState::None;
	void setMovementState(AgentType::MovementState state);

	// Time, in game ticks, until unit can turn by 1/8th of a circle
	unsigned int turning_animation_ticks_remaining = 0;
	void beginTurning(Vec2<int> newFacing);

	// Mission logic

	std::list<up<BattleUnitMission>> missions;
	// Pops all finished missions, returns true if unit retreated and you should return
	bool popFinishedMissions(GameState &state);
	bool getNextDestination(GameState &state, Vec3<float> &dest);
	bool getNextFacing(GameState &state, Vec2<int> &dest);
	bool getNextBodyState(GameState &state, AgentType::BodyState &dest);
	bool addMission(GameState &state, BattleUnitMission *mission, bool start = true);
	bool addMission(GameState &state, BattleUnitMission::MissionType type);

	Vec3<float> position;
	Vec3<float> goalPosition;
	bool atGoal = false;
	bool usingLift = false;
	Vec2<int> facing;
	Vec2<int> goalFacing;
	// Value 0 - 100, Simulates slow takeoff, when starting to use flight this slowly rises to 1
	unsigned int flyingSpeedModifier = 0;

	// Successfully retreated from combat
	bool retreated = false;

	// Died and corpse was destroyed in an explosion
	bool destroyed = false;

	// Freefalling
	bool falling = false;
	float fallingSpeed = 0.0f;

	// If unit is asked to give way, this list will be filled with facings
	// in order of priority that should be tried by it
	std::list<Vec2<int>> giveWayRequest;

	StateRef<AEquipmentType> displayedItem;
	void updateDisplayedItem();

	// Returns true if the unit is dead
	bool isDead() const;
	// Returns true if the unit is unconscious and not dead
	bool isUnconscious() const;
	// Return true if the unit is conscious
	// and not currently in the dropped state (curent and target)
	bool isConscious() const;
	// So we can stop going ->isLarge() all the time
	bool isLarge() const { return agent->type->bodyType->large; }
	// Wether unit is static - not moving, not falling, not changing body state
	bool isStatic() const;
	// Wether unit is busy - with aiming or firing or otherwise involved
	bool isBusy() const;
	// Wether unit is firing its weapon (or aiming in preparation of firing)
	bool isAttacking() const;
	// Wether unit is throwing an item
	bool isThrowing() const;

	// Returns true if the unit is conscious and can fly
	bool canFly() const;
	// Returns true if the unit is conscious and can fly
	bool canMove() const;
	// Wether unit can go prone in position and facing
	bool canProne(Vec3<int> pos, Vec2<int> fac) const;
	// Wether unit can kneel in current position and facing
	bool canKneel() const;

	// Get unit's height in current situation
	int getCurrentHeight() const { return agent->type->bodyType->height.at(current_body_state); }

	// TU functions
	// Wether unit can afford action
	bool canAfford(int cost) const;
	// Returns if unit did spend (false if unsufficient TUs)
	bool spendTU(int cost);

	bool applyDamage(GameState &state, int damage, float armour);
	void handleCollision(GameState &state, Collision &c);
	// sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile);
	// void attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile, sp<TileObjectVehicle>
	// enemyTile);

	const Vec3<float> &getPosition() const { return this->position; }

	float getMaxThrowDistance(int weight, int heightDifference);

	int getMaxHealth() const;
	int getHealth() const;

	int getMaxShield() const;
	int getShield() const;

	void setPosition(const Vec3<float> &pos);
	void resetGoal();

	void update(GameState &state, unsigned int ticks);

	void retreat(GameState &state);
	void dropDown(GameState &state);
	void tryToRiseUp(GameState &state);
	void fallUnconscious(GameState &state);
	void die(GameState &state, bool violently);
	void destroy(GameState &state);

	// Following members are not serialized, but rather are set in initBattle method

	std::list<sp<BattleUnit>> focusedByUnits;

	sp<TileObjectBattleUnit> tileObject;
	sp<TileObjectShadow> shadowObject;
	wp<Battle> battle;

	/*
	- curr. mind state (controlled/berserk/…)
	- ref. to psi attacker (who is controlling it/...)
	*/
  private:
	void startAttacking(WeaponStatus status);
};
}
