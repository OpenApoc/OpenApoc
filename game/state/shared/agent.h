#pragma once

#include "framework/image.h"
#include "game/state/gametime.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/agenttype.h"
#include "game/state/shared/equipment.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>
#include <set>

// How many in-game ticks are required to travel one in-game unit
#define TICKS_PER_UNIT_TRAVELLED_AGENT 8

namespace OpenApoc
{

static const unsigned TELEPORT_TICKS_REQUIRED_AGENT = TICKS_PER_SECOND * 30;

class Organisation;
class AEquipment;
class BattleUnitAnimationPack;
class BattleUnitImagePack;
class Sample;
class AgentBodyType;
class BattleUnit;
class DamageModifier;
class DamageType;
class Building;
class Vehicle;
class AgentMission;
class VoxelMap;
class City;
enum class AIType;

enum class Rank
{
	Rookie = 0,
	Squaddie = 1,
	SquadLeader = 2,
	Sergeant = 3,
	Captain = 4,
	Colonel = 5,
	Commander = 6
};

enum class TrainingAssignment
{
	None,
	Physical,
	Psi
};

class Agent : public StateObject<Agent>,
              public std::enable_shared_from_this<Agent>,
              public EquippableObject
{
  public:
	Agent() = default;

	StateRef<AgentType> type;

	UString name;

	// Appearance that this specific agent chose from available list of its type
	int appearance = 0;
	int portrait = 0;
	const AgentPortrait &getPortrait() const { return type->portraits.at(gender).at(portrait); }
	AgentType::Gender gender = AgentType::Gender::Male;

	AgentStats initial_stats;  // Stats at agent creation
	AgentStats current_stats;  // Stats after agent training/improvement
	AgentStats modified_stats; // Stats after 'temporary' modification (health damage, slowdown due
	                           // to equipment weight, used stamina etc)
	bool overEncumbred = false;
	Rank rank = Rank::Rookie;

	unsigned int teleportTicksAccumulated = 0;
	bool canTeleport() const;
	bool hasTeleporter() const;

	// Training
	unsigned trainingPhysicalTicksAccumulated = 0;
	unsigned trainingPsiTicksAccumulated = 0;
	TrainingAssignment trainingAssignment = TrainingAssignment::None;

	bool recentlyHired = false;
	bool recentryTransferred = false;
	bool recentlyFought = false;
	float healingProgress = 0.0f;

	void assignTraining(TrainingAssignment assignment);

	void hire(GameState &state, StateRef<Building> newHome);
	void transfer(GameState &state, StateRef<Building> newHome);

	sp<AEquipment> getArmor(BodyPart bodyPart) const;
	bool isBodyStateAllowed(BodyState bodyState) const;
	bool isMovementStateAllowed(MovementState movementState) const;
	bool isFireDuringMovementStateAllowed(MovementState movementState) const;
	bool isFacingAllowed(Vec2<int> facing) const;
	const std::set<Vec2<int>> *getAllowedFacings() const;
	int getReactionValue() const;
	int getTULimit(int reactionValue) const;
	UString getRankName() const;
	// Get relevant skill
	int getSkill() const;

	StateRef<Organisation> owner;

	StateRef<City> city;
	StateRef<Building> homeBuilding;
	// Building the agent is currently stored inside, nullptr if it's in the city or a vehicle
	StateRef<Building> currentBuilding;
	// Vehicle the agent is currently stored inside, nullptr if it's in the city in a building
	StateRef<Vehicle> currentVehicle;

	/* leave the building and put agent into the city */
	void leaveBuilding(GameState &state, Vec3<float> initialPosition);
	/* 'enter' the agent in a building from city or vehicle*/
	void enterBuilding(GameState &state, StateRef<Building> b);
	/* 'enter' the agent in a vehicle from building*/
	void enterVehicle(GameState &state, StateRef<Vehicle> v);
	// Note that agent cannot ever leave vehicle into city, or enter vehicle from city

	// Agent's position in the city
	Vec3<float> position = {0, 0, 0};
	// Position agent is moving towards
	Vec3<float> goalPosition = {0, 0, 0};

	StateRef<Lab> lab_assigned = nullptr;
	bool assigned_to_lab = false;

	StateRef<BattleUnit> unit;

	std::list<up<AgentMission>> missions;
	std::list<sp<AEquipment>> equipment;
	bool canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> equipmentType,
	                     EquipmentSlotType &slotType) const;
	bool canAddEquipment(Vec2<int> pos, StateRef<AEquipmentType> equipmentType) const;
	Vec2<int> findFirstSlotByType(EquipmentSlotType slotType,
	                              StateRef<AEquipmentType> equipmentType = nullptr);
	Vec2<int> findFirstSlot(StateRef<AEquipmentType> equipmentType = nullptr);
	// Add equipment by type to the first available slot of any type
	sp<AEquipment> addEquipmentByType(GameState &state, StateRef<AEquipmentType> equipmentType,
	                                  bool allowFailure);
	// Add equipment to the first available slot of a specific type
	sp<AEquipment> addEquipmentByType(GameState &state, StateRef<AEquipmentType> equipmentType,
	                                  EquipmentSlotType slotType, bool allowFailure);
	// Add equipment by type to a specific position
	sp<AEquipment> addEquipmentByType(GameState &state, Vec2<int> pos,
	                                  StateRef<AEquipmentType> equipmentType);
	// Add equipment as ammo
	sp<AEquipment> addEquipmentAsAmmoByType(StateRef<AEquipmentType> equipmentType);

	// Add equipment to the first available slot of a specific type
	void addEquipment(GameState &state, sp<AEquipment> object, EquipmentSlotType slotType);
	// Add equipment to a specific position
	void addEquipment(GameState &state, Vec2<int> pos, sp<AEquipment> object);
	void removeEquipment(GameState &state, sp<AEquipment> object);
	void updateSpeed();
	// Called when current stats were changed and modified stats need to catch up
	void updateModifiedStats();
	bool canRun() { return modified_stats.canRun(); }

	void updateIsBrainsucker();
	bool isBrainsucker = false;

	// Adds mission to list of missions, returns true if successful
	bool addMission(GameState &state, AgentMission *mission, bool toBack = false);
	// Replaces all missions with provided mission, returns true if successful
	bool setMission(GameState &state, AgentMission *mission);

	// Pops all finished missions, returns true if popped
	bool popFinishedMissions(GameState &state);
	// Get new goal for vehicle position or facing
	bool getNewGoal(GameState &state);

	void die(GameState &state, bool silent = false);
	bool isDead() const;

	// for agents spawned specifically for the current battle, like turrets
	bool destroyAfterBattle = false;

	// Update agent in city
	void update(GameState &state, unsigned ticks);
	void updateEachSecond(GameState &state);
	void updateDaily(GameState &state);
	void updateHourly(GameState &state);
	void updateMovement(GameState &state, unsigned ticks);

	void trainPhysical(GameState &state, unsigned ticks);
	void trainPsi(GameState &state, unsigned ticks);

	StateRef<BattleUnitAnimationPack> getAnimationPack() const;
	// If item was fired before, it should be passed here, and it will remain dominant unless it was
	// removed
	StateRef<AEquipmentType>
	getDominantItemInHands(GameState &state,
	                       StateRef<AEquipmentType> itemLastFired = nullptr) const;
	sp<AEquipment> getFirstItemInSlot(EquipmentSlotType equipmentSlotType, bool lazy = true) const;
	sp<AEquipment> getFirstShield(GameState &state) const;
	sp<AEquipment> getFirstItemByType(StateRef<AEquipmentType> equipmentType) const;
	sp<AEquipment> getFirstItemByType(AEquipmentType::Type itemType) const;

	StateRef<BattleUnitImagePack> getImagePack(BodyPart bodyPart) const;

	bool drawLines() const override { return false; }
	sp<Equipment> getEquipmentAt(const Vec2<int> &position) const override;
	const std::list<EquipmentLayoutSlot> &getSlots() const override;
	std::list<std::pair<Vec2<int>, sp<Equipment>>> getEquipment() const override;

	int getMaxHealth() const;
	int getHealth() const;

	int getMaxShield(GameState &state) const;
	int getShield(GameState &state) const;

	// Following members are not serialized, but rather are set up in the initBattle method

	sp<AEquipment> leftHandItem;  // Left hand item, frequently accessed so will be stored here
	sp<AEquipment> rightHandItem; // Right hand item, frequently accessed so will be stored here

	void destroy() override;
};

class AgentGenerator
{
  public:
	AgentGenerator() = default;
	// FIXME: I think there should be some kind of 'nationality' stuff going on here
	std::map<AgentType::Gender, std::list<UString>> first_names;
	std::list<UString> second_names;

	// Create an agent of specified role
	StateRef<Agent> createAgent(GameState &state, StateRef<Organisation> org,
	                            AgentType::Role role) const;
	// Create an agent of specified type
	StateRef<Agent> createAgent(GameState &state, StateRef<Organisation> org,
	                            StateRef<AgentType> type) const;
};

} // namespace OpenApoc
