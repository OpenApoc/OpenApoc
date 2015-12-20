#pragma once

#include "framework/includes.h"
#include "game/tileview/tile.h"
#include "game/rules/vehicle_type.h"

#include <deque>
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

class VehicleMover
{
  public:
	Vehicle &vehicle;
	VehicleMover(Vehicle &vehicle);
	virtual void update(unsigned int ticks) = 0;
	virtual ~VehicleMover();
};

class Vehicle : public std::enable_shared_from_this<Vehicle>
{
  public:
	virtual ~Vehicle();
	// An empty name will generate a name from type.name and type.numCreated
	Vehicle(const VehicleType &type, sp<Organisation> owner, UString name = "");

	void equipDefaultEquipment(Rules &rules);

	const VehicleType &type;
	sp<Organisation> owner;

	UString name;

	sp<TileObjectVehicle> tileObject;
	sp<TileObjectShadow> shadowObject;

	std::deque<std::unique_ptr<VehicleMission>> missions;
	std::unique_ptr<VehicleMover> mover;

	/* The building we are current landed in (May be nullptr if in the air) */
	std::weak_ptr<Building> building;

	/* 'launch' the vehicle into the city */
	void launch(TileMap &map, Vec3<float> initialPosition);
	/* 'land' the vehicle in a building*/
	void land(TileMap &map, sp<Building> b);

	std::list<sp<VEquipment>> equipment;
	Vec3<float> position;

	const Vec3<float> &getPosition() const { return this->position; }
	const Vec3<float> &getDirection() const;

	int getConstitution() const;
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

	virtual void update(Framework &fw, GameState &state, unsigned int ticks);
};

}; // namespace OpenApoc
