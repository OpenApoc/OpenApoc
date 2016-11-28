#pragma once

#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>

namespace OpenApoc
{

class GameState;
class AEquipment;
class BattleUnit;
enum class MovementMode;
enum class KneelingMode;

class AIAction
{
  public:
	enum class Type
	{
		None,
		AttackWeapon,
		AttackGrenade,
		AttackPsiPanic,
		AttackPsiStun,
		AttackPsiMC,
	};

	// Parameters that are stored for future reference

	// Action type (an attack usually)
	Type type = Type::None;
	// Who to attack
	sp<BattleUnit> targetUnit;
	// What to fire / what to throw / what to use for PSI
	sp<AEquipment> item;

	// Parameters that are only used immediately when decision is done

	// Ticks until action must be re-thought
	int ticksUntilThinkAgain = -1;

	// Priority if comparing to other options
	float priority = FLT_MIN;

	// Methods

	UString getName();
};

class AIMovement
{
  public:
	enum class Type
	{
		None,
		// Turn to target location
		Turn,
		// Move to target location (patrol)
		Patrol,
		// Move to target location (advancing on enemy)
		Advance,
		// Move to target location (getting in range of fire/throw)
		GetInRange,
		// Move to target location (taking cover)
		TakeCover,
		// Move to target location (retreating)
		Retreat
	};
	AIMovement();
	AIMovement(const AIMovement &obj) = default;

	// Parameters that are stored for future reference

	// Movement type
	Type type = Type::None;
	// Where to move / face
	Vec3<int> targetLocation;
	// Preferred movement speed
	MovementMode movementMode;
	// Preferred kneeling state
	KneelingMode kneelingMode;

	// Parameters that are only used immediately when decision is done

	// Ticks until movement must be re-thought
	int ticksUntilThinkAgain = -1;

	// Priority if comparing to other options
	float priority = FLT_MIN;

	// Other units participating (if group moving)
	std::list<StateRef<BattleUnit>> units;

	// Methods

	UString getName();
};

class AIDecision
{
  public:
	AIDecision() = default;
	AIDecision(sp<AIAction> action, sp<AIMovement> movement);

	// Parameters that are stored for future reference

	// Action to be taken (nullptr = carry on with previous action)
	sp<AIAction> action;

	// Movement to be done (nullptr = carry on with previous movement)
	sp<AIMovement> movement;

	// Time to wait until re-thinking this decision can happen
	//   (-1) = no timer, never re-think based on timer, only on other happenings.
	// Following rules also apply.
	// Decision is never re-thought if:
	// - actionType == grenade && currently throwing
	// Decision is always re-thought if:
	// - we see enemies now and we didn't on last think()
	// - someone attacked us since last think()
	// - movementType != none && not currently moving
	// - actionType == grenade && not currently throwing
	// - actionType == psi && not currently attacking with psi
	// - actionType == weapon && not currently attacking with weapon
	int ticksUntilThinkAgain = -1;

	// Parameters that are only used immediately when decision is done

	float priority = FLT_MIN;

	// Methods

	UString getName();
};

class AIState
{
  public:
	// What AI decided to do at last think()
	AIDecision lastDecision;
	// Was enemy ever visible since last think()
	bool enemySpotted = false;
	// Previous value, to identify when enemy was first spotted
	bool enemySpottedPrevious = false;
	// Relative position of a person who attacked us since last think()
	Vec3<int> attackerPosition = {0, 0, 0};
	// Relative position of last seen enemy's last seen position since last think()
	Vec3<int> lastSeenEnemyPosition = {0, 0, 0};

	void reset(GameState &state, bool duringBattle = true);
};

enum class AIType
{
	None,     // Crysalises
	Civilian, //
	Loner,    // Poppers, Large units
	Group,    // Majority of the units
	Panic,    //
	Berserk,  //
};

class AI
{
  public:
	// Calculate AI's next decision, then do the routine
	// If unit has group AI, and patrol decision is made, the group will move together
	static AIDecision think(GameState &state, BattleUnit &u);

	// Do the AI routine - organise inventory, move speed, stance etc.
	static void routine(GameState &state, BattleUnit &u, const AIDecision &decision);

	// Calculate AI's next decision in case no enemy is engaged
	static AIDecision thinkGreen(GameState &state, BattleUnit &u);
	// Calculate AI's next decision in case enemy is engaged (attacking)
	static AIDecision thinkRed(GameState &state, BattleUnit &u);

	static AIDecision getAttackDecision(GameState &state, BattleUnit &u);

	static AIDecision getWeaponDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
	                                    sp<BattleUnit> target);
	static AIDecision getPsiDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
	                                 sp<BattleUnit> target, AIAction::Type type);
	static AIDecision getGrenadeDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
	                                     sp<BattleUnit> target);

	static sp<AIMovement> getPatrolMovement(GameState &state, BattleUnit &u);

	static sp<AIMovement> getRetreatMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getTakeCoverMovement(GameState &state, BattleUnit &u,
	                                           bool forced = false);

	static sp<AIMovement> getKneelMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getTurnMovement(GameState &state, BattleUnit &u, Vec3<int> target);

	static sp<AIMovement> getPursueMovement(GameState &state, BattleUnit &u, Vec3<int> target);
};
}