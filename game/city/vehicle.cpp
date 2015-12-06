#include "library/sp.h"
#include "framework/logger.h"
#include "game/city/vehicle.h"
#include "game/city/vequipment.h"
#include "game/rules/rules.h"
#include "game/city/projectile.h"
#include "game/organisation.h"
#include "framework/image.h"
#include "game/city/city.h"
#include "game/city/building.h"
#include "game/city/vehiclemission.h"
#include "game/city/vequipment.h"
#include "game/rules/vequipment.h"
#include "game/gamestate.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_shadow.h"

#include <cfloat>
#include <random>
#include <limits>

namespace OpenApoc
{

class FlyingVehicleMover : public VehicleMover
{
  public:
	Vec3<float> goalPosition;
	FlyingVehicleMover(Vehicle &v, Vec3<float> initialGoal)
	    : VehicleMover(v), goalPosition(initialGoal)
	{
	}
	virtual void update(unsigned int ticks) override
	{
		float speed = vehicle.getSpeed();
		if (!vehicle.missions.empty())
		{
			vehicle.missions.front()->update(ticks);
			auto vehicleTile = this->vehicle.tileObject;
			if (!vehicleTile)
			{
				return;
			}
			float distanceLeft = speed * ticks;
			distanceLeft /= TICK_SCALE;
			while (distanceLeft > 0)
			{
				Vec3<float> vectorToGoal = goalPosition - vehicleTile->getPosition();
				float distanceToGoal = glm::length(vectorToGoal * VELOCITY_SCALE);
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
					Vec3<float> newPosition = distanceLeft * dir;
					newPosition /= VELOCITY_SCALE;
					newPosition += vehicleTile->getPosition();
					vehicle.setPosition(newPosition);
					distanceLeft = 0;
					break;
				}
			}
		}
	}
};

VehicleMover::VehicleMover(Vehicle &v) : vehicle(v) {}

VehicleMover::~VehicleMover() {}

Vehicle::Vehicle(const VehicleType &type, sp<Organisation> owner) : type(type), owner(owner) {}

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

void Vehicle::update(Framework &fw, GameState &state, unsigned int ticks)

{
	if (!this->missions.empty())
		this->missions.front()->update(ticks);
	if (this->mover)
		this->mover->update(ticks);
	auto vehicleTile = this->tileObject;
	if (vehicleTile)
	{
		for (auto &equipment : this->equipment)
		{
			if (equipment->type.type != VEquipmentType::Type::Weapon)
				continue;
			auto weapon = std::dynamic_pointer_cast<VWeapon>(equipment);
			weapon->update(ticks);
			if (weapon->canFire())
			{
				// Find something to shoot at!
				// FIXME: Only run on 'aggressive'? And not already a manually-selected target?
				float range = weapon->getRange();
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
					auto projectile = weapon->fire(fw, target);
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

float Vehicle::getSpeed() const
{
	// FIXME: This is somehow modulated by weight?
	float speed = this->type.top_speed;

	for (auto &e : this->equipment)
	{
		if (e->type.type != VEquipmentType::Type::Engine)
			continue;
		auto engine = std::dynamic_pointer_cast<VEngine>(e);
		auto &engineType = static_cast<const VEngineType &>(engine->type);
		speed += engineType.top_speed;
	}

	if (speed == 0)
	{
		LogError("Vehicle with no engine");
	}
	return speed;
}

void Vehicle::equipDefaultEquipment(Rules &rules)
{
	LogInfo("Equipping \"%s\" with default equipment", this->type.name.c_str());
	for (auto &pair : this->type.initial_equipment_list)
	{
		auto &pos = pair.first;
		auto &ename = pair.second;

		auto &etype = rules.getVEquipmentType(ename);
		switch (etype.type)
		{
			case VEquipmentType::Type::Engine:
			{
				auto engine = std::make_shared<VEngine>(static_cast<const VEngineType &>(etype));
				this->equipment.emplace_back(engine);
				LogInfo("Equipped \"%s\" with engine \"%s\"", this->type.name.c_str(),
				        ename.c_str());
				break;
			}
			case VEquipmentType::Type::Weapon:
			{
				auto &wtype = static_cast<const VWeaponType &>(etype);
				auto weapon = std::make_shared<VWeapon>(wtype, shared_from_this(), wtype.max_ammo);
				this->equipment.emplace_back(weapon);
				LogInfo("Equipped \"%s\" with weapon \"%s\"", this->type.name.c_str(),
				        ename.c_str());
				break;
			}
			case VEquipmentType::Type::General:
			{
				// FIXME: Implement 'general' equipment
				LogInfo("Equipped \"%s\" with general equipment \"%s\"", this->type.name.c_str(),
				        ename.c_str());
				break;
			}
			default:
				LogError("Default equipment for \"%s\" at pos (%d,%d} has invalid type",
				         this->type.name.c_str(), pos.x, pos.y);
		}
	}
}

}; // namespace OpenApoc
