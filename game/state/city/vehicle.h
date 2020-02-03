#pragma once

#include "game/state/city/vehiclemission.h"
#include "game/state/gametime.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/shared/equipment.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

// Uncomment to allow projectiles to shoot down friendly projectiles
//#define DEBUG_ALLOW_PROJECTILE_ON_PROJECTILE_FRIENDLY_FIRE

namespace OpenApoc
{
// Smoke slightly over vehicle.
static const Vec3<float> SMOKE_DOODAD_SHIFT{0.0f, 0.0f, 0.25f};

static const unsigned CLOAK_TICKS_REQUIRED_VEHICLE = TICKS_PER_SECOND * 3 / 2;
static const unsigned TELEPORT_TICKS_REQUIRED_VEHICLE = TICKS_PER_SECOND * 30;
static const unsigned TICKS_AUTO_ACTION_DELAY = TICKS_PER_SECOND / 4;
static const unsigned TICKS_CARGO_TTL = 6 * TICKS_PER_HOUR;
static const unsigned TICKS_CARGO_WARNING = TICKS_PER_HOUR;

// Falling vehicle params

static const float FV_ACCELERATION = 0.16666667f;
// How much max damage can colliding with scenery deal to us
static const float FV_COLLISION_DAMAGE_LIMIT = 10.0f;
// Minimal collision damage as percentage of health
static const float FV_COLLISION_DAMAGE_MIN = 1.0f / 100.0f;
// How much is constitution multiplied by to figure out collision damage to us
static const float FV_COLLISION_DAMAGE_CONSTITUTION_MULTIPLIER = 0.5f;
// Flat chance (in percent) to plow through scenery
static const float FV_PLOW_CHANCE_FLAT = 50.0f;
// Vehicle weight multiplied by this and by speed multiplier below
// is added to percent chance of plowing through
static const float FV_PLOW_CHANCE_WEIGHT_MULTIPLIER = 1.0f / 125.0f;
// If velocity > this then a multiplier is applied to above chance
static const float FV_PLOW_CHANCE_HIGH_SPEED_THRESHOLD = 15.0f;
// If velocity > threshold then this multiplier is applied to weight-based chance
static const float FV_PLOW_CHANCE_HIGH_SPEED_MULTIPLIER = 1.5f;
// How many percent per constitution is plow through chance decreased by
static const float FV_PLOW_CHANCE_CONSTITUTION_MULTIPLIER = 2.0f;
// How much 1 of X is the damage evasion chance (i.e. 8 means 1/8th or 12.5%)
static const int FV_COLLISION_DAMAGE_ONE_IN_CHANCE_TO_EVADE = 8;
// How much X in 100 is the chance for recovered vehicle to arrive intact (otherwise scrapped)
static const int FV_CHANCE_TO_RECOVER_VEHICLE = 100;
// How much X in 100 is chance to recover every equipment part (otherwise scrapped)
static const int FV_CHANCE_TO_RECOVER_EQUIPMENT = 90;
// How much percent is "scrapped" sold for
static const int FV_SCRAPPED_COST_PERCENT = 25;
// How much ticks is accumulated per second of engine usage
static const int FUEL_TICKS_PER_SECOND = 144;
// How much ticks is required to spend one unit of fuel
static const int FUEL_TICKS_PER_UNIT = 40000;

class Image;
class TileObjectVehicle;
class TileObjectShadow;
class Vehicle;
class AEquipmentType;
class Organisation;
class VehicleMission;
class Building;
class GameState;
class ResearchTopic;
class TileObjectProjectile;
class VEquipment;
class VEquipmentType;
class VAmmoType;
class City;
class Doodad;
class TileMap;
class Agent;
class Collision;

class Cargo
{
  public:
	enum class Type
	{
		Agent,
		Bio,
		VehicleAmmo,
		VehicleEquipment
	};

	// Item type (Agent equipment, bio-containment item or vehicle equipment
	Type type = Type::Agent;
	// Item id
	UString id;
	// Amount of items
	int count = 0;
	// For agent ammo this is equal to max ammo
	int divisor = 0;
	// Space one item takes
	int space = 0;
	// Cost of one item
	int cost = 0;
	// Original owner
	StateRef<Organisation> originalOwner;
	// Destination building
	StateRef<Building> destination;
	// Time when cargo expires and is refunded
	uint64_t expirationDate = 0;
	// Warned player that cargo is about to expire
	bool warned = false;
	// Do not make events about this cargo, set if this is items left on floor
	bool suppressEvents = false;

	Cargo() = default;
	Cargo(GameState &state, StateRef<AEquipmentType> equipment, int count, int price,
	      StateRef<Organisation> originalOwner, StateRef<Building> destination);
	Cargo(GameState &state, StateRef<VEquipmentType> equipment, int count, int price,
	      StateRef<Organisation> originalOwner, StateRef<Building> destination);
	Cargo(GameState &state, StateRef<VAmmoType> equipment, int count, int price,
	      StateRef<Organisation> originalOwner, StateRef<Building> destination);
	Cargo(GameState &state, Type type, UString id, int count, int divisor, int space, int cost,
	      StateRef<Organisation> originalOwner, StateRef<Building> destination);

	// Check expiry date, expire if past expiration date, return true if expires soon
	bool checkExpiryDate(GameState &state, StateRef<Building> currentBuilding);
	// Refund cargo to destination org
	void refund(GameState &state, StateRef<Building> currentBuilding);
	// Put cargo into base
	void arrive(GameState &state);
	// Put cargo into base
	void arrive(GameState &state, bool &cargoArrived, bool &bioArrived, bool &recoveryArrived,
	            bool &transferArrived, std::set<StateRef<Organisation>> &suppliers);
	// Seize cargo
	void seize(GameState &state, StateRef<Organisation> org);
	// Clear cargo (set count to zero, will be removed)
	void clear();
};

class VehicleMover
{
  public:
	Vehicle &vehicle;
	VehicleMover(Vehicle &vehicle);
	virtual void update(GameState &state, unsigned int ticks) = 0;
	void updateFalling(GameState &state, unsigned int ticks);
	void updateSliding(GameState &state, unsigned int ticks);
	void updateCrashed(GameState &state, unsigned int ticks);
	virtual ~VehicleMover();
};

class Vehicle : public StateObject<Vehicle>,
                public std::enable_shared_from_this<Vehicle>,
                public EquippableObject
{
  public:
	~Vehicle() override;
	Vehicle() = default;

	enum class AttackMode
	{
		Aggressive,
		Standard,
		Defensive,
		Evasive
	};
	AttackMode attackMode = AttackMode::Standard;

	enum class Altitude
	{
		Highest = 12,
		High = 9,
		Standard = 6,
		Low = 3
	};
	Altitude altitude = Altitude::Standard;
	// Adjusts position by altitude preference
	Vec3<int> getPreferredPosition(Vec3<int> position) const;
	Vec3<int> getPreferredPosition(int x, int y, int z = 0) const;

	void equipDefaultEquipment(GameState &state);

	StateRef<VehicleType> type;
	StateRef<Organisation> owner;
	UString name;
	std::list<up<VehicleMission>> missions;
	std::list<sp<VEquipment>> equipment;
	std::list<StateRef<VEquipmentType>> loot;
	StateRef<City> city;
	Vec3<float> position = {0, 0, 0};
	Vec3<float> goalPosition = {0, 0, 0};
	std::list<Vec3<float>> goalWaypoints;
	Vec3<float> velocity = {0, 0, 0};
	float facing = 0.0f;
	float goalFacing = 0.0f;
	float angularVelocity = 0.0f;
	unsigned int ticksToTurn = 0;
	// Current banking, updated every time vehicle changes facing
	VehicleType::Banking banking = VehicleType::Banking::Flat;
	// Current vehicle direction, updated every time vehicle changes facing
	VehicleType::Direction direction = VehicleType::Direction::N;
	// Current shadow direction, updated every time vehicle changes facing
	VehicleType::Direction shadowDirection = VehicleType::Direction::N;
	int health = 0;
	int shield = 0;
	unsigned int shieldRecharge = 0;
	int stunTicksRemaining = 0;
	bool crashed = false;
	bool falling = false;
	bool sliding = false;
	int fuelSpentTicks = 0;
	// Cloak, increases each turn, set to 0 when firing or no cloaking device on vehicle
	// Vehicle is cloaked when this is >= CLOAK_TICKS_REQUIRED_VEHICLE
	unsigned int cloakTicksAccumulated = 0;

	unsigned int teleportTicksAccumulated = 0;

	uint64_t ticksAutoActionAvailable = 0;

	StateRef<Building> homeBuilding;
	// Building the vehicle is currently stored inside, nullptr if it's in the city
	StateRef<Building> currentBuilding;
	// Agents currently residing in vehicle
	std::set<StateRef<Agent>> currentAgents;
	// Cargo loaded on vehicle
	std::list<Cargo> cargo;

	StateRef<Vehicle> carriedVehicle;
	StateRef<Vehicle> carriedByVehicle;

	sp<TileObjectVehicle> tileObject;
	sp<TileObjectShadow> shadowObject;

	// If the vehicle is currently traveling through a dimension gate
	bool betweenDimensions = false;

	/* leave the building and put vehicle into the city */
	void leaveDimensionGate(GameState &state);
	/* 'enter' the vehicle into a building*/
	void enterDimensionGate(GameState &state);
	/* leave the building and put vehicle into the city */
	void leaveBuilding(GameState &state, Vec3<float> initialPosition, float initialFacing = 0.0f);
	/* 'enter' the vehicle into a building*/
	void enterBuilding(GameState &state, StateRef<Building> b);
	/* Sets up the 'mover' after state serialize in */
	void setupMover();
	// Remove all tile objects that belongs to vehicle.
	void removeFromMap(GameState &state);
	// Set the vehicle crashed (or not).
	void setCrashed(GameState &state, bool crashed = true);

	void processRecoveredVehicle(GameState &state);
	void dropCarriedVehicle(GameState &state);

	// Provide cargo or passenger service. Loads cargo or passengers or bio.
	// If otherOrg true - provides service to other orgs but only if type provides freight
	// If otherOrg false - provides service only to own org and ignores type flag
	void provideService(GameState &state, bool otherOrg);
	void provideServiceCargo(GameState &state, bool bio, bool otherOrg);
	void provideServicePassengers(GameState &state, bool otherOrg);
	// Get destination for current cargo or passengers, nullptr if none
	// Also offloads arrived cargo or passengers
	StateRef<Building> getServiceDestination(GameState &state);

	void die(GameState &state, bool silent = false, StateRef<Vehicle> attacker = nullptr);
	void crash(GameState &state, StateRef<Vehicle> attacker);
	void startFalling(GameState &state, StateRef<Vehicle> attacker = nullptr);
	void adjustRelationshipOnDowned(GameState &state, StateRef<Vehicle> attacker);
	bool isDead() const;

	bool canAddEquipment(Vec2<int> pos, StateRef<VEquipmentType> type) const;
	sp<VEquipment> addEquipment(GameState &state, Vec2<int> pos,
	                            StateRef<VEquipmentType> equipmentType);
	sp<VEquipment> addEquipment(GameState &state, StateRef<VEquipmentType> equipmentType);
	void removeEquipment(sp<VEquipment> object);

	bool applyDamage(GameState &state, int damage, float armour);
	bool applyDamage(GameState &state, int damage, float armour, bool &soundHandled,
	                 StateRef<Vehicle> attacker = nullptr);
	bool handleCollision(GameState &state, Collision &c, bool &soundHandled);
	sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile,
	                                       Vec2<int> arc = {8, 8});
	sp<TileObjectProjectile> findClosestHostileMissile(GameState &state,
	                                                   sp<TileObjectVehicle> vehicleTile,
	                                                   Vec2<int> arc = {8, 8});
	bool fireWeaponsPointDefense(GameState &state, Vec2<int> arc = {8, 8});

	bool fireAtBuilding(GameState &state, Vec2<int> arc = {8, 8});
	void fireWeaponsManual(GameState &state, Vec2<int> arc = {8, 8});
	bool attackTarget(GameState &state, sp<TileObjectVehicle> enemyTile);
	bool attackTarget(GameState &state, sp<TileObjectProjectile> enemyTile);
	bool attackTarget(GameState &state, Vec3<float> target);
	sp<VEquipment> getFirstFiringWeapon(GameState &state, Vec3<float> &target,
	                                    bool checkLOF = false,
	                                    Vec3<float> targetVelocity = {0.0f, 0.0f, 0.0f},
	                                    sp<TileObjectVehicle> enemyTile = nullptr, bool pd = false);
	float getFiringRange() const;

	Vec3<float> getMuzzleLocation() const;

	const Vec3<float> &getPosition() const { return this->position; }

	//'Constitution' is the sum of health and shield
	int getMaxConstitution() const;
	int getConstitution() const;

	int getMaxHealth() const;
	int getHealth() const;

	int getMaxShield() const;
	int getShield() const;
	int getShieldRechargeRate() const;

	bool isCloaked() const;
	bool hasCloak() const;
	bool canTeleport() const;
	bool hasTeleporter() const;
	bool hasDimensionShifter() const;
	bool isIdle() const;

	// This is the 'sum' of all armors?
	int getArmor() const;
	int getAccuracy() const;
	int getTopSpeed() const;
	int getAcceleration() const;
	int getWeight() const;
	int getMaxFuel() const;
	int getFuel() const;
	int getMaxPassengers() const;
	int getPassengers() const;
	int getMaxCargo() const;
	int getCargo() const;
	int getMaxBio() const;
	int getBio() const;
	float getSpeed() const;
	float getAngularSpeed() const;

	void nextFrame(int ticks);

	void setPosition(const Vec3<float> &pos);

	void setManualFirePosition(const Vec3<float> &pos);

	// Adds mission to list of missions, returns iterator to mission if successful, missions.end()
	// otherwise
	typename decltype(missions)::iterator addMission(GameState &state, VehicleMission *mission,
	                                                 bool toBack = false);
	// Replaces all missions with provided mission, returns true if successful
	bool setMission(GameState &state, VehicleMission *mission);
	bool clearMissions(GameState &state, bool forced = false);

	// Pops all finished missions, returns true if popped
	bool popFinishedMissions(GameState &state);
	// Get new goal for vehicle position or facing
	bool getNewGoal(GameState &state, int &turboTiles);

	void update(GameState &state, unsigned int ticks);
	void updateEachSecond(GameState &state);
	void updateCargo(GameState &state);
	void updateSprite(GameState &state);

	sp<VEquipment> getEngine() const;
	// Returns true if vehicle does not require an engine
	bool hasEngine() const;

	sp<Equipment> getEquipmentAt(const Vec2<int> &position) const override;
	const std::list<EquipmentLayoutSlot> &getSlots() const override;
	std::list<std::pair<Vec2<int>, sp<Equipment>>> getEquipment() const override;

	// Following members are not serialized, but rather setup during game

	up<VehicleMover> mover;
	sp<Doodad> smokeDoodad;
	std::list<sp<Image>>::iterator animationFrame;
	int animationDelay = 0;

	// Following members are not serialized, but rather are set in initCity method

	sp<std::vector<sp<Image>>> strategyImages;

	// Following members are not serialized, since there is no need

  private:
	Vec3<float> manualFirePosition = {0.0f, 0.0f, 0.0f};
	bool manualFire = false;
	std::list<sp<VEquipmentType>> getEquipmentTypes() const;
};

}; // namespace OpenApoc
