#pragma once

#include "game/state/equipment.h"
#include "game/state/gametime.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/stateobject.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <list>
#include <map>

// Uncomment to allow projectiles to shoot down friendly projectiles
#define ALLOW_PROJECTILE_ON_PROJECTILE_FRIENDLY_FIRE

namespace OpenApoc
{

static const unsigned CLOAK_TICKS_REQUIRED_VEHICLE = TICKS_PER_SECOND * 3 / 2;
static const unsigned TELEPORT_TICKS_REQUIRED_VEHICLE = TICKS_PER_SECOND * 30;
static const unsigned TICKS_AUTO_ACTION_DELAY = TICKS_PER_SECOND / 4;

class Image;
class TileObjectVehicle;
class TileObjectShadow;
class Vehicle;
class Organisation;
class VehicleMission;
class Building;
class GameState;
class TileObjectProjectile;
class VEquipment;
class VEquipmentType;
class City;
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
	int multiplier = 0;
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

	// Check expiry date, expire if past expiration date, return true if expires soon
	bool checkExpiryDate(GameState &state);
	// Refund cargo to destination org
	void refund(GameState &state);
	// Put cargo into base
	void arrive(GameState &state);
	// Put cargo into base
	void arrive(GameState &state, bool &cargoArrived, bool &bioArrived, bool &recoveryArrived, bool &transferArrived, std::set<StateRef<Organisation>> &suppliers);
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
	virtual ~VehicleMover();
};

class Vehicle : public StateObject,
                public std::enable_shared_from_this<Vehicle>,
                public EquippableObject
{
	STATE_OBJECT(Vehicle)
  public:
	~Vehicle() override;
	Vehicle();

	enum class AttackMode
	{
		Aggressive,
		Standard,
		Defensive,
		Evasive
	};
	AttackMode attackMode;

	enum class Altitude
	{
		Highest = 12,
		High = 9,
		Standard = 6,
		Low = 3
	};
	Altitude altitude;

	void equipDefaultEquipment(GameState &state);

	StateRef<VehicleType> type;
	StateRef<Organisation> owner;
	UString name;
	std::list<up<VehicleMission>> missions;
	std::list<sp<VEquipment>> equipment;
	StateRef<City> city;
	Vec3<float> position;
	Vec3<float> goalPosition;
	std::list<Vec3<float>> goalWaypoints;
	Vec3<float> velocity;
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
	std::list<Cargo> cargo;

	sp<TileObjectVehicle> tileObject;
	sp<TileObjectShadow> shadowObject;

	up<VehicleMover> mover;

	/* leave the building and put vehicle into the city */
	void leaveBuilding(GameState &state, Vec3<float> initialPosition, float initialFacing = 0.0f);
	/* 'enter' the vehicle into a building*/
	void enterBuilding(GameState &state, StateRef<Building> b);
	/* Sets up the 'mover' after state serialize in */
	void setupMover();

	// Provide cargo or passenger service. Loads cargo or passengers or bio.
	// If otherOrg true - provides service to other orgs but only if type provides freight
	// If otherOrg false - provides service only to own org and ignores type flag
	void provideService(GameState &state, bool otherOrg);
	void provideServiceCargo(GameState &state, bool bio, bool otherOrg);
	void provideServicePassengers(GameState &state, bool otherOrg);
	// Get destination for current cargo or passengers, nullptr if none
	// Also offloads arrived cargo or passengers
	StateRef<Building> getServiceDestination(GameState &state);

	void die(GameState &state, StateRef<Vehicle> attacker = nullptr, bool silent = false);
	void crash(GameState &state, StateRef<Vehicle> attacker);
	void adjustRelationshipOnDowned(GameState &state, StateRef<Vehicle> attacker);
	bool isDead() const;

	bool canAddEquipment(Vec2<int> pos, StateRef<VEquipmentType> type) const;
	void addEquipment(GameState &state, Vec2<int> pos, StateRef<VEquipmentType> type);
	void removeEquipment(sp<VEquipment> object);

	bool isCrashed() const;
	bool applyDamage(GameState &state, int damage, float armour,
	                 StateRef<Vehicle> attacker = nullptr);
	bool handleCollision(GameState &state, Collision &c);
	sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile,
	                                       Vec2<int> arc = {8, 8});
	sp<TileObjectProjectile> findClosestHostileMissile(GameState &state,
	                                                   sp<TileObjectVehicle> vehicleTile,
	                                                   Vec2<int> arc = {8, 8});
	bool firePointDefenseWeapons(GameState &state, Vec2<int> arc = {8, 8});
	void fireNormalWeapons(GameState &state, Vec2<int> arc = {8, 8});
	void attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile,
	                  sp<TileObjectVehicle> enemyTile);
	bool attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile,
	                  sp<TileObjectProjectile> enemyTile);
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

	// This is the 'sum' of all armors?
	int getArmor() const;
	int getAccuracy() const;
	int getTopSpeed() const;
	int getAcceleration() const;
	int getWeight() const;
	int getFuel() const;
	int getMaxPassengers() const;
	int getPassengers() const;
	int getMaxCargo() const;
	int getCargo() const;
	int getMaxBio() const;
	int getBio() const;
	float getSpeed() const;

	void nextFrame(int ticks);

	void setPosition(const Vec3<float> &pos);

	// Adds mission to list of missions, returns true if successful
	bool addMission(GameState &state, VehicleMission *mission, bool toBack = false);
	// Replaces all missions with provided mission, returns true if successful
	bool setMission(GameState &state, VehicleMission *mission);

	void update(GameState &state, unsigned int ticks);
	void updateCargo(GameState &state);
	void updateSprite(GameState &state);

	sp<Equipment> getEquipmentAt(const Vec2<int> &position) const override;
	const std::list<EquipmentLayoutSlot> &getSlots() const override;
	std::list<std::pair<Vec2<int>, sp<Equipment>>> getEquipment() const override;

	// Following members are not serialized, but rather setup when vehicle moves into the city
	std::list<sp<Image>>::iterator animationFrame;
	int animationDelay;

	// Following members are not serialized, but rather are set in initCity method

	sp<std::vector<sp<Image>>> strategyImages;
};

}; // namespace OpenApoc
