#pragma once
#include "game/state/agent.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/tileview/tile.h"
#include "library/sp.h"
#include "library/strings.h"
#include <algorithm>

namespace OpenApoc
{

class BattleStrategyIconList;
class TileObjectBattleUnit;
class TileObjectShadow;

class BattleUnit : public std::enable_shared_from_this<BattleUnit>
{
  public:
	
	static const int TICKS_PER_UNIT_TRAVELLED = 16;
	static const int TICKS_PER_FRAME = 4;

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

	// User set modes
	BehaviorMode behavior_mode = BehaviorMode::Normal;
	FireAimingMode fire_aiming_mode = FireAimingMode::Snap;
	FirePermissionMode fire_permission_mode = FirePermissionMode::AtWill;
	MovementMode movement_mode = MovementMode::Walking;
	KneelingMode kneeling_mode = KneelingMode::None;

	// Animation frames and state

	// Time, in game ticks, until body animation is finished
	int body_animation_ticks_remaining = 0;
	int getBodyAnimationFrame() { return std::max(0, (body_animation_ticks_remaining - 1) / TICKS_PER_FRAME); }
	AgentType::BodyState current_body_state = AgentType::BodyState::Standing;
	AgentType::BodyState target_body_state = AgentType::BodyState::Standing;
	// Time, in game ticks, until hands animation is finished
	int hand_animation_ticks_remaining = 0;
	int getHandAnimationFrame() { return std::max(0, (hand_animation_ticks_remaining - 1) / TICKS_PER_FRAME); }
	AgentType::HandState current_hand_state = AgentType::HandState::AtEase;
	AgentType::HandState target_hand_state = AgentType::HandState::AtEase;
	// Distance, in movement ticks, spent since starting to move
	int movement_ticks_passed = 0;
	int getDistanceTravelled() { return std::max(0, movement_ticks_passed / TICKS_PER_UNIT_TRAVELLED); }
	AgentType::MovementState movement_state = AgentType::MovementState::None;
	// Time, in game ticks, until unit can turn by 1/8th of a circle
	int turning_ticks_remaining = 0;

	// std::list<up<BattleUnitMission>> missions;

	Vec3<float> position;
	Vec2<int> facing;
	
	// Successfully retreated from combat
	bool retreated = false;
	// Died and corpse was destroyed in an explosion
	bool destroyed = false;
	// Freefalling
	bool falling = false;

	 sp<TileObjectBattleUnit> tileObject;
	 sp<TileObjectShadow> shadowObject;

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

	virtual void update(GameState &state, unsigned int ticks);

	// Following members are not serialized, but rather are set in initBattle method

	sp<BattleStrategyIconList> strategy_icon_list;


	/*
	- current order
	- curr. mind state (controlled/berserk/…)
	- ref. to psi attacker (who is controlling it/...)
	- unit side current
	- squad number"
	*/
};
}
