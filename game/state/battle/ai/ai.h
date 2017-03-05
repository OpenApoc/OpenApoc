// FIXME: Separate this file into multiple files properly
#pragma once

#include "game/state/stateobject.h"
#include "game/state/gametime.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include "float.h"
#include <list>

namespace OpenApoc
{

static const int AI_THINK_INTERVAL = TICKS_PER_SECOND / 16;

class GameState;
class AEquipment;
class BattleUnit;
class Organisation;
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
	bool isEmpty();

	// Parameters that are stored for future reference

	UString ai; // AI that made the decision

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

	// Halt AI processing, preventing AI further down the list from thinking
	bool halt = false;

	// Parameters that are only used immediately when decision is done

	float priority = FLT_MIN;

	// Methods

	UString getName();
};

// Unit's AI type (AI may check this and act differently based on values here)
enum class AIType
{
	None,			// Crysalises
	Civilian,		// Civilians
	Loner,			// Poppers, Large units
	Group,			// Majority of the units
	PanicFreeze,	// During Panic (the one that makes you freeze in place)
	PanicRun,		// During Panic (the one that makes you drop weapon and run)
	Berserk,		// During Berserk
};


// Represents a logic that unit uses to decide it's automatic actions
class UnitAI
{
  public:
	virtual const UString getName() { return ""; };
	  
	// Wether AI is currently active
	bool active = false;

	virtual void reset(GameState &state, BattleUnit &u) {};
	virtual AIDecision think(GameState &state, BattleUnit &u) { return{}; };

	virtual void notifyUnderFire(Vec3<int> position) {};
	virtual void notifyHit(Vec3<int> position) {};
	virtual void notifyEnemySpotted(Vec3<int> position) {};
};

class UnitAIList
{
public:
	// What AI decided to do at last think()
	AIDecision lastDecision;

	std::vector<sp<UnitAI>> aiList;
	std::map<UString, int> aiMap;

	unsigned int ticksLastThink = 0;
	unsigned int ticksUntilThinkAgain = 0;

	AIDecision think(GameState &state, BattleUnit &u);

	void registerNewDecision(AIDecision decision);

	void init(GameState &state, BattleUnit &u);
	void reset(GameState &state, BattleUnit &u);

	void notifyUnderFire(Vec3<int> position);
	void notifyHit(Vec3<int> position);
	void notifyEnemySpotted(Vec3<int> position);
};

// AI that handles Panic and Berserk
class LowMoraleUnitAI : public UnitAI
{
  public:
	const UString getName() override { return "LowMorale"; };

	void reset(GameState &state, BattleUnit &u) override;
	AIDecision think(GameState &state, BattleUnit &u)  override;
	
	// FIXME: Implement panic and berserk AI
};

// AI that handles unit's automatic actions (turning to attacker, visible enemy, firing held weapons)
class DefaultUnitAI : public UnitAI
{
  public:
	const UString getName() override { return "Default"; };

	void reset(GameState &state, BattleUnit &u) override;
	AIDecision think(GameState &state, BattleUnit &u) override;
};

// AI that handles unit's behavior (Aggressive, Normal, Cautious)
class BehaviorUnitAI : public UnitAI
{
  public:
	const UString getName() override { return "Behavior"; };
	
	void reset(GameState &state, BattleUnit &u) override;
	AIDecision think(GameState &state, BattleUnit &u) override;

	void notifyUnderFire(Vec3<int> position)  override { };
	void notifyHit(Vec3<int> position)  override { };
	void notifyEnemySpotted(Vec3<int> position)  override { };
};

// AI that handles vanilla alien and civilian behavior
class VanillaUnitAI : public UnitAI
{
  public:
    const UString getName() override { return "Vanilla"; };
	unsigned int ticksLastThink = 0;
	int ticksUntilThinkAgain = -1;

	// Was enemy ever visible since last think()
    bool enemySpotted = false;
    // Previous value, to identify when enemy was first spotted
    bool enemySpottedPrevious = false;
    // Relative position of a person who attacked us since last think()
    Vec3<int> attackerPosition = { 0, 0, 0 };
    // Relative position of last seen enemy's last seen position since last think()
    Vec3<int> lastSeenEnemyPosition = { 0, 0, 0 };

	void reset(GameState &state, BattleUnit &u) override;
	
	// Calculate AI's next decision, then do the routine
	// If unit has group AI, and patrol decision is made, the group will move together
	AIDecision think(GameState &state, BattleUnit &u) override;

	void notifyUnderFire(Vec3<int> position) override;
	void notifyHit(Vec3<int> position) override;
	void notifyEnemySpotted(Vec3<int> position) override;

  private:
	AIDecision thinkInternal(GameState &state, BattleUnit &u);
	// Do the AI routine - organise inventory, move speed, stance etc.
	void routine(GameState &state, BattleUnit &u, const AIDecision &decision);

	// Calculate AI's next decision in case no enemy is engaged
	AIDecision thinkGreen(GameState &state, BattleUnit &u);
	// Calculate AI's next decision in case enemy is engaged (attacking)
	AIDecision thinkRed(GameState &state, BattleUnit &u);

	AIDecision getAttackDecision(GameState &state, BattleUnit &u);

	AIDecision getWeaponDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
		sp<BattleUnit> target);
	AIDecision getPsiDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
		sp<BattleUnit> target, AIAction::Type type);
	AIDecision getGrenadeDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
		sp<BattleUnit> target);

	sp<AIMovement> getPatrolMovement(GameState &state, BattleUnit &u);

	sp<AIMovement> getRetreatMovement(GameState &state, BattleUnit &u, bool forced = false);

	sp<AIMovement> getTakeCoverMovement(GameState &state, BattleUnit &u,
		bool forced = false);

	sp<AIMovement> getKneelMovement(GameState &state, BattleUnit &u, bool forced = false);

	sp<AIMovement> getTurnMovement(GameState &state, BattleUnit &u, Vec3<int> target);

	sp<AIMovement> getPursueMovement(GameState &state, BattleUnit &u, Vec3<int> target);
};

// AI that tries to be as hard towards the player as possible, TBD
class HardcoreUnitAI : public UnitAI
{
  public:
	const UString getName() override { return "Hardcore"; };

	void reset(GameState &state, BattleUnit &u) { active = true; };
	AIDecision think(GameState &state, BattleUnit &u) override { return {}; };

	void notifyUnderFire(Vec3<int> position)  override { };
	void notifyHit(Vec3<int> position)  override { };
	void notifyEnemySpotted(Vec3<int> position)  override { };
};

// Represents an AI that controls all units on the tactical as a whole
class TacticalAI
{
  public:
	virtual const UString getName() { return ""; };

	virtual void reset(GameState &state, const Organisation &o) {};
	virtual AIDecision think(GameState &state, const Organisation &o) { return{}; };
};

// Represents tactical AI for every AI-controlled org in battle
class TacticalAIBlock
{
  public:
	void init(GameState &state);
	void reset(GameState &state);
	AIDecision think(GameState &state);

	std::map<StateRef<Organisation>, sp<TacticalAI>> aiList;

	unsigned int ticksLastThink = 0;
	unsigned int ticksUntilThinkAgain = 0;
};

// Vanilla AI that tries to replicate how aliens/civilians/security moved around the map
class VanillaTacticalAI : public TacticalAI
{
  public:
	const UString getName() override { return "Vanilla"; };

	void reset(GameState &state, const Organisation &o);
	AIDecision think(GameState &state, const Organisation &o) override;

	unsigned int ticksLastThink = 0;
	unsigned int ticksUntilThinkAgain = 0;
};

}