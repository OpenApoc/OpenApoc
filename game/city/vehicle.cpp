#include "library/sp.h"
#include "framework/logger.h"
#include "game/city/vehicle.h"
#include "game/city/weapon.h"
#include "game/city/projectile.h"
#include "game/organisation.h"
#include "framework/image.h"
#include "game/city/city.h"
#include "game/city/building.h"
#include "game/city/vehiclemission.h"
#include "game/gamestate.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_shadow.h"

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

		if (!vehicle.missions.empty())
		{
			vehicle.missions.front()->update(ticks);
			auto vehicleTile = this->vehicle.tileObject;
			if (!vehicleTile)
			{
				return;
			}
			float distanceLeft = speed * ticks;
			while (distanceLeft > 0)
			{
				Vec3<float> vectorToGoal = goalPosition - vehicleTile->getPosition();
				float distanceToGoal = glm::length(vectorToGoal);
				if (distanceToGoal <= distanceLeft)
				{
					distanceLeft -= distanceToGoal;
					vehicle.setPosition(goalPosition);
					auto dir = glm::normalize(vectorToGoal);
					if (dir.z >= 0.9f || dir.z <= -0.9f)
					{
						dir = vehicleTile->getDirection();
						dir.z = 0;
						dir = glm::normalize(vectorToGoal);
					}
					vehicleTile->setDirection(dir);
					while (vehicle.missions.front()->isFinished())
					{
						LogInfo("Vehicle mission \"%s\" finished",
						        vehicle.missions.front()->getName().c_str());
						vehicle.missions.pop_front();
						if (!vehicle.missions.empty())
						{
							LogInfo("Vehicle mission \"%s\" starting",
							        vehicle.missions.front()->getName().c_str());
							vehicle.missions.front()->start();
							continue;
						}
						else
						{
							LogInfo("No next vehicle mission, going idle");
							break;
						}
					}
					if (vehicle.missions.empty() ||
					    vehicle.missions.front()->getNextDestination(goalPosition) == false)
					{
						distanceLeft = 0;
						break;
					}
				}
				else
				{
					// If we're going straight up/down  use the horizontal version of the last
					// direction
					// instead
					auto dir = glm::normalize(vectorToGoal);
					if (dir.z >= 0.9f || dir.z <= -0.9f)
					{
						dir = vehicleTile->getDirection();
						dir.z = 0;
						dir = glm::normalize(vectorToGoal);
					}
					vehicleTile->setDirection(dir);
					vehicle.setPosition(vehicleTile->getPosition() +
					                    distanceLeft * glm::normalize(vectorToGoal));
					distanceLeft = 0;
					break;
				}
			}
		}
	}
};

VehicleMover::VehicleMover(Vehicle &v) : vehicle(v) {}

VehicleMover::~VehicleMover() {}

Vehicle::Vehicle(const VehicleDefinition &def, sp<Organisation> owner) : def(def), owner(owner) {}

Vehicle::~Vehicle() {}

void Vehicle::launch(TileMap &map, Vec3<float> initialPosition)
{
	if (this->tileObject)
	{
		LogError("Trying to launch already-launched vehicle");
		return;
	}
	auto bld = this->building.lock();
	if (!bld)
	{
		LogError("Vehicle not in a building?");
		return;
	}
	this->position = initialPosition;
	bld->landed_vehicles.erase(shared_from_this());
	this->building.reset();
	this->mover.reset(new FlyingVehicleMover(*this, initialPosition));
	map.addObjectToMap(shared_from_this());
}

void Vehicle::land(TileMap &map, sp<Building> b)
{
	std::ignore = map;
	auto vehicleTile = this->tileObject;
	if (!vehicleTile)
	{
		LogError("Trying to land already-landed vehicle");
		return;
	}
	if (this->building.lock())
	{
		LogError("Vehicle already in a building?");
		return;
	}
	this->building = b;
	b->landed_vehicles.insert(shared_from_this());
	this->tileObject->removeFromMap();
	this->tileObject.reset();
	this->shadowObject->removeFromMap();
	this->shadowObject = nullptr;
	this->position = {0, 0, 0};
}

void Vehicle::update(GameState &state, unsigned int ticks)

{
	if (!this->missions.empty())
		this->missions.front()->update(ticks);
	if (this->mover)
		this->mover->update(ticks);
	auto vehicleTile = this->tileObject;
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
				sp<TileObjectVehicle> closestEnemy;
				for (auto otherVehicle : state.city->vehicles)
				{
					if (otherVehicle.get() == this)
					{
						/* Can't fire at yourself */
						continue;
					}
					if (!this->owner->isHostileTo(*otherVehicle->owner))
					{
						/* Not hostile, skip */
						continue;
					}
					auto myPosition = vehicleTile->getPosition();
					auto otherVehicleTile = otherVehicle->tileObject;
					if (!otherVehicleTile)
					{
						/* Not in the map, ignore */
						continue;
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
					// and fire at the center of the tile
					auto target = closestEnemy->getPosition();
					target += Vec3<float>{0.5, 0.5, 0.5};
					auto projectile = weapon->fire(target);
					if (projectile)
					{
						vehicleTile->map.addObjectToMap(projectile);
						state.city->projectiles.insert(projectile);
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

const Vec3<float> &Vehicle::getDirection() const
{
	static const Vec3<float> noDirection = {1, 0, 0};
	if (!this->tileObject)
	{
		LogError("getDirection() called on vehicle with no tile object");
		return noDirection;
	}
	return this->tileObject->getDirection();
}

void Vehicle::setPosition(const Vec3<float> &pos)
{
	if (!this->tileObject)
	{
		LogError("setPosition called on vehicle with no tile object");
	}
	else
	{
		this->tileObject->setPosition(pos);
	}

	if (!this->shadowObject)
	{
		LogError("setPosition called on vehicle with no shadow object");
	}
	else
	{
		this->shadowObject->setPosition(pos);
	}
}

}; // namespace OpenApoc
