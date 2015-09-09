#include "framework/logger.h"
#include "game/city/vehicle.h"
#include "game/city/weapon.h"
#include "game/tileview/projectile.h"
#include "game/organisation.h"
#include "framework/image.h"
#include <cfloat>
#include <random>
#include <limits>

std::default_random_engine rng;

namespace OpenApoc
{

class VehicleRandomWalk : public VehicleMission
{
  public:
	std::uniform_int_distribution<int> distribution;
	VehicleRandomWalk(Vehicle &vehicle) : VehicleMission(vehicle), distribution(-1, 1) {}
	std::list<Tile *> destination;
	virtual Vec3<float> getNextDestination() override
	{
		auto vehicleTile = this->vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogError("Calling on vehicle with no tile object?");
		}
		TileMap &map = vehicleTile->getOwningTile()->map;
		Vec3<int> nextPosition;
		int tries = 0;
		do
		{
			nextPosition = vehicleTile->getPosition();
			Vec3<int> diff{distribution(rng), distribution(rng), distribution(rng)};
			nextPosition += diff;
			// FIXME HACK - abort after some attempts (e.g. if we're completely trapped)
			// and just phase through whatever obstruction is there
			tries++;
			// Keep looping until we find an empty tile within the map
		} while (nextPosition.x >= map.size.x || nextPosition.x < 0 ||
		         nextPosition.y >= map.size.y || nextPosition.y < 0 ||
		         nextPosition.z >= map.size.z || nextPosition.z < 0
		         // FIXME: Proper routing/obstruction handling
		         //(This below could cause an infinite loop if a vehicle gets 'trapped'
		         || (tries < 50 && !map.getTile(nextPosition)->ownedObjects.empty()));
		destination = std::list<Tile *>{map.getTile(nextPosition)};
		return Vec3<float>{nextPosition.x, nextPosition.y, nextPosition.z};
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return destination; }
};

class VehicleRandomDestination : public VehicleMission
{
  public:
	std::uniform_int_distribution<int> xydistribution;
	std::uniform_int_distribution<int> zdistribution;
	VehicleRandomDestination(Vehicle &v)
	    : VehicleMission(v), xydistribution(0, 99), zdistribution(0, 9)
	{
	}
	std::list<Tile *> path;
	virtual Vec3<float> getNextDestination() override
	{
		auto vehicleTile = this->vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogError("Calling on vehicle with no tile object?");
		}
		while (path.empty())
		{
			Vec3<int> newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
			while (!vehicleTile->getOwningTile()->map.getTile(newTarget)->ownedObjects.empty())
				newTarget = {xydistribution(rng), xydistribution(rng), zdistribution(rng)};
			path = vehicleTile->getOwningTile()->map.findShortestPath(
			    vehicleTile->getOwningTile()->position, newTarget);
			if (path.empty())
			{
				LogInfo("Failed to path - retrying");
				continue;
			}
			// Skip first in the path (as that's current tile)
			path.pop_front();
		}
		if (!path.front()->ownedObjects.empty())
		{
			Vec3<int> target = path.back()->position;
			path = vehicleTile->getOwningTile()->map.findShortestPath(
			    vehicleTile->getOwningTile()->position, target);
			if (path.empty())
			{
				LogInfo("Failed to path after obstruction");
				path.clear();
				return this->getNextDestination();
			}
			// Skip first in the path (as that's current tile)
			path.pop_front();
		}
		Tile *nextTile = path.front();
		path.pop_front();
		return Vec3<float>{nextTile->position.x, nextTile->position.y, nextTile->position.z}
		       // Add {0.5,0.5,0.5} to make it route to the center of the tile
		       + Vec3<float>{0.5, 0.5, 0.5};
	}
	virtual const std::list<Tile *> &getCurrentPlannedPath() override { return path; }
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
		// Tweak the speed slightly, makes everything a little less synchronised
		speed = 0.05 + distribution(rng);
	}
	virtual void update(unsigned int ticks) override
	{
		auto vehicleTile = this->vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogError("Calling on vehicle with no tile object?");
		}
		float distanceLeft = speed * ticks;
		while (distanceLeft > 0)
		{
			Vec3<float> vectorToGoal = goalPosition - vehicleTile->getPosition();
			float distanceToGoal = glm::length(vectorToGoal);
			if (distanceToGoal <= distanceLeft)
			{
				distanceLeft -= distanceToGoal;
				vehicleTile->setPosition(goalPosition);
				goalPosition = vehicle.mission->getNextDestination();
			}
			else
			{
				vehicleTile->setDirection(vectorToGoal);
				vehicleTile->setPosition(vehicleTile->getPosition() +
				                         distanceLeft * glm::normalize(vectorToGoal));
				distanceLeft = -1;
			}
		}
		for (auto &weapon : vehicle.weapons)
		{
			weapon->update(ticks);
			if (weapon->canFire())
			{
				// Find something to shoot at!
				// FIXME: Only run on 'aggressive'? And not already a manually-selected target?
				float range = weapon->getWeaponDef().range;
				// Find the closest enemy within the firing arc
				float closestEnemyRange = std::numeric_limits<float>::max();
				std::shared_ptr<VehicleTileObject> closestEnemy;
				for (auto obj : vehicleTile->getOwningTile()->map.activeObjects)
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
					auto myPosition = vehicleTile->getPosition();
					auto otherVehicleTile = otherVehicle.tileObject.lock();
					if (!otherVehicleTile)
					{
						LogError("Firing on vehicle with no tile object?");
					}
					auto enemyPosition = otherVehicleTile->getPosition();
					// FIXME: Check weapon arc against otherVehicle
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
					// Only fire if we're in range
					auto projectile = weapon->fire(closestEnemy->getPosition());
					if (projectile)
					{
						vehicleTile->getOwningTile()->map.addObject(projectile);
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

VehicleMission::VehicleMission(Vehicle &v) : vehicle(v) {}

VehicleMission::~VehicleMission() {}

VehicleMover::VehicleMover(Vehicle &v) : vehicle(v) {}

VehicleMover::~VehicleMover() {}

Vehicle::Vehicle(const VehicleDefinition &def, Organisation &owner) : def(def), owner(owner) {}

Vehicle::~Vehicle() {}

void Vehicle::launch(TileMap &map, Vec3<float> initialPosition)
{
	if (this->tileObject.lock())
	{
		LogError("Trying to launch already-launched vehicle");
		return;
	}
	this->mover.reset(new FlyingVehicleMover(*this, initialPosition));
	this->mission.reset(new VehicleRandomDestination(*this));
	auto vehicleTile = std::make_shared<VehicleTileObject>(*this, map, initialPosition);
	this->tileObject = vehicleTile;
	map.addObject(vehicleTile);
}

VehicleTileObject::VehicleTileObject(Vehicle &vehicle, TileMap &map, Vec3<float> position)
    : TileObject(map, position),
      TileObjectDirectionalSprite(map, position, vehicle.def.directionalSprites),
      TileObjectCollidable(map, position, Vec3<int>{32, 32, 16}, vehicle.def.voxelMap),
      vehicle(vehicle)
{
	this->active = true;
	this->selectable = true;
}

VehicleTileObject::~VehicleTileObject() {}

void VehicleTileObject::update(unsigned int ticks)
{
	if (this->vehicle.mover)
		this->vehicle.mover->update(ticks);
}

Vec3<float> VehicleTileObject::getDrawPosition() const
{
	return this->getPosition() - Vec3<float>{2, 1, 1};
}

Rect<float> VehicleTileObject::getSelectableBounds() const
{
	auto spriteBounds = std::dynamic_pointer_cast<PaletteImage>(this->getSprite())->bounds;
	return Rect<float>{static_cast<float>(spriteBounds.p0.x), static_cast<float>(spriteBounds.p0.y),
	                   static_cast<float>(spriteBounds.p1.x),
	                   static_cast<float>(spriteBounds.p1.y)};
}

void VehicleTileObject::setSelected(bool selected)
{
	if (selected)
		LogWarning("Selected vehicle");
	else
		LogWarning("De-Selected vehicle");
}

}; // namespace OpenApoc
