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

	enum class Behavior
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
		Prone,
		Walking,
		Running
	};
	enum class Stance
	{
		// Standart
		Standing,
		Hovering,
		Kneeling,
		Lying,

		// Special
		Strafing,
		Jumping,
		Downed,
	};
	enum class HandState
	{
		Empty,
		Carrying,
		Aiming,
		Firing
	};

	StateRef<Agent> agent;

	Behavior behavior;
	FireMode fire_mode;
	MovementMode movement_mode;
	bool kneel;

	Stance current_stance;
	Stance target_stance;
	HandState current_hand_state;

	// std::list<up<BattleUnitMission>> missions;

	Vec3<float> position;
	Vec2<int> facing;

	int health;
	int shield;
	int shieldRecharge;

	// sp<BattleTileObjectUnit> tileObject;
	// sp<BattleTileObjectShadow> shadowObject;

	// bool applyDamage(GameState &state, int damage, float armour);
	// void handleCollision(GameState &state, Collision &c);
	// sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile);
	// void attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile, sp<TileObjectVehicle>
	// enemyTile);

	// const Vec3<float> &getPosition() const { return this->position; }
	// const Vec3<float> &getDirection() const;

	//'Constitution' is the sum of health and shield
	// int getMaxConstitution() const;
	// int getConstitution() const;

	// int getMaxHealth() const;
	// int getHealth() const;

	// int getMaxShield() const;
	// int getShield() const;

	// void setPosition(const Vec3<float> &pos);

	// virtual void update(GameState &state, unsigned int ticks);

	/*
	"* Structure for a combat entity

- reference to a unit (who is this)
- current pos
- current state (standing/sitting/etc)
- current frame (i.e. jumping #3)
- current TUs
- current order
- curr. mind state (controlled/berserk/…)
- ref. to psi attacker (who is controlling it/...)
- unit side current
- squad number"

*/
};
}
