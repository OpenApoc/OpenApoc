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

namespace OpenApoc
{

static const unsigned CLOAK_TICKS_REQUIRED_VEHICLE = TICKS_PER_SECOND / 2;
static const unsigned TICKS_AUTO_ACTION_DELAY = TICKS_PER_SECOND / 4;

class Image;
class TileObjectVehicle;
class TileObjectShadow;
class Vehicle;
class Organisation;
class VehicleMission;
class Building;
class GameState;
class VEquipment;
class VEquipmentType;
class Base;
class City;
class TileMap;
class Collision;

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

	uint64_t ticksAutoActionAvailable = 0;

	StateRef<Building> homeBuilding;
	StateRef<Building> currentlyLandedBuilding;

	sp<TileObjectVehicle> tileObject;
	sp<TileObjectShadow> shadowObject;

	up<VehicleMover> mover;

	/* 'launch' the vehicle into the city */
	void launch(TileMap &map, GameState &state, Vec3<float> initialPosition);
	/* 'land' the vehicle in a building*/
	void land(GameState &state, StateRef<Building> b);
	/* Sets up the 'mover' after state serialize in */
	void setupMover();

	bool canAddEquipment(Vec2<int> pos, StateRef<VEquipmentType> type) const;
	void addEquipment(GameState &state, Vec2<int> pos, StateRef<VEquipmentType> type);
	void removeEquipment(sp<VEquipment> object);

	bool isCrashed() const;
	bool applyDamage(GameState &state, int damage, float armour);
	void handleCollision(GameState &state, Collision &c);
	sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile,
	                                       Vec2<int> arc = {8, 8});
	void attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile,
	                  sp<TileObjectVehicle> enemyTile);
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
	float getSpeed() const;

	void setPosition(const Vec3<float> &pos);

	virtual void update(GameState &state, unsigned int ticks);
	void updateSprite(GameState &state);

	sp<Equipment> getEquipmentAt(const Vec2<int> &position) const override;
	const std::list<EquipmentLayoutSlot> &getSlots() const override;
	std::list<std::pair<Vec2<int>, sp<Equipment>>> getEquipment() const override;

	// Following members are not serialized, but rather are set in initCity method

	sp<std::vector<sp<Image>>> strategyImages;
};

}; // namespace OpenApoc
