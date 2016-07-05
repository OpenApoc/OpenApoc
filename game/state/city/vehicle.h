#pragma once

#include "framework/includes.h"
#include "game/state/organisation.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/tileview/tile.h"
#include <list>

namespace OpenApoc
{

class Image;
class TileObjectVehicle;
class TileObjectShadow;
class Vehicle;
class Organisation;
class Weapon;
class VehicleMission;
class Building;
class GameState;
class VEquipment;
class VEquipmentType;
class Base;
class City;

class VehicleMover
{
  public:
	Vehicle &vehicle;
	VehicleMover(Vehicle &vehicle);
	virtual void update(GameState &state, unsigned int ticks) = 0;
	virtual ~VehicleMover();
};

class Vehicle : public StateObject<Vehicle>, public std::enable_shared_from_this<Vehicle>
{
  public:
	virtual ~Vehicle();
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
		Highest = 11,
		High = 8,
		Standard = 5,
		Low = 2
	};
	Altitude altitude;

	void equipDefaultEquipment(GameState &state);

	StateRef<VehicleType> type;
	StateRef<Organisation> owner;
	UString name;
	std::list<up<VehicleMission>> missions;
	std::list<sp<VEquipment>> equipment;
	Vec3<float> position;
	Vec3<float> velocity;
	Vec3<float> facing;
	StateRef<City> city;
	int health;
	int shield;
	StateRef<Building> homeBuilding;
	StateRef<Building> currentlyLandedBuilding;

	sp<TileObjectVehicle> tileObject;
	sp<TileObjectShadow> shadowObject;

	std::unique_ptr<VehicleMover> mover;

	/* 'launch' the vehicle into the city */
	void launch(TileMap &map, GameState &state, Vec3<float> initialPosition);
	/* 'land' the vehicle in a building*/
	void land(GameState &state, StateRef<Building> b);
	/* Sets up the 'mover' after state serialize in */
	void setupMover();

	bool canAddEquipment(Vec2<int> pos, StateRef<VEquipmentType> type) const;
	void addEquipment(GameState &state, Vec2<int> pos, StateRef<VEquipmentType> type);
	void removeEquipment(sp<VEquipment> object);

	bool applyDamage(int damage, float armour);
	void handleCollision(GameState &state, Collision &c);
	sp<TileObjectVehicle> findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile);
	void attackTarget(sp<TileObjectVehicle> vehicleTile, sp<TileObjectVehicle> enemyTile);
	float getFiringRange() const;

	const Vec3<float> &getPosition() const { return this->position; }
	const Vec3<float> &getDirection() const;

	//'Constitution' is the sum of health and shield
	int getMaxConstitution() const;
	int getConstitution() const;

	int getMaxHealth() const;
	int getHealth() const;

	int getMaxShield() const;
	int getShield() const;

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
};

}; // namespace OpenApoc
