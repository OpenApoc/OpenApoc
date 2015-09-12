#include "framework/logger.h"
#include "game/city/vehicle.h"
#include "game/city/weapon.h"
#include "game/tileview/projectile.h"
#include "game/organisation.h"
#include "framework/image.h"
#include "game/city/vehicle.h"
#include "game/city/vehiclemission.h"

#include <cfloat>
#include <random>
#include <limits>

namespace
{
std::default_random_engine speed_rng;
} // anonymous namespace

namespace OpenApoc
{

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
		speed = 0.05 + distribution(speed_rng);
	}
	virtual void update(unsigned int ticks) override
	{
		auto vehicleTile = this->vehicle.tileObject.lock();
		if (!vehicleTile)
		{
			LogError("Calling on vehicle with no tile object?");
		}
		float distanceLeft = speed * ticks;
		if (!vehicle.missions.empty())
		{
			vehicle.missions.front()->update(ticks);
			while (distanceLeft > 0)
			{
				Vec3<float> vectorToGoal = goalPosition - vehicleTile->getPosition();
				float distanceToGoal = glm::length(vectorToGoal);
				if (distanceToGoal <= distanceLeft)
				{
					distanceLeft -= distanceToGoal;
					vehicleTile->setPosition(goalPosition);
					if (vehicle.missions.front()->isFinished())
					{
						vehicle.missions.pop_front();
						if (!vehicle.missions.empty())
						{
							vehicle.missions.front()->start();
						}
						else
						{
							distanceLeft = 0;
							break;
						}
					}
					goalPosition = vehicle.missions.front()->getNextDestination();
				}
				else
				{
					vehicleTile->setDirection(vectorToGoal);
					vehicleTile->setPosition(vehicleTile->getPosition() +
					                         distanceLeft * glm::normalize(vectorToGoal));
					distanceLeft = -1;
				}
			}
		}
	}
};

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
	this->missions.clear();
	auto vehicleTile = std::make_shared<VehicleTileObject>(*this, map, initialPosition);
	this->tileObject = vehicleTile;
	map.addObject(vehicleTile);
	map.activeObjects.insert(std::dynamic_pointer_cast<ActiveObject>(shared_from_this()));
}

VehicleTileObject::VehicleTileObject(Vehicle &vehicle, TileMap &map, Vec3<float> position)
    : TileObject(map, position),
      TileObjectDirectionalSprite(map, position, vehicle.def.directionalSprites),
      TileObjectCollidable(map, position, Vec3<int>{32, 32, 16}, vehicle.def.voxelMap),
      vehicle(vehicle)
{
	this->selectable = true;
}

void Vehicle::update(unsigned int ticks)

{
	if (!this->missions.empty())
		this->missions.front()->update(ticks);
	if (this->mover)
		this->mover->update(ticks);
	auto vehicleTile = this->tileObject.lock();
	if (vehicleTile)
	{
		for (auto &weapon : this->weapons)
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
					auto otherVehicle = std::dynamic_pointer_cast<Vehicle>(obj);
					if (!otherVehicle)
					{
						/* Not a vehicle, skip */
						continue;
					}
					if (!this->owner.isHostileTo(otherVehicle->owner))
					{
						/* Not hostile, skip */
						continue;
					}
					auto myPosition = vehicleTile->getPosition();
					auto otherVehicleTile = otherVehicle->tileObject.lock();
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
						closestEnemy = otherVehicleTile;
					}
				}

				if (closestEnemyRange <= range)
				{
					// Only fire if we're in range
					auto projectile = weapon->fire(closestEnemy->getPosition());
					if (projectile)
					{
						vehicleTile->getOwningTile()->map.addObject(projectile);
						vehicleTile->getOwningTile()->map.activeObjects.insert(
						    std::dynamic_pointer_cast<ActiveObject>(projectile));
					}
					else
					{
						LogWarning("Fire() produced no object");
					}
				}
			}
		}
	}
}

VehicleTileObject::~VehicleTileObject() {}

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
