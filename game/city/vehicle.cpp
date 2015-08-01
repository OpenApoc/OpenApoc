#include "framework/logger.h"
#include "game/city/vehicle.h"
#include "game/city/weapon.h"
#include "game/tileview/projectile.h"
#include "game/organisation.h"
#include <cfloat>
#include <random>
#include <limits>

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
		TileMap &map = this->vehicle.tileObject->getOwningTile()->map;
		Vec3<int> nextPosition;
		int tries = 0;
		do {
			nextPosition = vehicle.tileObject->getPosition();
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
			|| (tries < 50 && !map.getTile(nextPosition)->ownedObjects.empty()));
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
			while (!vehicle.tileObject->getOwningTile()->map.getTile(newTarget)->ownedObjects.empty())
				newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
			path = vehicle.tileObject->getOwningTile()->map.findShortestPath(vehicle.tileObject->getOwningTile()->position, newTarget);
			if (path.empty())
			{
				LogInfo("Failed to path - retrying");
				continue;
			}
			//Skip first in the path (as that's current tile)
			path.pop_front();
		}
		if (!path.front()->ownedObjects.empty())
		{
			Vec3<int> target = path.back()->position;
			path = vehicle.tileObject->getOwningTile()->map.findShortestPath(vehicle.tileObject->getOwningTile()->position, target);
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
		return Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z}
			//Add {0.5,0.5,0.5} to make it route to the center of the tile
			+ Vec3<float>{0.5,0.5,0.5};
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
				vehicle.tileObject->getPosition();
			float distanceToGoal = glm::length(vectorToGoal);
			if (distanceToGoal <= distanceLeft)
			{
				distanceLeft -= distanceToGoal;
				vehicle.tileObject->setPosition(goalPosition);
				goalPosition = vehicle.mission->getNextDestination();
			}
			else
			{
				vehicle.tileObject->setDirection(vectorToGoal);
				vehicle.tileObject->setPosition(vehicle.tileObject->getPosition() + distanceLeft * glm::normalize(vectorToGoal));
				distanceLeft = -1;
			}
		}
		for (auto &weapon : vehicle.weapons)
		{
			weapon->update(ticks);
			if (weapon->canFire())
			{
				//Find something to shoot at!
				//FIXME: Only run on 'aggressive'? And not already a manually-selected target?
				float range = weapon->getWeaponDef().range;
				//Find the closest enemy within the firing arc
				float closestEnemyRange = std::numeric_limits<float>::max();
				std::shared_ptr<VehicleTileObject> closestEnemy;
				for (auto obj : vehicle.tileObject->getOwningTile()->map.activeObjects)
				{
					auto vehicleObj = std::dynamic_pointer_cast<VehicleTileObject>(obj);
					if (!vehicleObj)
					{
						/* Not a vehicle, skip */
						continue;
					}
					auto &otherVehicle = vehicleObj->getVehicle();
					if (!vehicle.owner.isHostileTo(otherVehicle.owner))
					{
						/* Not hostile, skip */
						continue;
					}
					auto myPosition = vehicle.tileObject->getPosition();
					auto enemyPosition = otherVehicle.tileObject->getPosition();
					//FIXME: Check weapon arc against otherVehicle
					auto offset = enemyPosition - myPosition;
					float distance = glm::length(offset);

					if (distance < closestEnemyRange)
					{
						closestEnemyRange = distance;
						closestEnemy = vehicleObj;
					}
				}

				if (closestEnemyRange <= range)
				{
					//Only fire if we're in range
					auto projectile = weapon->fire(closestEnemy->getPosition());
					if (projectile)
					{
						vehicle.tileObject->getOwningTile()->map.addObject(projectile);
					}
					else
					{
						LogWarning("Fire() produced no object");
					}
				}
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

Vehicle::Vehicle(const VehicleDefinition &def, Organisation &owner)
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
	this->mover.reset(new FlyingVehicleMover(*this, initialPosition));
	this->mission.reset(new VehicleRandomDestination(*this));
	this->tileObject = std::make_shared<VehicleTileObject>(*this, map, initialPosition);
	map.addObject(this->tileObject);
}

VehicleTileObject::VehicleTileObject(Vehicle &vehicle, TileMap &map, Vec3<float> position)
	: TileObject(map, position),
	TileObjectDirectionalSprite(map, position, vehicle.def.directionalSprites),
	TileObjectCollidable(map, position, Vec3<int>{32,32,16}, vehicle.def.voxelMap),
	vehicle(vehicle)
{
	this->active = true;
}

VehicleTileObject::~VehicleTileObject()
{
}

void
VehicleTileObject::update(unsigned int ticks)
{
	if (this->vehicle.mover)
		this->vehicle.mover->update(ticks);
}

Vec3<float> VehicleTileObject::getDrawPosition() const
{
	return this->getPosition() - Vec3<float>{2,1,1};
}



}; //namespace OpenApoc
