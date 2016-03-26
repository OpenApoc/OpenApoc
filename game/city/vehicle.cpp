#include "game/city/vehicle.h"
#include "framework/logger.h"
#include "game/city/building.h"
#include "game/city/city.h"
#include "game/city/vehiclemission.h"
#include "game/city/vequipment.h"
#include "game/gamestate.h"
#include "game/organisation.h"
#include "game/rules/vequipment.h"
#include "game/tileview/tileobject_shadow.h"
#include "game/tileview/tileobject_vehicle.h"
#include "library/sp.h"

#include <limits>
#include <random>

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
	virtual void update(GameState &state, unsigned int ticks) override
	{
		float speed = vehicle.getSpeed();
		if (!vehicle.missions.empty())
		{
			vehicle.missions.front()->update(state, this->vehicle, ticks);
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
					while (vehicle.missions.front()->isFinished(state, this->vehicle))
					{
						LogInfo("Vehicle mission \"%s\" finished",
						        vehicle.missions.front()->getName().c_str());
						vehicle.missions.pop_front();
						if (!vehicle.missions.empty())
						{
							LogInfo("Vehicle mission \"%s\" starting",
							        vehicle.missions.front()->getName().c_str());
							vehicle.missions.front()->start(state, this->vehicle);
							continue;
						}
						else
						{
							LogInfo("No next vehicle mission, going idle");
							break;
						}
					}
					if (vehicle.missions.empty() ||
					    vehicle.missions.front()->getNextDestination(state, this->vehicle,
					                                                 goalPosition) == false)
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

Vehicle::Vehicle() : health(0), shield(0) {}

Vehicle::~Vehicle() {}

void Vehicle::launch(TileMap &map, GameState &state, Vec3<float> initialPosition)
{
	LogWarning("Launching");
	if (this->tileObject)
	{
		LogError("Trying to launch already-launched vehicle");
		return;
	}
	auto bld = this->currentlyLandedBuilding;
	if (!bld)
	{
		LogError("Vehicle not in a building?");
		return;
	}
	this->position = initialPosition;
	bld->landed_vehicles.erase({&state, shared_from_this()});
	this->currentlyLandedBuilding = "";
	this->mover.reset(new FlyingVehicleMover(*this, initialPosition));
	map.addObjectToMap(shared_from_this());
}

void Vehicle::land(GameState &state, StateRef<Building> b)
{
	auto vehicleTile = this->tileObject;
	if (!vehicleTile)
	{
		LogError("Trying to land already-landed vehicle");
		return;
	}
	if (this->currentlyLandedBuilding)
	{
		LogError("Vehicle already in a building?");
		return;
	}
	this->currentlyLandedBuilding = b;
	b->landed_vehicles.insert({&state, shared_from_this()});
	this->tileObject->removeFromMap();
	this->tileObject.reset();
	this->shadowObject->removeFromMap();
	this->shadowObject = nullptr;
	this->position = {0, 0, 0};
}

void Vehicle::setupMover()
{
	switch (this->type->type)
	{
		case VehicleType::Type::Flying:
			this->mover.reset(new FlyingVehicleMover(*this, this->position));
		default:
			LogWarning("TODO: non flying vehicle movers");
	}
}

void Vehicle::update(GameState &state, unsigned int ticks)

{
	if (!this->missions.empty())
		this->missions.front()->update(state, *this, ticks);
	if (this->mover)
		this->mover->update(state, ticks);
	auto vehicleTile = this->tileObject;
	if (vehicleTile)
	{
		for (auto &equipment : this->equipment)
		{
			if (equipment->type->type != VEquipmentType::Type::Weapon)
				continue;
			equipment->update(ticks);
			if (equipment->canFire())
			{
				// Find something to shoot at!
				// FIXME: Only run on 'aggressive'? And not already a manually-selected target?
				float range = equipment->getRange();
				// Find the closest enemy within the firing arc
				float closestEnemyRange = std::numeric_limits<float>::max();
				sp<TileObjectVehicle> closestEnemy;
				for (auto pair : state.vehicles)
				{
					auto otherVehicle = pair.second;
					if (otherVehicle.get() == this)
					{
						/* Can't fire at yourself */
						continue;
					}
					if (otherVehicle->city != this->city)
					{
						/* Can't fire on things a world away */
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
					auto projectile = equipment->fire(target);
					if (projectile)
					{
						vehicleTile->map.addObjectToMap(projectile);
						this->city->projectiles.insert(projectile);
					}
					else
					{
						LogWarning("Fire() produced no object");
					}
				}
			}
		}
	}
	// FIXME: Make shield recharge rate variable?
	this->shield += ticks;
	if (this->shield > this->getMaxShield())
	{
		this->shield = this->getMaxShield();
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
	this->position = pos;
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
	float speed = this->type->top_speed;

	for (auto &e : this->equipment)
	{
		if (e->type->type != VEquipmentType::Type::Engine)
			continue;
		speed += e->type->top_speed;
	}

	return speed;
}

int Vehicle::getMaxConstitution() const { return this->getMaxHealth() + this->getMaxShield(); }

int Vehicle::getConstitution() const { return this->getHealth() + this->getShield(); }

int Vehicle::getMaxHealth() const { return this->type->health; }

int Vehicle::getHealth() const { return this->health; }

int Vehicle::getMaxShield() const
{
	int maxShield = 0;

	for (auto &e : this->equipment)
	{
		if (e->type->type != VEquipmentType::Type::General)
			continue;
		maxShield += e->type->shielding;
	}

	return maxShield;
}

int Vehicle::getShield() const { return this->shield; }

int Vehicle::getArmor() const
{
	int armor = 0;
	// FIXME: Check this the sum of all directions
	for (auto &armorDirection : this->type->armour)
	{
		armor += armorDirection.second;
	}
	return armor;
}

int Vehicle::getAccuracy() const
{
	int accuracy = 0;

	for (auto &e : this->equipment)
	{
		if (e->type->type != VEquipmentType::Type::General)
			continue;
		accuracy += e->type->accuracy_modifier;
	}
	return accuracy;
}

// FIXME: Check int/float speed conversions
int Vehicle::getTopSpeed() const { return this->getSpeed(); }

int Vehicle::getAcceleration() const
{
	// FIXME: This is somehow related to enginer 'power' and weight
	int weight = this->getWeight();
	int acceleration = this->type->acceleration;
	int power = 0;
	for (auto &e : this->equipment)
	{
		if (e->type->type != VEquipmentType::Type::Engine)
			continue;
		power += e->type->power;
	}
	if (weight == 0)
	{
		LogError("Vehicle %s has zero weight", this->name.c_str());
		return 0;
	}
	acceleration += std::max(1, power / weight);

	if (power == 0 && acceleration == 0)
	{
		// No engine shows a '0' acceleration in the stats ui
		return 0;
	}
	return acceleration;
}

int Vehicle::getWeight() const
{
	int weight = this->type->weight;
	for (auto &e : this->equipment)
	{
		weight += e->type->weight;
	}
	if (weight == 0)
	{
		LogError("Vehicle with no weight");
	}
	return weight;
}

int Vehicle::getFuel() const
{
	// Zero fuel is normal on some vehicles (IE ufos/'dimension-capable' xcom)
	int fuel = 0;

	for (auto &e : this->equipment)
	{
		if (e->type->type != VEquipmentType::Type::Engine)
			continue;
		fuel += e->type->max_ammo;
	}

	return fuel;
}

int Vehicle::getMaxPassengers() const
{
	int passengers = this->type->passengers;

	for (auto &e : this->equipment)
	{
		if (e->type->type != VEquipmentType::Type::General)
			continue;
		passengers += e->type->passengers;
	}
	return passengers;
}

int Vehicle::getPassengers() const
{ // FIXME: Track passengers
	return 0;
}

int Vehicle::getMaxCargo() const
{
	int cargo = 0;

	for (auto &e : this->equipment)
	{
		if (e->type->type != VEquipmentType::Type::General)
			continue;
		cargo += e->type->cargo_space;
	}
	return cargo;
}

int Vehicle::getCargo() const
{ // FIXME: Track cargo
	return 0;
}

bool Vehicle::canAddEquipment(Vec2<int> pos, StateRef<VEquipmentType> type) const
{
	Vec2<int> slotOrigin;
	bool slotFound = false;
	// Check the slot this occupies hasn't already got something there
	for (auto &slot : this->type->equipment_layout_slots)
	{
		if (slot.bounds.within(pos))
		{
			slotOrigin = slot.bounds.p0;
			slotFound = true;
			break;
		}
	}
	// If this was not within a slot fail
	if (!slotFound)
	{
		return false;
	}
	// Check that the equipment doesn't overlap with any other and doesn't
	// go outside a slot of the correct type
	Rect<int> bounds{pos, pos + type->equipscreen_size};
	for (auto &otherEquipment : this->equipment)
	{
		// Something is already in that slot, fail
		if (otherEquipment->equippedPosition == slotOrigin)
		{
			return false;
		}
		Rect<int> otherBounds{otherEquipment->equippedPosition,
		                      otherEquipment->equippedPosition +
		                          otherEquipment->type->equipscreen_size};
		if (otherBounds.intersects(bounds))
		{
			LogInfo("Equipping \"%s\" on \"%s\" at {%d,%d} failed: Intersects with other equipment",
			        type->name.c_str(), this->name.c_str(), pos.x, pos.y);
			return false;
		}
	}

	// Check that this doesn't go outside a slot of the correct type
	for (int y = 0; y < type->equipscreen_size.y; y++)
	{
		for (int x = 0; x < type->equipscreen_size.x; x++)
		{
			Vec2<int> slotPos = {x, y};
			slotPos += pos;
			bool validSlot = false;
			for (auto &slot : this->type->equipment_layout_slots)
			{
				if (slot.bounds.within(slotPos) && slot.type == type->type)
				{
					validSlot = true;
					break;
				}
			}
			if (!validSlot)
			{
				LogInfo("Equipping \"%s\" on \"%s\" at {%d,%d} failed: No valid slot",
				        type->name.c_str(), this->name.c_str(), pos.x, pos.y);
				return false;
			}
		}
	}
	return true;
}

void Vehicle::addEquipment(GameState &state, Vec2<int> pos, StateRef<VEquipmentType> type)
{
	// We can't check this here, as some of the non-buyable vehicles have insane initial equipment
	// layouts
	// if (!this->canAddEquipment(pos, type))
	//{
	//	LogError("Trying to add \"%s\" at {%d,%d} on vehicle \"%s\" failed", type.id.c_str(), pos.x,
	//	         pos.y, this->name.c_str());
	//}
	Vec2<int> slotOrigin;
	bool slotFound = false;
	// Check the slot this occupies hasn't already got something there
	for (auto &slot : this->type->equipment_layout_slots)
	{
		if (slot.bounds.within(pos))
		{
			slotOrigin = slot.bounds.p0;
			slotFound = true;
			break;
		}
	}
	// If this was not within a slow fail
	if (!slotFound)
	{
		LogError("Equipping \"%s\" on \"%s\" at {%d,%d} failed: No valid slot", type->name.c_str(),
		         this->name.c_str(), pos.x, pos.y);
		return;
	}

	switch (type->type)
	{
		case VEquipmentType::Type::Engine:
		{
			auto engine = mksp<VEquipment>();
			engine->type = type;
			this->equipment.emplace_back(engine);
			engine->equippedPosition = slotOrigin;
			LogInfo("Equipped \"%s\" with engine \"%s\"", this->name.c_str(), type->name.c_str());
			break;
		}
		case VEquipmentType::Type::Weapon:
		{
			auto thisRef = StateRef<Vehicle>(&state, shared_from_this());
			auto weapon = mksp<VEquipment>();
			weapon->type = type;
			weapon->owner = thisRef;
			weapon->ammo = type->max_ammo;
			this->equipment.emplace_back(weapon);
			weapon->equippedPosition = slotOrigin;
			LogInfo("Equipped \"%s\" with weapon \"%s\"", this->name.c_str(), type->name.c_str());
			break;
		}
		case VEquipmentType::Type::General:
		{
			auto equipment = mksp<VEquipment>();
			equipment->type = type;
			LogInfo("Equipped \"%s\" with general equipment \"%s\"", this->name.c_str(),
			        type->name.c_str());
			equipment->equippedPosition = slotOrigin;
			this->equipment.emplace_back(equipment);
			break;
		}
		default:
			LogError("Equipment \"%s\" for \"%s\" at pos (%d,%d} has invalid type",
			         type->name.c_str(), this->name.c_str(), pos.x, pos.y);
	}
}

void Vehicle::removeEquipment(sp<VEquipment> object)
{
	this->equipment.remove(object);
	// TODO: Any other variable values here?
	// Clamp shield
	if (this->shield > this->getMaxShield())
	{
		this->shield = this->getMaxShield();
	}
}

void Vehicle::equipDefaultEquipment(GameState &state)
{
	LogInfo("Equipping \"%s\" with default equipment", this->type->name.c_str());
	for (auto &pair : this->type->initial_equipment_list)
	{
		auto &pos = pair.first;
		auto &etype = pair.second;

		this->addEquipment(state, pos, etype);
	}
}

template <> sp<Vehicle> StateObject<Vehicle>::get(const GameState &state, const UString &id)
{
	auto it = state.vehicles.find(id);
	if (it == state.vehicles.end())
	{
		LogError("No vehicle matching ID \"%s\"", id.c_str());
		return nullptr;
	}
	return it->second;
}

template <> const UString &StateObject<Vehicle>::getPrefix()
{
	static UString prefix = "VEHICLE_";
	return prefix;
}
template <> const UString &StateObject<Vehicle>::getTypeName()
{
	static UString name = "Vehicle";
	return name;
}
template <>
const UString &StateObject<Vehicle>::getId(const GameState &state, const sp<Vehicle> ptr)
{
	static const UString emptyString = "";
	for (auto &v : state.vehicles)
	{
		if (v.second == ptr)
			return v.first;
	}
	LogError("No vehicle matching pointer %p", ptr.get());
	return emptyString;
}
}; // namespace OpenApoc
