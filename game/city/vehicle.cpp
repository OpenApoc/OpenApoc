#include "framework/logger.h"
#include "game/city/vehicle.h"
#include "game/resources/vehiclefactory.h"
#include <cfloat>
#include <random>

std::default_random_engine rng;

namespace OpenApoc {


class VehicleRandomWalk : public VehicleMission
{
public:
	std::uniform_int_distribution<int> distribution;
	VehicleRandomWalk(Vehicle &vehicle)
		: VehicleMission(vehicle), distribution(-1,1)
			{};
	virtual Vec3<float> getNextDestination()
	{
		TileMap &map = this->vehicle.tileObject->owningTile->map;
		Vec3<int> nextPosition;
		int tries = 0;
		do {
			nextPosition = {vehicle.position.x, vehicle.position.y, vehicle.position.z};
			Vec3<int> diff {distribution(rng), distribution(rng), distribution(rng)};
			nextPosition += diff;
			//FIXME HACK - abort after some attempts (e.g. if we're completely trapped)
			//and just phase through whatever obstruction is there
			tries++;
			//Keep looping until we find an empty tile within the map
		} while (nextPosition.x >= map.size.x || nextPosition.x < 0 ||
			nextPosition.y >= map.size.y || nextPosition.y < 0 ||
			nextPosition.z >= map.size.z || nextPosition.z < 0
			//FIXME: Proper routing/obstruction handling
			//(This below could cause an infinite loop if a vehicle gets 'trapped'
			|| (tries < 50 && !map.getTile(nextPosition)->objects.empty()));
		return Vec3<float>{nextPosition.x, nextPosition.y, nextPosition.z};
	}
};

class VehicleRandomDestination : public VehicleMission
{
public:
	std::uniform_int_distribution<int> xydistribution;
	std::uniform_int_distribution<int> zdistribution;
	VehicleRandomDestination(Vehicle &v)
		: VehicleMission(v), xydistribution(0,99), zdistribution(0,9)
			{};
	std::list<Tile*> path;
	virtual Vec3<float> getNextDestination()
	{
		while (path.empty())
		{
			Vec3<int> newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
			while (!vehicle.tileObject->owningTile->map.getTile(newTarget)->objects.empty())
				newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
			path = vehicle.tileObject->owningTile->map.findShortestPath(vehicle.tileObject->owningTile->position, newTarget);
			if (path.empty())
			{
				LogInfo("Failed to path - retrying");
				continue;
			}
			//Skip first in the path (as that's current tile)
			path.pop_front();
		}
		if (!path.front()->objects.empty())
		{
			Vec3<int> target = path.back()->position;
			path = vehicle.tileObject->owningTile->map.findShortestPath(vehicle.tileObject->owningTile->position, target);
			if (path.empty())
			{
				LogInfo("Failed to path after obstruction");
				path.clear();
				return this->getNextDestination();
			}
			//Skip first in the path (as that's current tile)
			path.pop_front();
		}
		Tile *nextTile = path.front();
		path.pop_front();
		return Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z};
	}

};


class FlyingVehicleMover : public VehicleMover
{
public:
	Vec3<float> goalPosition;
	float speed;
	FlyingVehicleMover(Vehicle &v, Vec3<float> initialGoal)
		: VehicleMover(v), goalPosition(initialGoal)
	{
		std::uniform_real_distribution<float> distribution(-0.02, 0.02);
		//Tweak the speed slightly, makes everything a little less synchronised
		speed = 0.05 + distribution(rng);
	}
	virtual void update(unsigned int ticks)
	{
		float distanceLeft = speed * ticks;
		while (distanceLeft > 0)
		{
			Vec3<float> vectorToGoal = goalPosition -
				vehicle.position;
			float distanceToGoal = glm::length(vectorToGoal);
			if (distanceToGoal <= distanceLeft)
			{
				distanceLeft -= distanceToGoal;
				vehicle.position = goalPosition;
				goalPosition = vehicle.mission->getNextDestination();
			}
			else
			{
				vehicle.direction = vectorToGoal;
				vehicle.position += distanceLeft * glm::normalize(vectorToGoal);
				distanceLeft = -1;
			}
		}
	}
};

VehicleMission::VehicleMission(Vehicle &v)
	: vehicle(v)
{

}

VehicleMission::~VehicleMission()
{

}

VehicleMover::VehicleMover(Vehicle &v)
: vehicle(v)
{

}

VehicleMover::~VehicleMover()
{

}

Vehicle::Vehicle(VehicleDefinition &def, Organisation &owner)
	: def(def), owner(owner)
{

}

Vehicle::~Vehicle()
{

}

void
Vehicle::launch(TileMap &map, Vec3<float> initialPosition)
{
	if (this->tileObject)
	{
		LogError("Trying to launch already-launched vehicle");
		return;
	}
	this->position = initialPosition;
	this->mover.reset(new FlyingVehicleMover(*this, initialPosition));
	this->mission.reset(new VehicleRandomDestination(*this));
	this->tileObject = std::make_shared<VehicleTileObject>(*this, map, initialPosition);
	map.addObject(this->tileObject);
}

VehicleTileObject::VehicleTileObject(Vehicle &vehicle, TileMap &map, Vec3<float> position)
	: TileObjectDirectionalSprite(map, vehicle.def.directionalSprites, position, Vec3<float>{0,1,0}), vehicle(vehicle)
{
}

VehicleTileObject::~VehicleTileObject()
{
}

void
VehicleTileObject::update(unsigned int ticks)
{
	if (this->vehicle.mover)
		this->vehicle.mover->update(ticks);
	//FIXME: Better way instead of mirroring pos/direction?
	this->setPosition(this->vehicle.position);
	this->setDirection(this->vehicle.direction);
}


}; //namespace OpenApoc
