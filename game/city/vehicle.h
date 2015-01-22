#pragma once

#include "framework/includes.h"
#include "game/tileview/tile.h"

namespace OpenApoc {

class Image;
class VehicleFactory;
class VehicleDefinition;



class Vehicle
{
public:
	virtual ~Vehicle();
	Vehicle(VehicleDefinition &def);

	VehicleDefinition &def;

	enum class Type
	{
		Flying,
		Ground,
	};
	enum class Direction
	{
		N,
		NNE,
		NE,
		NEE,
		E,
		SEE,
		SE,
		SSE,
		S,
		SSW,
		SW,
		SWW,
		W,
		NWW,
		NW,
		NNW,
	};
	enum class Banking
	{
		Flat,
		Left,
		Right,
		Ascending,
		Decending,
	};

	std::shared_ptr<TileObject> tileObject;
};

class VehicleMission
{
public:
	Vehicle &vehicle;
	VehicleMission(Vehicle &vehicle);
	virtual Vec3<float> getNextDestination() = 0;
	virtual ~VehicleMission();
};

class VehicleMover
{
public:
	Vehicle &vehicle;
	VehicleMover(Vehicle &vehicle);
	virtual void update(unsigned int ticks) = 0;
	virtual ~VehicleMover();
};

class FlyingVehicle : public TileObject
{
public:
	Vehicle &vehicle;
	FlyingVehicle(Vehicle &vehicle, Tile *owningTile);
	std::unique_ptr<VehicleMission> mission;
	std::unique_ptr<VehicleMover> mover;
	Vec3<float> direction;
	virtual ~FlyingVehicle();
	virtual Image& getSprite();
	virtual void update(unsigned int ticks);
	virtual void processCollision(TileObject &otherObject);
};

}; //namespace OpenApoc
