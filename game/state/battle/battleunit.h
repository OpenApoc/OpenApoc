#pragma once
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/agent.h"
#include "game/state/battle/ai/ai.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/gametime.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <vector>

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
// A bit faster than items
#define FALLING_ACCELERATION_UNIT 0.16666667f // 1/6th
// How far should unit spread information about seeing an enemy
#define DISTANCE_TO_RELAY_VISIBLE_ENEMY_INFORMATION 5
// How far does unit see
#define VIEW_DISTANCE 20

namespace OpenApoc
{

// FIXME: Seems to correspond to vanilla behavior, but ensure it's right
static const unsigned TICKS_PER_WOUND_EFFECT = TICKS_PER_TURN;
static const unsigned TICKS_PER_ENZYME_EFFECT = TICKS_PER_SECOND / 9; 
static const unsigned TICKS_PER_FIRE_EFFECT = TICKS_PER_SECOND;
// FIXME: Ensure correct
static const unsigned TICKS_PER_LOWMORALE_STATE = TICKS_PER_TURN;
// FIXME: Ensure correct
static const unsigned LOWMORALE_CHECK_INTERVAL = TICKS_PER_TURN;
// How frequently unit tracks its target
static const unsigned LOS_CHECK_INTERVAL_TRACKING = TICKS_PER_SECOND / 4;

class TileObjectBattleUnit;
class TileObjectShadow;
class Battle;
class DamageType;
class AIDecision;
class AIAction;
class AIMovement;
enum class DamageSource;

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

enum class WeaponAimingMode
{
	Aimed = 1,
	Snap = 2,
	Auto = 4
};

// Unit's general type, used in pathfinding
enum class BattleUnitType
{
	SmallWalker,
	SmallFlyer,
	LargeWalker,
	LargeFlyer
};

// Enum for tracking unit's weapon state
enum class WeaponStatus
{
	NotFiring,
	FiringLeftHand,
	FiringRightHand,
	FiringBothHands
};

enum class MoraleState
{
	Normal,
	PanicFreeze,
	PanicRun,
	Berserk
};

static const std::list<BattleUnitType> BattleUnitTypeList = {
    BattleUnitType::LargeFlyer, BattleUnitType::LargeWalker, BattleUnitType::SmallFlyer,
    BattleUnitType::SmallWalker};

class BattleUnit : public StateObject, public std::enable_shared_from_this<BattleUnit>
{
	STATE_OBJECT(BattleUnit)
  public:
	// [Enums]

	enum class BehaviorMode
	{
		Aggressive,
		Normal,
		Evasive
	};
	enum class FirePermissionMode
	{
		AtWill,
		CeaseFire
	};
	// Enum for tracking unit's targeting mode
	enum class TargetingMode
	{
		NoTarget,
		Unit,
		TileCenter,
		TileGround
	};

	// [Properties]

	UString id;

	// Unit ownership

	StateRef<Agent> agent;
	StateRef<Organisation> owner;

	// Squad

	// Squad number, -1 = not assigned to any squad
	int squadNumber = 0;
	// Squad position, has no meaning if not in squad
	unsigned int squadPosition = 0;

	// Attacking and turning to hostiles

	WeaponStatus weaponStatus = WeaponStatus::NotFiring;
	// What are we targeting - unit or tile
	TargetingMode targetingMode = TargetingMode::NoTarget;
	// Tile we're targeting right now (or tile with unit we're targeting, if targeting unit)
	Vec3<int> targetTile;
	// Unit we're targeting right now
	StateRef<BattleUnit> targetUnit;
	// Unit we're ordered to focus on (in real time)
	StateRef<BattleUnit> focusUnit;
	// Units focusing this unit
	std::list<StateRef<BattleUnit>> focusedByUnits;
	// Ticks until we check if target is still valid, turn to it etc.
	unsigned ticksUntillNextTargetCheck = 0;

	// Stats

	// Accumulated xp points for each stat
	AgentStats experiencePoints;
	// Fatal wounds for each body part
	std::map<BodyPart, int> fatalWounds;
	// Which body part is medikit used on
	BodyPart healingBodyPart = BodyPart::Body;
	// Is using a medikit
	bool isHealing = false;
	// Ticks towards next wound damage or medikit heal application
	unsigned woundTicksAccumulated = 0;
	// Stun damage acquired
	int stunDamageInTicks = 0;
	// Ticks accumulated towards next enzyme hit
	int enzymeDebuffTicksAccumulated = 0;
	// Enzyme debuff intensity remaining
	int enzymeDebuffIntensity = 0;
	// Ticks accumulated towards next fire hit
	int fireDebuffTicksAccumulated = 0;
	// Fire debuff intensity remaining
	int fireDebuffTicksRemaining = 0;
	// State of unit's morale
	MoraleState moraleState = MoraleState::Normal;
	// How much time unit has to spend in low morale state
	int moraleStateTicksRemaining = 0;
	// Ticks accumulated towards next morale check
	int moraleTicksAccumulated = 0;

	// User set modes

	BehaviorMode behavior_mode = BehaviorMode::Normal;
	WeaponAimingMode fire_aiming_mode = WeaponAimingMode::Snap;
	FirePermissionMode fire_permission_mode = FirePermissionMode::AtWill;
	MovementMode movement_mode = MovementMode::Walking;
	KneelingMode kneeling_mode = KneelingMode::None;

	// Body

	// Time, in game ticks, until body animation is finished
	unsigned int body_animation_ticks_remaining = 0;
	BodyState current_body_state = BodyState::Standing;
	BodyState target_body_state = BodyState::Standing;

	// Hands

	// Time, in game ticks, until hands animation is finished
	unsigned int hand_animation_ticks_remaining = 0;
	// Time, in game ticks, until unit will lower it's weapon
	unsigned int aiming_ticks_remaining = 0;
	// Time, in game ticks, until firing animation is finished
	unsigned int firing_animation_ticks_remaining = 0;
	HandState current_hand_state = HandState::AtEase;
	HandState target_hand_state = HandState::AtEase;

	// Movement

	// Distance, in movement ticks, spent since starting to move
	unsigned int movement_ticks_passed = 0;
	// Movement sounds played
	unsigned int movement_sounds_played = 0;
	MovementState current_movement_state = MovementState::None;
	Vec3<float> position;
	Vec3<float> goalPosition;
	bool atGoal = false;
	bool usingLift = false;
	// Value 0 - 100, Simulates slow takeoff, when starting to use flight this slowly rises to 1
	unsigned int flyingSpeedModifier = 0;
	// Freefalling
	bool falling = false;
	// Current falling speed
	float fallingSpeed = 0.0f;

	// Turning

	// Time, in game ticks, until unit can turn by 1/8th of a circle
	unsigned int turning_animation_ticks_remaining = 0;
	Vec2<int> facing;
	Vec2<int> goalFacing;

	// Missions

	// Mission list
	std::list<up<BattleUnitMission>> missions;

	// Vision

	StateRef<AEquipmentType> displayedItem;
	std::set<StateRef<BattleUnit>> visibleUnits;
	std::list<StateRef<BattleUnit>> visibleEnemies;

	// Miscellaneous

	// Successfully retreated from combat
	bool retreated = false;

	// Died and corpse was destroyed in an explosion
	bool destroyed = false;

	// If unit is asked to give way, this list will be filled with facings
	// in order of priority that should be tried by it
	std::list<Vec2<int>> giveWayRequestData;

	// AI list
	UnitAIList aiList;

	// [Methods]

	void init(GameState &state);

	// Squad

	void removeFromSquad(Battle &b);
	bool assignToSquad(Battle &b, int squadNumber);
	void moveToSquadPosition(Battle &b, int squadPosition);

	// Stats

	bool isFatallyWounded();
	void addFatalWound(BodyPart fatalWoundPart);
	int getStunDamage() const;
	void dealDamage(GameState &state, int damage, bool generateFatalWounds, BodyPart fatalWoundPart,
	                int stunPower);

	// Attacking and turning to hostiles

	void setFocus(GameState &state, StateRef<BattleUnit> unit);
	void startAttacking(GameState &state, StateRef<BattleUnit> unit,
	                    WeaponStatus status = WeaponStatus::FiringBothHands);
	void startAttacking(GameState &state, Vec3<int> tile,
	                    WeaponStatus status = WeaponStatus::FiringBothHands, bool atGround = false);
	void stopAttacking();
	// Returns which hands can be used for an attack (or none if attack cannot be made)
	// Checks wether target unit is in range, and clear LOF exists to it
	// Clear LOF means no friendly fire and no map part in between
	WeaponStatus canAttackUnit(GameState &state, sp<BattleUnit> unit);
	// Returns wether unit can be attacked by one of the two supplied weapons
	// Checks wether target unit is in range, and clear LOF exists to it
	// Clear LOF means no friendly fire and no map part in between
	WeaponStatus canAttackUnit(GameState &state, sp<BattleUnit> unit, sp<AEquipment> rightHand,
	                           sp<AEquipment> leftHand = nullptr);

	// Body

	unsigned int getBodyAnimationFrame() const;
	void setBodyState(GameState &state, BodyState bodyState);
	void beginBodyStateChange(GameState &state, BodyState bodyState);

	// Hands

	unsigned int getHandAnimationFrame() const;
	void setHandState(HandState state);
	void beginHandStateChange(HandState state);

	// Movement

	void setMovementState(MovementState state);
	unsigned int getDistanceTravelled() const;
	bool shouldPlaySoundNow();
	unsigned int getWalkSoundIndex();
	void startFalling();
	void startMoving();

	// Turning

	void beginTurning(GameState &state, Vec2<int> newFacing);
	void setFacing(GameState &state, Vec2<int> newFacing);

	// Movement and turning

	void setPosition(GameState &state, const Vec3<float> &pos);
	void resetGoal();

	// Missions

	// Pops all finished missions, returns true if unit retreated and you should return
	bool popFinishedMissions(GameState &state);
	// Wether unit has a mission queued that will make it move
	bool hasMovementQueued();
	bool getNextDestination(GameState &state, Vec3<float> &dest);
	bool getNextFacing(GameState &state, Vec2<int> &dest);
	bool getNextBodyState(GameState &state, BodyState &dest);
	// Add mission, if possible to the front, otherwise to the back
	// (or, if toBack=true, then always to the back)
	bool addMission(GameState &state, BattleUnitMission *mission, bool toBack = false);
	bool addMission(GameState &state, BattleUnitMission::Type type);
	// Attempt to cancel all unit missions, returns true if successful
	bool cancelMissions(GameState &state, bool forced = false);
	// Attempt to give unit a new mission, replacing others, returns true if successful
	bool setMission(GameState &state, BattleUnitMission *mission);

	// TU functions

	// Wether unit can afford action
	bool canAfford(GameState &state, int cost) const;
	// Returns if unit did spend (false if unsufficient TUs)
	bool spendTU(GameState &state, int cost);

	// AI execution

	static void executeGroupAIDecision(GameState &state, AIDecision &decision,
	                                   std::list<StateRef<BattleUnit>> &units);
	void executeAIDecision(GameState &state, AIDecision &decision);
	void executeAIAction(GameState &state, AIAction &action);
	void executeAIMovement(GameState &state, AIMovement &movement);

	// Notifications
	void notifyUnderFire(Vec3<int> position);
	void notifyHit(Vec3<int> position);

	// Misc

	void requestGiveWay(const BattleUnit &requestor, const std::list<Vec3<int>> &plannedPath,
	                    Vec3<int> pos);

	// Unit state queries

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
	// Wether unit is moving (has Goto Location queued)
	bool isMoving() const;
	// Wether unit is doing a specific mission (has it queued)
	bool isDoing(BattleUnitMission::Type missionType) const;
	// Return unit's general type
	BattleUnitType getType() const;
	// Wether unit is AI controlled
	bool isAIControlled(GameState &state) const;
	// Unit's current AI type (can be modified by panic etc.)
	AIType getAIType() const;

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
	// Get unit's gun muzzle location (where shots come from)
	Vec3<float> getMuzzleLocation() const;
	// Get thrown item's departure location
	Vec3<float> getThrownItemLocation() const;

	// Determine body part hit
	BodyPart determineBodyPartHit(StateRef<DamageType> damageType, Vec3<float> cposition,
	                              Vec3<float> direction);
	// Returns true if sound and doodad were handled by it
	bool applyDamage(GameState &state, int power, StateRef<DamageType> damageType,
	                 BodyPart bodyPart, DamageSource source);
	// Returns true if sound and doodad were handled by it
	bool handleCollision(GameState &state, Collision &c);

	const Vec3<float> &getPosition() const { return this->position; }

	int getMaxHealth() const;
	int getHealth() const;

	int getMaxShield() const;
	int getShield() const;

	// Update

	void update(GameState &state, unsigned int ticks);
	void updateStateAndStats(GameState &state, unsigned int ticks);
	void updateEvents(GameState &state);
	void updateIdling(GameState &state);
	void updateCheckIfFalling(GameState &state);
	void updateBody(GameState &state, unsigned int &bodyTicksRemaining);
	void updateHands(GameState &state, unsigned int &handsTicksRemaining);
	void updateMovement(GameState &state, unsigned int &moveTicksRemaining, bool &wasUsingLift);
	void updateTurning(GameState &state, unsigned int &turnTicksRemaining);
	void updateDisplayedItem();
	void updateAttacking(GameState &state, unsigned int ticks);
	void updateAI(GameState &state, unsigned int ticks);

	void triggerProximity(GameState &state);
	void retreat(GameState &state);
	void dropDown(GameState &state);
	void tryToRiseUp(GameState &state);
	void fallUnconscious(GameState &state);
	void die(GameState &state, bool violently, bool bledToDeath = false);

	// Following members are not serialized, but rather are set in initBattle method

	sp<std::vector<sp<Image>>> strategyImages;
	StateRef<DoodadType> burningDoodad;
	sp<std::list<sp<Sample>>> genericHitSounds;
	sp<TileObjectBattleUnit> tileObject;
	sp<TileObjectShadow> shadowObject;

	/*
	- curr. mind state (controlled/berserk/…)
	- ref. to psi attacker (who is controlling it/...)
	*/
  private:
	friend class Battle;

	void startAttacking(GameState &state, WeaponStatus status);

	// Visibility theory (* is implemented)
	//
	// We update unit's vision to other stuff when
	// * unit changes position,
	// * unit changes facing,
	// * battlemappart or hazard changes in his field of vision
	//
	// We update other units's vision to this unit when:
	// * unit changes position
	// - unit changes "cloaked" flag

	void calculateVisionToTerrain(GameState &state, Battle &battle, TileMap &map,
	                              Vec3<float> eyesPos);
	void calculateVisionToUnits(GameState &state, Battle &battle, TileMap &map,
	                            Vec3<float> eyesPos);
	bool isWithinVision(Vec3<int> pos);

	// Update unit's vision of other units and terrain
	void refreshUnitVisibility(GameState &state, Vec3<float> oldPosition);
	// Update other units's vision of this unit
	void refreshUnitVision(GameState &state);
	// Update both this unit's vision and other unit's vision of this unit
	void refreshUnitVisibilityAndVision(GameState &state, Vec3<float> oldPosition);
};
}
