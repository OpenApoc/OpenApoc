#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/gametime.h"
#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{

// class BattleTileObjectUnit;
// class BattleTileObjectShadow;

class BattleUnit : public StateObject<BattleUnit>, public std::enable_shared_from_this<BattleUnit>
{
  public:
	static const unsigned SHIELD_RECHARGE_TIME = TICKS_PER_SECOND * 100;

	// Enums for player selectable modes for the agent
	enum class BehaviorMode
	{
		Aggressive,
		Normal,
		Evasive
	};
	enum class FireMode
	{
		CeaseFire,
		Aimed,
		Snap,
		Auto
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

	// User set modes

	BehaviorMode behavior_mode;
	FireMode fire_mode;
	MovementMode movement_mode;
	KneelingMode kneeling_mode;

	// Animation frames and state

	// Time, in game ticks, until body animation is finished
	int body_animation_ticks_remaining = 0;
	AgentType::BodyState current_body_state = AgentType::BodyState::Standing;
	AgentType::BodyState target_body_state = AgentType::BodyState::Standing;
	// Time, in game ticks, until hands animation is finished
	int hand_animation_ticks_remaining = 0;
	AgentType::HandState current_hand_state = AgentType::HandState::AtEase;
	AgentType::HandState target_hand_state = AgentType::HandState::AtEase;
	// Distance, in movement ticks, spent since starting to move
	int movement_ticks_passed = 0;
	AgentType::MovementState movement_state = AgentType::MovementState::None;
	// Time, in game ticks, until unit can turn by 1/8th of a circle
	int turning_animation_ticks_remaining = 0;

	// std::list<up<BattleUnitMission>> missions;

	Vec3<float> position;
	Vec2<int> facing;

	bool retreated = false;
	bool destroyed = false;

	//int shield;
	//int shieldRecharge;

	// sp<BattleTileObjectUnit> tileObject;
	// sp<BattleTileObjectShadow> shadowObject;

	// bool applyDamage(GameState &state, int damage, float armour);
	// void handleCollision(GameState &state, Collision &c);
	// sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile);
	// void attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile, sp<TileObjectVehicle>
	// enemyTile);

	// const Vec3<float> &getPosition() const { return this->position; }
	// const Vec3<float> &getDirection() const;

	// int getMaxHealth() const;
	// int getHealth() const;

	// int getMaxShield() const;
	// int getShield() const;

	// void setPosition(const Vec3<float> &pos);

	// virtual void update(GameState &state, unsigned int ticks);

	/*
	- current pos
	- current order
	- curr. mind state (controlled/berserk/…)
	- ref. to psi attacker (who is controlling it/...)
	- unit side current
	- squad number"
	*/
};
}
