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

static const int AI_THINK_INTERVAL = TICKS_PER_SECOND / 24;

class GameState;
class AEquipment;
class BattleUnit;
class Organisation;
enum class MovementMode;
enum class KneelingMode;
enum class WeaponStatus;
enum class AEquipmentSlotType;

class AIAction
{
  public:
	enum class Type
	{
		AttackWeapon,
		AttackGrenade,
		AttackPsiPanic,
		AttackPsiStun,
		AttackPsiMC,
	};
	AIAction();

	// Parameters that are stored for future reference

	// Action type (an attack usually)
	Type type = Type::AttackWeapon;
	// Who to attack
	sp<BattleUnit> targetUnit;
	// What to fire / what to throw / what to use for PSI
	// For simply attacking with all weapons in hands, this can be null
	sp<AEquipment> item;
	// If attacking with weapons, which hands to use
	// (init in constructor since it's undefined here)
	WeaponStatus weaponStatus;

	// Methods

	UString getName();
};

class AIMovement
{
  public:
	enum class Type
	{
		// Stop moving
		Stop,
		// Turn to target location
		Turn,
		// Change stance
		ChangeStance,
		// Move to target location (patrol)
		Patrol,
		// Move to target location (advancing on enemy)
		Advance,
		// Move to target location (pursuing an enemy that left LOS)
		Pursue,
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
	Type type = Type::Stop;
	// Where to move / face
	Vec3<int> targetLocation = { 0,0,0 };
	// Preferred movement speed (not used for Stop or Turn)
	// (init in constructor since it's undefined here)
	MovementMode movementMode;
	// Preferred kneeling state (not used for Stop or Turn)
	// (init in constructor since it's undefined here)
	KneelingMode kneelingMode;

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
	// Returns decision that was made, and wether we should stop going forward on the AI chain
	virtual std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) { return{}; };

	virtual void notifyUnderFire(Vec3<int> position) {};
	virtual void notifyHit(Vec3<int> position) {};
	virtual void notifyEnemySpotted(Vec3<int> position) {};
};

class UnitAIHelper
{
  public:
	static sp<AIMovement> getRetreatMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getTakeCoverMovement(GameState &state, BattleUnit &u,
		bool forced = false);

	static sp<AIMovement> getKneelMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getPursueMovement(GameState &state, BattleUnit &u, Vec3<int> target, bool forced = false);

	static sp<AIMovement> getTurnMovement(GameState &state, BattleUnit &u, Vec3<int> target);

	static void ensureItemInSlot(GameState &state, sp<AEquipment> item, AEquipmentSlotType slot);
};

class UnitAIList
{
public:
	std::vector<sp<UnitAI>> aiList;
	std::map<UString, unsigned> aiMap;

	unsigned int ticksLastThink = 0;
	unsigned int ticksUntilReThink = 0;

	AIDecision think(GameState &state, BattleUnit &u);

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
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u)  override;
	
	// FIXME: Implement panic and berserk AI
};

// AI that handles unit's automatic actions (turning to attacker, visible enemy, firing held weapons)
class DefaultUnitAI : public UnitAI
{
  public:
	const UString getName() override { return "Default"; };
	
	int ticksAutoTurnAvailable = 0;
	int ticksAutoTargetAvailable = 0;

	// Relative position of a person who attacked us since last think()
	Vec3<int> attackerPosition = { 0, 0, 0 };

	void reset(GameState &state, BattleUnit &u) override;
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) override;

	void notifyUnderFire(Vec3<int> position) override;
	void notifyHit(Vec3<int> position) override;
};

// AI that handles unit's behavior (Aggressive, Normal, Cautious)
class BehaviorUnitAI : public UnitAI
{
  public:
	const UString getName() override { return "Behavior"; };
	
	void reset(GameState &state, BattleUnit &u) override;
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) override;

	void notifyUnderFire(Vec3<int> position)  override { };
	void notifyHit(Vec3<int> position)  override { };
	void notifyEnemySpotted(Vec3<int> position)  override { };
};

// AI that handles vanilla alien and civilian behavior
class VanillaUnitAI : public UnitAI
{
  public:
    const UString getName() override { return "VanillaUnit"; };
	
	unsigned int ticksLastThink = 0;
	// Value of 0 means we will not re-think based on timer
	unsigned int ticksUntilReThink = 0;

	// What AI decided to do at last think()
	AIDecision lastDecision;

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
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) override;

	void notifyUnderFire(Vec3<int> position) override;
	void notifyHit(Vec3<int> position) override;
	void notifyEnemySpotted(Vec3<int> position) override;

  private:
	AIDecision thinkInternal(GameState &state, BattleUnit &u);
	// Do the AI routine - organise inventory, move speed, stance etc.
	void routine(GameState &state, BattleUnit &u, const AIDecision &decision);

	// Calculate AI's next decision in case no enemy is engaged
	std::tuple<AIDecision, float, unsigned> thinkGreen(GameState &state, BattleUnit &u);
	// Calculate AI's next decision in case enemy is engaged (attacking)
	std::tuple<AIDecision, float, unsigned> thinkRed(GameState &state, BattleUnit &u);

	std::tuple<AIDecision, float, unsigned> getAttackDecision(GameState &state, BattleUnit &u);

	std::tuple<AIDecision, float, unsigned> getWeaponDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
		sp<BattleUnit> target);
	std::tuple<AIDecision, float, unsigned> getPsiDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
		sp<BattleUnit> target, AIAction::Type type);
	std::tuple<AIDecision, float, unsigned> getGrenadeDecision(GameState &state, BattleUnit &u, sp<AEquipment> e,
		sp<BattleUnit> target);

};

// AI that tries to be as hard towards the player as possible, TBD
class HardcoreUnitAI : public UnitAI
{
  public:
	const UString getName() override { return "Hardcore"; };

	void reset(GameState &state, BattleUnit &u) { active = true; };
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) override { return {}; };

	void notifyUnderFire(Vec3<int> position)  override { };
	void notifyHit(Vec3<int> position)  override { };
	void notifyEnemySpotted(Vec3<int> position)  override { };
};

// Represents an AI that controls all units on the tactical as a whole
class TacticalAI
{
  public:
	virtual const UString getName() { return ""; };

	virtual void reset(GameState &state, StateRef<Organisation> o) {};
	virtual std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> think(GameState &state, StateRef<Organisation> o) { return{}; };
};

// Represents tactical AI for every AI-controlled org in battle
class TacticalAIBlock
{
  public:
	void init(GameState &state);
	void reset(GameState &state);
	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> think(GameState &state);

	std::map<StateRef<Organisation>, sp<TacticalAI>> aiList;

	unsigned int ticksLastThink = 0;
	unsigned int ticksUntilReThink = 0;
};

// Vanilla AI that tries to replicate how aliens/civilians/security moved around the map
class VanillaTacticalAI : public TacticalAI
{
  public:
	const UString getName() override { return "VanillaTactical"; };

	void reset(GameState &state, StateRef<Organisation> o);
	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> think(GameState &state, StateRef<Organisation> o) override;

	unsigned int ticksLastThink = 0;
	unsigned int ticksUntilReThink = 0;

 	std::tuple<std::list<StateRef<BattleUnit>>, sp<AIMovement>> getPatrolMovement(GameState &state, BattleUnit &u);
};

}