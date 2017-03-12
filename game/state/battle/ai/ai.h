// FIXME: Separate this file into multiple files properly
#pragma once

#include "float.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>

// --- Read note about serialization at the end of the file ---

namespace OpenApoc
{

static const uint64_t AI_THINK_INTERVAL = TICKS_PER_SECOND / 24;
static const uint64_t LOWMORALE_AI_INTERVAL = TICKS_PER_SECOND * 2;

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
		AttackWeaponTile,
		AttackWeaponUnit,
		AttackGrenade,
		AttackPsiPanic,
		AttackPsiStun,
		AttackPsiMC,
	};
	AIAction();

	// Parameters that are stored for future reference

	// Action type (an attack usually)
	Type type = Type::AttackWeaponTile;
	// Who to attack
	StateRef<BattleUnit> targetUnit;
	// What location to attack
	Vec3<int> targetLocation = {0, 0, 0};
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
	Vec3<int> targetLocation = {0, 0, 0};
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
	None,        // Crysalises
	Civilian,    // Civilians
	Loner,       // Poppers, Large units
	Group,       // Majority of the units
	PanicFreeze, // During Panic (the one that makes you freeze in place)
	PanicRun,    // During Panic (the one that makes you drop weapon and run)
	Berserk,     // During Berserk
};

// Represents a logic that unit uses to decide it's automatic actions
class UnitAI
{
  public:
	enum class Type
	{
		LowMorale,
		Default,
		Behavior,
		Vanilla,
		Hardcore
	};
	Type type; // cannot hide because serializer won't work
	virtual const UString getName();

	// Wether AI is currently active
	bool active = false;

	virtual void reset(GameState &, BattleUnit &){};
	// Returns decision that was made, and wether we should stop going forward on the AI chain
	virtual std::tuple<AIDecision, bool> think(GameState &, BattleUnit &) { return {}; };

	virtual void notifyUnderFire(Vec3<int>){};
	virtual void notifyHit(Vec3<int>){};
	virtual void notifyEnemySpotted(Vec3<int>){};
};

class UnitAIHelper
{
  public:
	static sp<AIMovement> getRetreatMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getTakeCoverMovement(GameState &state, BattleUnit &u,
	                                           bool forced = false);

	static sp<AIMovement> getKneelMovement(GameState &state, BattleUnit &u, bool forced = false);

	static sp<AIMovement> getPursueMovement(GameState &state, BattleUnit &u, Vec3<int> target,
	                                        bool forced = false);

	static sp<AIMovement> getTurnMovement(GameState &state, BattleUnit &u, Vec3<int> target);

	static void ensureItemInSlot(GameState &state, sp<AEquipment> item, AEquipmentSlotType slot);
};

class UnitAIList
{
  public:
	std::vector<sp<UnitAI>> aiList;
	std::map<UString, unsigned> aiMap;

	uint64_t ticksLastThink = 0;
	uint64_t ticksUntilReThink = 0;

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
	LowMoraleUnitAI();

	uint64_t ticksActionAvailable = 0;

	void reset(GameState &state, BattleUnit &u) override;
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) override;
};

// AI that handles unit's automatic actions (turning to attacker, visible enemy, firing held
// weapons)
class DefaultUnitAI : public UnitAI
{
  public:
	DefaultUnitAI();

	uint64_t ticksAutoTurnAvailable = 0;
	uint64_t ticksAutoTargetAvailable = 0;

	// Relative position of a person who attacked us since last think()
	Vec3<int> attackerPosition = {0, 0, 0};

	void reset(GameState &state, BattleUnit &u) override;
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) override;

	void notifyUnderFire(Vec3<int> position) override;
	void notifyHit(Vec3<int> position) override;
};

// AI that handles unit's behavior (Aggressive, Normal, Cautious)
class BehaviorUnitAI : public UnitAI
{
  public:
	BehaviorUnitAI();

	void reset(GameState &state, BattleUnit &u) override;
	std::tuple<AIDecision, bool> think(GameState &state, BattleUnit &u) override;

	void notifyUnderFire(Vec3<int> position) override{};
	void notifyHit(Vec3<int> position) override{};
	void notifyEnemySpotted(Vec3<int> position) override{};
};

// AI that handles vanilla alien and civilian behavior
class VanillaUnitAI : public UnitAI
{
  public:
	VanillaUnitAI();

	uint64_t ticksLastThink = 0;
	// Value of 0 means we will not re-think based on timer
	uint64_t ticksUntilReThink = 0;

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
	void routine(GameState &state, BattleUnit &u);

	// Calculate AI's next decision in case no enemy is engaged
	std::tuple<AIDecision, float, unsigned> thinkGreen(GameState &state, BattleUnit &u);
	// Calculate AI's next decision in case enemy is engaged (attacking)
	std::tuple<AIDecision, float, unsigned> thinkRed(GameState &state, BattleUnit &u);

	std::tuple<AIDecision, float, unsigned> getAttackDecision(GameState &state, BattleUnit &u);

	std::tuple<AIDecision, float, unsigned> getWeaponDecision(GameState &state, BattleUnit &u,
	                                                          sp<AEquipment> e,
	                                                          StateRef<BattleUnit> target);
	std::tuple<AIDecision, float, unsigned> getPsiDecision(GameState &state, BattleUnit &u,
	                                                       sp<AEquipment> e,
	                                                       StateRef<BattleUnit> target,
	                                                       AIAction::Type type);
	std::tuple<AIDecision, float, unsigned> getGrenadeDecision(GameState &state, BattleUnit &u,
	                                                           sp<AEquipment> e,
	                                                           StateRef<BattleUnit> target);
};

// AI that tries to be as hard towards the player as possible, TBD
class HardcoreUnitAI : public UnitAI
{
  public:
	HardcoreUnitAI();
};

// Represents an AI that controls all units on the tactical as a whole
class TacticalAI
{
  public:
	enum class Type
	{
		Vanilla
	};
	Type type; // cannot hide because serializer won't work
	const UString getName();

	virtual void reset(GameState &, StateRef<Organisation>){};
	virtual std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
	think(GameState &, StateRef<Organisation>)
	{
		return {};
	};
};

// Represents tactical AI for every AI-controlled org in battle
class TacticalAIBlock
{
  public:
	void init(GameState &state);
	void reset(GameState &state);
	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>> think(GameState &state);

	std::map<StateRef<Organisation>, sp<TacticalAI>> aiList;

	uint64_t ticksLastThink = 0;
	uint64_t ticksUntilReThink = 0;
};

// Vanilla AI that tries to replicate how aliens/civilians/security moved around the map
class VanillaTacticalAI : public TacticalAI
{
  public:
	VanillaTacticalAI();

	void reset(GameState &state, StateRef<Organisation> o) override;
	std::list<std::pair<std::list<StateRef<BattleUnit>>, AIDecision>>
	think(GameState &state, StateRef<Organisation> o) override;

	uint64_t ticksLastThink = 0;
	uint64_t ticksUntilReThink = 0;

	std::tuple<std::list<StateRef<BattleUnit>>, sp<AIMovement>> getPatrolMovement(GameState &state,
	                                                                              BattleUnit &u);
};
}

/*
// Alexey Andronov (Istrebitel):

Due to the way OpenAPoc serializes things, we do not know what class is stored in the file.
So when we have a pointer to derived class stored in a pointer to base class, and we serialized it,
we have no way of knowing which class is stored there.
Therefore, every AI has to have a UString attached to it that is it's class name, and method in
gamestate_serialize needs to be updated for any new AI introduced.

*/