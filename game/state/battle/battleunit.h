#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/tileview/tile.h"
#include "library/sp.h"
#include "library/strings.h"
#include <algorithm>

#define TICKS_PER_UNIT_TRAVELLED 32
#define TICKS_PER_FRAME 8
// Every this amount of units travelled, unit will emit an appropriate sound
#define UNITS_TRAVELLED_PER_SOUND 12
// If running, unit will only emit sound every this number of times it normally would
#define UNITS_TRAVELLED_PER_SOUND_RUNNING_DIVISOR 2
#define FLYING_ACCELERATION_DIVISOR 2

namespace OpenApoc
{

class TileObjectBattleUnit;
class TileObjectShadow;
class Battle;

class BattleUnit : public std::enable_shared_from_this<BattleUnit>
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
		Aimed,
		Snap,
		Auto
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

	StateRef<Agent> agent;

	StateRef<Organisation> owner;
	int squadNumber = 0;
	int squadPosition = 0;
	bool assignToSquad(int squadNumber);
	void moveToSquadPosition(int squadPosition);

	AgentStats experiencePoints;
	std::map<AgentType::BodyPart, int> fatalWounds;
	bool isFatallyWounded();
	AgentType::BodyPart healingBodyPart = AgentType::BodyPart::Body;
	bool isHealing = false;
	int woundTicksAccumulated = 0;

	// User set modes
	BehaviorMode behavior_mode = BehaviorMode::Normal;
	FireAimingMode fire_aiming_mode = FireAimingMode::Snap;
	FirePermissionMode fire_permission_mode = FirePermissionMode::AtWill;
	MovementMode movement_mode = MovementMode::Walking;
	KneelingMode kneeling_mode = KneelingMode::None;

	// Animation frames and state

	// Time, in game ticks, until body animation is finished
	int body_animation_ticks_remaining = 0;
	int getBodyAnimationFrame() const
	{
		return (body_animation_ticks_remaining + TICKS_PER_FRAME - 1) / TICKS_PER_FRAME;
	}
	AgentType::BodyState current_body_state = AgentType::BodyState::Standing;
	AgentType::BodyState target_body_state = AgentType::BodyState::Standing;
	void setBodyState(AgentType::BodyState state);
	void beginBodyStateChange(AgentType::BodyState state, int ticks);
	// Time, in game ticks, until hands animation is finished
	int hand_animation_ticks_remaining = 0;
	int getHandAnimationFrame() const
	{
		return (hand_animation_ticks_remaining + TICKS_PER_FRAME - 1) / TICKS_PER_FRAME;
	}
	AgentType::HandState current_hand_state = AgentType::HandState::AtEase;
	AgentType::HandState target_hand_state = AgentType::HandState::AtEase;
	void setHandState(AgentType::HandState state);
	// Distance, in movement ticks, spent since starting to move
	int movement_ticks_passed = 0;
	int getDistanceTravelled() const
	{
		return std::max(0, movement_ticks_passed / TICKS_PER_UNIT_TRAVELLED);
	}
	// Movement sounds
	int movement_sounds_played = 0;
	bool shouldPlaySoundNow();
	int getWalkSoundIndex();
	AgentType::MovementState current_movement_state = AgentType::MovementState::None;
	void setMovementState(AgentType::MovementState state);
	// Time, in game ticks, until unit can turn by 1/8th of a circle
	int turning_animation_ticks_remaining = 0;

	std::list<up<BattleUnitMission>> missions;

	Vec3<float> position;
	Vec3<float> goalPosition;
	bool atGoal = false;
	bool usingLift = false;
	Vec2<int> facing;
	Vec2<int> goalFacing;
	// Value 0 - 100, Simulates slow takeoff, when starting to use flight this slowly rises to 1
	int flyingSpeedModifier = 0;

	// Successfully retreated from combat
	bool retreated = false;
	// Died and corpse was destroyed in an explosion
	bool destroyed = false;
	// Freefalling
	bool falling = false;
	float fallingSpeed = 0.0f;
	// Stun damage acquired
	int stunDamageInTicks = 0;
	int getStunDamage() const;
	// Returns true if the unit is dead
	bool isDead() const;
	// Returns true if the unit is unconscious and not dead
	bool isUnconscious() const;
	// Return true if the unit is conscious
	// and not currently in the dropped state (curent and target)
	bool isConscious() const;
	// Returns true if the unit is conscious and can fly
	bool canFly() const;
	// Returns true if the unit is conscious and can fly
	bool canMove() const;
	// So we can stop going ->isLarge() all the time
	bool isLarge() const { return agent->type->bodyType->large; }
	// Wether unit is static - not moving, not falling, not changing body state
	bool isStatic() const;
	// Wether unit is busy - with aiming or firing or otherwise involved
	bool isBusy() const;
	// Wether unit can go prone in current position and facing
	bool canGoProne() const;
	// Wether unit can kneel in current position and facing
	bool canKneel() const;
	int getCurrentHeight() const { return agent->type->bodyType->height.at(current_body_state); }

	// If unit is asked to give way, this list will be filled with facings
	// in order of priority that should be tried by it
	std::list<Vec2<int>> giveWayRequest;

	bool applyDamage(GameState &state, int damage, float armour);
	void handleCollision(GameState &state, Collision &c);
	// sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile);
	// void attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile, sp<TileObjectVehicle>
	// enemyTile);

	const Vec3<float> &getPosition() const { return this->position; }

	int getMaxHealth() const;
	int getHealth() const;

	int getMaxShield() const;
	int getShield() const;

	void setPosition(const Vec3<float> &pos);
	void resetGoal();

	virtual void update(GameState &state, unsigned int ticks);

	// Following members are not serialized, but rather are set in initBattle method

	sp<TileObjectBattleUnit> tileObject;
	sp<TileObjectShadow> shadowObject;
	wp<Battle> battle;

	void dropDown(GameState &state);
	void tryToRiseUp(GameState &state);
	void fallUnconscious(GameState &state);
	void die(GameState &state, bool violently);
	void destroy(GameState &state);

	/*
	- current order
	- curr. mind state (controlled/berserk/…)
	- ref. to psi attacker (who is controlling it/...)
	- unit side current
	- squad number"
	*/
};
}
