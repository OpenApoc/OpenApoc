#pragma once
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/battle/ai/unitai.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/gametime.h"
#include "game/state/rules/agenttype.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <vector>

// How many in-game ticks are required to travel one in-game unit for battleunits
#define TICKS_PER_UNIT_TRAVELLED_BATTLEUNIT 32
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
// Base movement ticks consumption rate, this allows us to divide by 2,3,4,5,6,8,9,10,12,15,18,20..
#define BASE_MOVETICKS_CONSUMPTION_RATE 360
// Movement cost in TUs for walking movement to adjacent (non-diagonal) tile
#define STANDART_MOVE_TU_COST 4

namespace OpenApoc
{

static const unsigned TICKS_REGEN_PER_TURN = TICKS_PER_TURN * 3;
static const unsigned TICKS_PER_PSI_CHECK = TICKS_PER_SECOND / 2;
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
// How many ticks to skip after weapon that was ready to fire could not fire
static const unsigned WEAPON_MISFIRE_DELAY_TICKS = TICKS_PER_SECOND / 16;
// How many times to wait for MIA target to come back before giving up
static const unsigned TIMES_TO_WAIT_FOR_MIA_TARGET =
    2 * TICKS_PER_SECOND / LOS_CHECK_INTERVAL_TRACKING;
// How many ticks are required to brainsuck a unit
static const unsigned TICKS_TO_BRAINSUCK = TICKS_PER_SECOND * 2;
// Chance out of 100 to be brainsucked
static const int BRAINSUCK_CHANCE = 66;
static const unsigned TICKS_SUPPRESS_SPOTTED_MESSAGES = TICKS_PER_TURN;
// As per Yataka Shimaoka on forums, cloaking effect returns after 2 seconds of inaction
static const unsigned CLOAK_TICKS_REQUIRED_UNIT = TICKS_PER_SECOND * 2;

class TileObjectBattleUnit;
class TileObjectShadow;
class Battle;
class DamageType;
class AIDecision;
class AIAction;
class AIMovement;
enum class GameEventType;
enum class DamageSource;
class Agent;

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

enum class ReserveShotMode
{
	Aimed = 1,
	Snap = 2,
	Auto = 4,
	None = 0
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

enum class PsiStatus
{
	NotEngaged,
	Control,
	Panic,
	Stun,
	Probe
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

class BattleUnit : public StateObject<BattleUnit>, public std::enable_shared_from_this<BattleUnit>
{
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
	// How many times we checked and target we are ready to fire on was MIA
	unsigned int timesTargetMIA = 0;
	// Unit we're targeting right now
	StateRef<BattleUnit> targetUnit;
	// Unit we're ordered to focus on (in real time)
	StateRef<BattleUnit> focusUnit;
	// Units focusing this unit
	std::list<StateRef<BattleUnit>> focusedByUnits;
	// Ticks until we check if target is still valid, turn to it etc.
	unsigned int ticksUntillNextTargetCheck = 0;

	// Psi

	PsiStatus psiStatus = PsiStatus::NotEngaged;
	// Unit in process of being psi attacked by this
	StateRef<BattleUnit> psiTarget;
	// Item used for psi attack
	StateRef<AEquipmentType> psiItem;
	// Ticks accumulated towards next psi check
	unsigned int ticksAccumulatedToNextPsiCheck = 0;
	// Map of units and attacks in progress against this unit
	std::map<UString, PsiStatus> psiAttackers;

	// Brainsucking

	StateRef<BattleUnit> brainSucker;

	// Stats

	// Accumulated xp points for each stat
	AgentStats experiencePoints;
	// Points earned for kills
	int combatRating = 0;
	// Fatal wounds for each body part
	std::map<BodyPart, int> fatalWounds;
	// Which body part is medikit used on
	BodyPart healingBodyPart = BodyPart::Body;
	// Is using a medikit
	bool isHealing = false;
	// Ticks towards next wound damage or medikit heal application
	unsigned int woundTicksAccumulated = 0;
	// Ticks towards next regeneration of stats (psi, stamina, stun)
	unsigned int regenTicksAccumulated = 0;
	// Stun damage acquired
	int stunDamage = 0;
	// Ticks accumulated towards next enzyme hit
	unsigned int enzymeDebuffTicksAccumulated = 0;
	// Enzyme debuff intensity remaining
	int enzymeDebuffIntensity = 0;
	// Ticks accumulated towards next fire hit
	unsigned int fireDebuffTicksAccumulated = 0;
	// Fire debuff intensity remaining
	unsigned int fireDebuffTicksRemaining = 0;
	// State of unit's morale
	MoraleState moraleState = MoraleState::Normal;
	// How much time unit has to spend in low morale state
	unsigned int moraleStateTicksRemaining = 0;
	// Ticks accumulated towards next morale check
	unsigned int moraleTicksAccumulated = 0;
	// TU at turn start (used to calculate percentage of TU)
	int initialTU = 0;
	// TU to reserve for shot
	int reserveShotCost = 0;
	// Stealth, increases each turn, set to 0 when taking action or no stealth in hand
	// Unit is cloaked when this is >= CLOAK_TICKS_REQUIRED_UNIT
	unsigned int cloakTicksAccumulated = 0;
	// Ticks until sound is emitted
	int ticksUntillNextCry = 0;

	// User set modes

	BehaviorMode behavior_mode = BehaviorMode::Normal;
	WeaponAimingMode fire_aiming_mode = WeaponAimingMode::Snap;
	FirePermissionMode fire_permission_mode = FirePermissionMode::AtWill;
	MovementMode movement_mode = MovementMode::Walking;
	KneelingMode kneeling_mode = KneelingMode::None;
	ReserveShotMode reserve_shot_mode = ReserveShotMode::None;
	KneelingMode reserve_kneel_mode = KneelingMode::None;

	// Body

	// Time, in game ticks, until body animation is finished
	unsigned int body_animation_ticks_remaining = 0;
	// Required for transition of derived params, like muzzle location
	unsigned int body_animation_ticks_total = 0;
	// Animations ticks for static body state (no body state change is in progress)
	unsigned int body_animation_ticks_static = 0;
	BodyState current_body_state = BodyState::Standing;
	BodyState target_body_state = BodyState::Standing;

	// Hands

	// Time, in game ticks, until hands animation is finished
	unsigned int hand_animation_ticks_remaining = 0;
	// Time, in game ticks, until unit will lower it's weapon
	unsigned int residual_aiming_ticks_remaining = 0;
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
	// Launched (will check launch goal if falling, will travel by parabola if moving)
	bool launched = false;
	// Goal we launched for, after reaching this will set xy velocity to 0
	Vec3<float> launchGoal;
	// Bounced after falling
	bool bounced = false;
	// Ticks to ignore collision when launching
	unsigned int collisionIgnoredTicks = 0;
	// Current falling/jumping speed
	Vec3<float> velocity;
	// Movement counter for TB motion scanner
	unsigned int tilesMoved = 0;

	// Turning

	// Time, in game ticks, until unit can turn by 1/8th of a circle
	unsigned int turning_animation_ticks_remaining = 0;
	Vec2<int> facing;
	Vec2<int> goalFacing;

	// Missions

	// Mission list
	std::list<up<BattleUnitMission>> missions;

	// Vision

	// Item shown in unit's hands
	// Units without inventory never show items in hands
	StateRef<AEquipmentType> displayedItem;
	// Visible units from other orgs
	std::set<StateRef<BattleUnit>> visibleUnits;
	// Visible units that are hostile to us
	std::set<StateRef<BattleUnit>> visibleEnemies;

	// Miscellaneous

	// Unit successfully retreated from combat
	bool retreated = false;

	// Unit died and corpse was destroyed in an explosion
	bool destroyed = false;

	// If unit is asked to give way, this list will be filled with facings
	// in order of priority that should be tried by it
	std::list<Vec2<int>> giveWayRequestData;

	// AI list
	AIBlockUnit aiList;

	// [Methods]

	// Misc methods

	// Called before unit is added to the map itself
	void init(GameState &state);
	// Clears all possible cases of circular refs
	void destroy() override;

	// Squad

	// Remove unit from squad in battle's forces
	void removeFromSquad(Battle &b);
	// Assign unit to squad (optionally specify squad number and position)
	bool assignToSquad(Battle &b, int squadNumber = -1, int squadPosition = -1);

	// Fatal wounds

	// Whether unit is fatally wounded
	bool isFatallyWounded();
	// Add fatal wound to a body part
	void addFatalWound(BodyPart fatalWoundPart);

	// Attacking and turning to hostiles

	// Get full cost of attacking (including turn and pose change)
	int getAttackCost(GameState &state, AEquipment &item, Vec3<int> tile);
	// Set unit's focus (RT)
	void setFocus(GameState &state, StateRef<BattleUnit> unit);
	// Start attacking a unit
	bool startAttacking(GameState &state, StateRef<BattleUnit> unit,
	                    WeaponStatus status = WeaponStatus::FiringBothHands);
	// Start attacking a tile
	bool startAttacking(GameState &state, Vec3<int> tile,
	                    WeaponStatus status = WeaponStatus::FiringBothHands, bool atGround = false);
	// Cease attacking
	void stopAttacking();
	// Returns which hands can be used for an attack (or none if attack cannot be made)
	// Checks whether target unit is in range, and clear LOF exists to it
	WeaponStatus canAttackUnit(GameState &state, sp<BattleUnit> unit);
	// Returns whether unit can be attacked by one of the two supplied weapons
	// Checks whether target unit is in range, and clear LOF exists to it
	WeaponStatus canAttackUnit(GameState &state, sp<BattleUnit> unit, sp<AEquipment> rightHand,
	                           sp<AEquipment> leftHand = nullptr);
	// Clear LOF means no friendly fire and no map part in between
	// Clear LOS means nothing in between
	bool hasLineToUnit(const sp<BattleUnit> unit, bool useLOS = false) const;
	// Clear LOF means no friendly fire and no map part in between
	// Clear LOS means nothing in between
	bool hasLineToPosition(Vec3<float> targetPosition, bool useLOS = false) const;

	// Psi

	// Get cost of psi attack or upkeep
	int getPsiCost(PsiStatus status, bool attack = true);
	// Get chance of psi attack to succeed
	int getPsiChance(StateRef<BattleUnit> target, PsiStatus status, StateRef<AEquipmentType> item);
	// Starts attacking target, returns if attack successful
	bool startAttackPsi(GameState &state, StateRef<BattleUnit> target, PsiStatus status,
	                    StateRef<AEquipmentType> item);
	// Stop / break psi attack
	void stopAttackPsi(GameState &state);
	// Applies psi attack effects to this unit, returns false if attack must be terminated because
	// of some failure
	void applyPsiAttack(GameState &state, BattleUnit &attacker, PsiStatus status,
	                    StateRef<AEquipmentType> item, bool impact);
	// Change unit's owner (mind control)
	void changeOwner(GameState &state, StateRef<Organisation> newOwner);

	// Items

	// Attempts to use item, returns if success
	bool useItem(GameState &state, sp<AEquipment> item);
	// Use medikit on bodypart
	bool useMedikit(GameState &state, BodyPart part);
	// Use brainsucker ability
	bool useBrainsucker(GameState &state);
	// Use unit spawner (suicide)
	bool useSpawner(GameState &state, const AEquipmentType &item);

	// Body

	// Get body animation frame to be drawn
	unsigned int getBodyAnimationFrame() const;
	// Set unit's body state
	void setBodyState(GameState &state, BodyState bodyState);
	// Begin unit's body change towards target body state
	void beginBodyStateChange(GameState &state, BodyState bodyState);

	// Hands

	// Get hand animation frame to be drawn
	unsigned int getHandAnimationFrame() const;
	// Set unit's hand state
	void setHandState(HandState state);
	// Begin unit's hand state change towards target state
	void beginHandStateChange(HandState state);
	// Whether unit is allowed to change handState to target state
	bool canHandStateChange(HandState state);

	// Movement

	// Set unit's movement state
	void setMovementState(MovementState state);
	// Get distance that unit has travelled (used in drawing and footstep sounds)
	unsigned int getDistanceTravelled() const;

	// User set modes

	// Set unit preferred movement mode
	void setMovementMode(MovementMode mode);
	// Set unit preferred kneeling mode
	void setKneelingMode(KneelingMode mode);
	// Set unit preferred weapon aiming mode
	void setWeaponAimingMode(WeaponAimingMode mode);
	// Set unit fire permission mode
	void setFirePermissionMode(FirePermissionMode mode);
	// Set unit behavior mode
	void setBehaviorMode(BehaviorMode mode);
	// Set unit's reserve TU for fire mode
	void setReserveShotMode(GameState &state, ReserveShotMode mode);
	// Set unit's reserve TU for kneeling mode
	void setReserveKneelMode(KneelingMode mode);

	// Sound

	// Should the unit play walk step sound now
	bool shouldPlaySoundNow();
	// Current sound index for the unit
	unsigned int getWalkSoundIndex();
	// Play walking sound
	void playWalkSound(GameState &state);
	// Play sound adjusting gain by distance to closest player unit
	void playDistantSound(GameState &state, sp<Sample> sfx, float gainMult = 1.0f);
	// Init the delay before first time unit emits a cry in combat
	void initCryTimer(GameState &state);
	// Reset the delay before unit emits a cry next time in combat
	void resetCryTimer(GameState &state);

	// Movement and position

	// Set unit's goal to unit's position
	void resetGoal();
	// Get new goal for unit position, returns true if goal acquired
	bool getNewGoal(GameState &state);
	// Updates to do when unit reached goal
	void onReachGoal(GameState &state);
	// Calculate velocity for jumping (manually) towards target,
	// returning fastest allowed velocity that makes unit drop on top of the tile.
	// Returns true if successful, false if not
	// For explanation how it works look @ AEquipment::calculateNextVelocityForThrow
	bool calculateVelocityForLaunch(float distanceXY, float diffZ, float &velocityXY,
	                                float &velocityZ, float initialXY = 0.5f);
	// Calculates velocity for jumping (during movement, auto-jump over tile gap)
	// For explanation how it works look @ AEquipment::calculateNextVelocityForThrow
	void calculateVelocityForJump(float distanceXY, float diffZ, float &velocityXY,
	                              float &velocityZ, bool diagonAlley);
	// Returns whether unit can launch towards target position
	bool canLaunch(Vec3<float> targetPosition);
	// Returns whether unit can launch, and calculates all vectors and velocities if yes
	bool canLaunch(Vec3<float> targetPosition, Vec3<float> &targetVectorXY, float &velocityXY,
	               float &velocityZ);
	// Launch unit towards target position
	void launch(GameState &state, Vec3<float> targetPosition,
	            BodyState bodyState = BodyState::Standing);
	// Start unit's falling routine
	void startFalling(GameState &state);
	// Make unit move
	void startMoving(GameState &state);
	// Set unit's position
	void setPosition(GameState &state, const Vec3<float> &pos, bool goal = false);
	// Get unit's velocity (for purpose of leading the unit
	Vec3<float> getVelocity() const;

	// Turning

	// Set unit's facing
	void setFacing(GameState &state, Vec2<int> newFacing);
	// Begin turning unit towards new facing
	void beginTurning(GameState &state, Vec2<int> newFacing);

	// Missions

	// Pops all finished missions, returns true if popped
	bool popFinishedMissions(GameState &state);
	// Returns whether unit has a mission queued that will make it move
	bool hasMovementQueued();
	// Gets next destination from current mission, returns true if got one
	bool getNextDestination(GameState &state, Vec3<float> &dest);
	// Gets next facing from current mission, returns true if got one
	bool getNextFacing(GameState &state, Vec2<int> &dest);
	// Gets next body state from current mission, returns true if got one
	bool getNextBodyState(GameState &state, BodyState &dest);
	// Add mission, if possible to the front, otherwise to the back
	// (or, if toBack = true, then always to the back)
	// Returns true if succeeded in adding
	bool addMission(GameState &state, BattleUnitMission *mission, bool toBack = false);
	// Add parameterless mission by type, if possible to front, otherwise to the back
	// Returns true if succeeded in adding
	bool addMission(GameState &state, BattleUnitMission::Type type);
	// Attempt to cancel all unit missions, returns true if successful
	bool cancelMissions(GameState &state, bool forced = false);
	// Attempt to give unit a new mission, replacing all others, returns true if successful
	bool setMission(GameState &state, BattleUnitMission *mission);

	// TB / TU functions

	// Refresh unit's reserve cost
	void refreshReserveCost(GameState &state);
	// Returns whether unit can afford action
	bool canAfford(GameState &state, int cost, bool ignoreKneelReserve = false,
	               bool ignoreShootReserve = false) const;
	// Returns if unit did spend (false if insufficient TUs)
	bool spendTU(GameState &state, int cost, bool ignoreKneelReserve = false,
	             bool ignoreShootReserve = false, bool allowInterrupt = false);
	// Spend all remaining TUs for the unit
	void spendRemainingTU(GameState &state, bool allowInterrupt = false);
	// Do routine that must be done at unit's turn start
	// Called after updateTB was called
	void beginTurn(GameState &state);

	// TU costs

	// Get cost to pick up item from ground
	int getPickupCost() const;
	// Get cost to throw item
	int getThrowCost() const;
	// Get cost to use medikit
	int getMedikitCost() const;
	// Get cost to use motion scanner
	int getMotionScannerCost() const;
	// Get cost to use teleporter
	int getTeleporterCost() const;
	// Get cost to turn 1 frame
	int getTurnCost() const;
	// Get cost to change body state
	int getBodyStateChangeCost(BodyState from, BodyState to) const;

	// AI execution

	// Execute group decision made by AI
	static void executeGroupAIDecision(GameState &state, AIDecision &decision,
	                                   std::list<StateRef<BattleUnit>> &units);
	// Execute decision
	void executeAIDecision(GameState &state, AIDecision &decision);
	// Execute action
	void executeAIAction(GameState &state, AIAction &action);
	// Execute movement
	void executeAIMovement(GameState &state, AIMovement &movement);

	// Notifications

	// Called to notify unit he's under fire
	void notifyUnderFire(GameState &state, Vec3<int> position, bool visible);
	// Called to notify unit he's hit
	void notifyHit(Vec3<int> position);

	// Experience

	// Returns a roll for primary state increase based on how much experience was acquired
	int rollForPrimaryStat(GameState &state, int experience);
	// Process unit experience into stat increases
	void processExperience(GameState &state);

	// Unit state queries

	// Returns true if the unit is dead
	bool isDead() const;
	// Returns true if the unit is unconscious and not dead
	bool isUnconscious() const;
	// Return true if the unit is conscious
	// and not currently in the dropped state (current and target)
	bool isConscious() const;
	// So we can stop going ->isLarge() all the time
	bool isLarge() const { return agent->type->bodyType->large; }
	// Whether unit is static - not moving, not falling, not changing body state
	bool isStatic() const;
	// Whether unit is busy - with aiming or firing or otherwise involved
	bool isBusy() const;
	// Whether unit is firing its weapon (or aiming in preparation of firing)
	bool isAttacking() const;
	// Whether unit is throwing an item
	bool isThrowing() const;
	// Whether unit is moving (has Goto Location queued)
	bool isMoving() const;
	// Whether unit is doing a specific mission (has it queued)
	bool isDoing(BattleUnitMission::Type missionType) const;
	// Return unit's general type
	BattleUnitType getType() const;
	// Whether unit is AI controlled
	bool isAIControlled(GameState &state) const;
	// Cloaked status
	bool isCloaked() const;
	// Unit's current AI type (can be modified by panic etc.)
	AIType getAIType() const;

	// Returns true if the unit is conscious and can fly
	bool canFly() const;
	// Returns true if the unit is conscious and can fly
	bool canMove() const;
	// Whether unit can go prone in position and facing
	bool canProne(Vec3<int> pos, Vec2<int> fac) const;
	// Whether unit can kneel in current position and facing
	bool canKneel() const;

	// Get unit's height in current situation
	int getCurrentHeight() const { return agent->type->bodyType->height.at(current_body_state); }
	// Get unit's gun muzzle location (where shots come from)
	Vec3<float> getMuzzleLocation() const;
	// Get unit's eyes (where vision rays come from)
	// FIXME: This likely won't work properly for large units
	// Idea here is to LOS from the center of the occupied tile
	Vec3<float> getEyeLocation() const;
	// Get thrown item's departure location
	Vec3<float> getThrownItemLocation() const;

	// Determine body part hit
	BodyPart determineBodyPartHit(StateRef<DamageType> damageType, Vec3<float> cposition,
	                              Vec3<float> direction);
	// Returns true if sound and doodad were handled by it
	bool applyDamage(GameState &state, int power, StateRef<DamageType> damageType,
	                 BodyPart bodyPart, DamageSource source,
	                 StateRef<BattleUnit> attacker = nullptr);
	// Apply damage directly (after all calculations)
	void applyDamageDirect(GameState &state, int damage, bool generateFatalWounds,
	                       BodyPart fatalWoundPart, int stunPower,
	                       StateRef<BattleUnit> attacker = nullptr, bool violent = true);

	// Returns true if sound and doodad were handled by it
	bool handleCollision(GameState &state, Collision &c);

	// Get unit's position
	const Vec3<float> &getPosition() const { return this->position; }

	// Misc

	// Request this unit give way to other unit
	void requestGiveWay(const BattleUnit &requestor, const std::list<Vec3<int>> &plannedPath,
	                    Vec3<int> pos);
	// Apply enzyme to this unit
	void applyEnzymeEffect(GameState &state);
	// Spawn enzyme smoke on this unit
	void spawnEnzymeSmoke(GameState &state);
	// Send AgentEvent of specified type
	// checkOwnership means event not set unless agent owned by player
	// checkVisibility means event not sent unless agent seen by player
	void sendAgentEvent(GameState &state, GameEventType type, bool checkOwnership = false,
	                    bool checkVisibility = false) const;
	// Trigger proximity mines around unit
	void triggerProximity(GameState &state);
	// Trigger brainsucker pods around unit
	void triggerBrainsuckers(GameState &state);
	// Retreat unit
	void retreat(GameState &state);
	// Drop unit to the ground
	void dropDown(GameState &state);
	// Try to make unit rise (may fail if other unit stands on top)
	void tryToRiseUp(GameState &state);
	// Process unit becoming unconscious
	void fallUnconscious(GameState &state, StateRef<BattleUnit> attacker = nullptr);
	// Process unit dying
	void die(GameState &state, StateRef<BattleUnit> attacker = nullptr, bool violently = true);
	// Remove unit from any 'visible' lists
	void markUnVisible(GameState &state);

	// Update

	// Main update function
	void update(GameState &state, unsigned int ticks);
	// Update function for TB (called when unit's turn begins
	void updateTB(GameState &state);
	// Updates unit's cloak status
	void updateCloak(GameState &state, unsigned int ticks);
	// Updates unit bleeding, debuffs and morale states
	void updateStateAndStats(GameState &state, unsigned int ticks);
	// Updates unit's morale
	void updateMorale(GameState &state, unsigned int ticks);
	// Updates unit's wounds and healing
	void updateWoundsAndHealing(GameState &state, unsigned int ticks);
	// Updates unit's regeneration
	void updateRegen(GameState &state, unsigned int ticks);
	// Updates unit give way request and events
	void updateEvents(GameState &state);
	// Updates unit's give way (give way if requested)
	void updateGiveWay(GameState &state);
	// Updates unit that is idle
	void updateIdling(GameState &state);
	// Update crying
	void updateCrying(GameState &state);
	// Checks if unit should begin falling
	void updateCheckBeginFalling(GameState &state);
	// Updates unit's body transition and acquires new target body state
	void updateBody(GameState &state, unsigned int &bodyTicksRemaining);
	// Updates unit's hands transition
	void updateHands(GameState &state, unsigned int &handsTicksRemaining);
	// Updates unit's movement if unit is falling
	void updateMovementFalling(GameState &state, unsigned int &moveTicksRemaining,
	                           bool &wasUsingLift);
	// Update unit falling into another unit
	void updateFallingIntoUnit(GameState &state, BattleUnit &unit);
	// Updates unit's movement if unit is sucking brain
	void updateMovementBrainsucker(GameState &state, unsigned int &moveTicksRemaining,
	                               bool &wasUsingLift);
	// Updates unit's movement if unit is jumping (over one tile gap)
	void updateMovementJumping(GameState &state, unsigned int &moveTicksRemaining,
	                           bool &wasUsingLift);
	// Updates unit's movement if unit is moving normally
	void updateMovementNormal(GameState &state, unsigned int &moveTicksRemaining,
	                          bool &wasUsingLift);
	// Updates unit's movement
	// Return true if retreated or destroyed and we must halt immediately
	void updateMovement(GameState &state, unsigned int &moveTicksRemaining, bool &wasUsingLift);
	// Updates unit's transition and acquires new target
	void updateTurning(GameState &state, unsigned int &turnTicksRemaining,
	                   unsigned int const handsTicksRemaining);
	// Updates unit's displayed item (which one will draw in unit's hands on screen)
	void updateDisplayedItem(GameState &state);
	// Runs all fire checks and returns false if we must stop attacking
	bool updateAttackingRunCanFireChecks(GameState &state, unsigned int ticks,
	                                     sp<AEquipment> &weaponRight, sp<AEquipment> &weaponLeft,
	                                     Vec3<float> &targetPosition);
	// Updates unit's attacking parameters (gun cooldown, hand states, aiming etc)
	void updateAttacking(GameState &state, unsigned int ticks);
	// Updates unit firing weapons
	void updateFiring(GameState &state, sp<AEquipment> &weaponLeft, sp<AEquipment> &weaponRight,
	                  Vec3<float> &targetPosition);
	// Updates unit's psi attack (sustain payment, effect application etc.)
	void updatePsi(GameState &state, unsigned int ticks);
	// Updates unit's AI list
	void updateAI(GameState &state, unsigned int ticks);

	// Following members are not serialized, but rather are set in initBattle method

	sp<std::vector<sp<Image>>> strategyImages;
	StateRef<DoodadType> burningDoodad;
	sp<std::list<sp<Sample>>> genericHitSounds;
	sp<std::list<sp<Sample>>> psiSuccessSounds;
	sp<std::list<sp<Sample>>> psiFailSounds;
	sp<TileObjectBattleUnit> tileObject;
	sp<TileObjectShadow> shadowObject;

	/*
	- curr. mind state (controlled/berserk/)
	- ref. to psi attacker (who is controlling it/...)
	*/
  private:
	friend class Battle;

	// Start attacking (inner function which initialises everything regardless of target)
	bool startAttacking(GameState &state, WeaponStatus status);
	// Start psi (internal function which actually does the attack)
	bool startAttackPsiInternal(GameState &state, StateRef<BattleUnit> target, PsiStatus status,
	                            StateRef<AEquipmentType> item);

	// Calculate unit's vision to terrain
	void calculateVisionToTerrain(GameState &state);
	// Calculate unit's vision to LBs checking every one independently
	// Figure out a center tile of a block and check if it's visible (no collision)
	void calculateVisionToLosBlocks(GameState &state, std::set<int> &discoveredBlocks,
	                                std::set<int> &blocksToCheck);
	// Calculate unit's vision to LBs using "shotgun" approach:
	// Shoot 25 beams and include everything that was passed through into list of visible blocks
	void calculateVisionToLosBlocksLazy(GameState &state, std::set<int> &discoveredBlocks);
	// Calculate vision to every other unit
	void calculateVisionToUnits(GameState &state);
	// Return if unit sees unit
	bool calculateVisionToUnit(GameState &state, BattleUnit &u);
	// Return if target is within vision cone
	bool isWithinVision(Vec3<int> pos);

	// Update unit's vision of other units and terrain
	void refreshUnitVisibility(GameState &state);
	// Update other units's vision of this unit
	void refreshUnitVision(GameState &state, bool forceBlind = false,
	                       StateRef<BattleUnit> targetUnit = StateRef<BattleUnit>());
	// Update both this unit's vision and other unit's vision of this unit
	void refreshUnitVisibilityAndVision(GameState &state);
};
} // namespace OpenApoc
