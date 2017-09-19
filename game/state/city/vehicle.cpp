#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/city/vehicle.h"
#include "framework/logger.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/projectile.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gamestate.h"
#include "game/state/organisation.h"
#include "game/state/rules/vehicle_type.h"
#include "game/state/rules/vequipment_type.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "library/sp.h"
#include <glm/glm.hpp>
#include <limits>
#include <queue>
#include <random>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

namespace 
{
	static const float M_2xPI = 2.0f * M_PI;

	VehicleType::Direction getDirectionLarge(float facing)
	{
		static std::map<float, VehicleType::Direction> DirectionMap = {
			{ 0.0f * (float)M_PI, VehicleType::Direction::N },
			{ 0.125f * (float)M_PI, VehicleType::Direction::NNE },
			{ 0.25f * (float)M_PI, VehicleType::Direction::NE },
			{ 0.375f * (float)M_PI, VehicleType::Direction::NEE },
			{ 0.5f * (float)M_PI, VehicleType::Direction::E },
			{ 0.625f * (float)M_PI, VehicleType::Direction::SEE },
			{ 0.75f * (float)M_PI, VehicleType::Direction::SE },
			{ 0.875f * (float)M_PI, VehicleType::Direction::SSE },
			{ 1.0f * (float)M_PI, VehicleType::Direction::S },
			{ 1.125f * (float)M_PI, VehicleType::Direction::SSW },
			{ 1.25f * (float)M_PI, VehicleType::Direction::SW },
			{ 1.375f * (float)M_PI, VehicleType::Direction::SWW },
			{ 1.5f * (float)M_PI, VehicleType::Direction::W },
			{ 1.625f * (float)M_PI, VehicleType::Direction::NWW },
			{ 1.75f * (float)M_PI, VehicleType::Direction::NW },
			{ 1.875f * (float)M_PI, VehicleType::Direction::NNW },
		};

		float closestDiff = FLT_MAX;
		VehicleType::Direction closestDir = VehicleType::Direction::N;
		for (auto &p : DirectionMap)
		{
			float d1 = p.first - facing;
			if (d1 < 0.0f)
			{
				d1 += M_2xPI;
			}
			float d2 = facing - p.first;
			if (d2 < 0.0f)
			{
				d2 += M_2xPI;
			}
			float diff = std::min(d1, d2);
			if (diff < closestDiff)
			{
				closestDiff = diff;
				closestDir = p.second;
			}
		}
		return closestDir;
	}

	VehicleType::Direction getDirectionSmall(float facing)
	{
		static std::map<float, VehicleType::Direction> DirectionMap = {
			{ 0.0f * (float)M_PI, VehicleType::Direction::N },
			{ 0.25f * (float)M_PI, VehicleType::Direction::NE },
			{ 0.5f * (float)M_PI, VehicleType::Direction::E },
			{ 0.75f * (float)M_PI, VehicleType::Direction::SE },
			{ 1.0f * (float)M_PI, VehicleType::Direction::S },
			{ 1.25f * (float)M_PI, VehicleType::Direction::SW },
			{ 1.5f * (float)M_PI, VehicleType::Direction::W },
			{ 1.75f * (float)M_PI, VehicleType::Direction::NW },
		};

		float closestDiff = FLT_MAX;
		VehicleType::Direction closestDir = VehicleType::Direction::N;
		for (auto &p : DirectionMap)
		{
			float d1 = p.first - facing;
			if (d1 < 0.0f)
			{
				d1 += M_2xPI;
			}
			float d2 = facing - p.first;
			if (d2 < 0.0f)
			{
				d2 += M_2xPI;
			}
			float diff = std::min(d1, d2);
			if (diff < closestDiff)
			{
				closestDiff = diff;
				closestDir = p.second;
			}
		}
		return closestDir;
	}
}

const UString &Vehicle::getPrefix()
{
	static UString prefix = "VEHICLE_";
	return prefix;
}
const UString &Vehicle::getTypeName()
{
	static UString name = "Vehicle";
	return name;
}

const UString &Vehicle::getId(const GameState &state, const sp<Vehicle> ptr)
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

class FlyingVehicleMover : public VehicleMover
{
  public:
	Vec3<float> goalPosition;
	FlyingVehicleMover(Vehicle &v, Vec3<float> initialGoal)
	    : VehicleMover(v), goalPosition(initialGoal)
	{
	}
	void update(GameState &state, unsigned int ticks) override
	{
		// Flag wether we need to update banking and direction
		bool updateSprite = false;
		// Move until we become idle or run out of ticks
		while (ticks > 0)
		{
			// We may have left the map and need to cease to move
			auto vehicleTile = this->vehicle.tileObject;
			if (!vehicleTile)
			{
				break;
			}
			// Move vehicle to goal
			if (goalPosition != vehicle.position)
			{
				updateSprite = true;
				Vec3<float> vectorToGoal = goalPosition - vehicle.position;
				int distanceToGoal = glm::length(vectorToGoal * VELOCITY_SCALE_CITY);
				int curVelocity = std::max(1.0f, glm::length(vehicle.velocity / (float)TICK_SCALE));
				// Cannot reach in one go
				if (distanceToGoal / curVelocity > ticks)
				{
					auto newPos = vehicle.position;
					newPos += vehicle.velocity * (float)ticks / VELOCITY_SCALE_CITY / (float)TICK_SCALE;
					auto ticksToTurn = std::min(vehicle.ticksToTurn, ticks);
					vehicle.ticksToTurn -= ticksToTurn;
					vehicle.facing += vehicle.angularVelocity * (float)ticksToTurn;
					if (vehicle.facing < 0.0f)
					{
						vehicle.facing += M_2xPI;
					}
					if (vehicle.facing >= M_2xPI)
					{
						vehicle.facing -= M_2xPI;
					}
					vehicle.setPosition(newPos);
					ticks = 0;
				}
				else
				// Can reach in one go
				{
					vehicle.setPosition(goalPosition);
					vehicle.velocity = { 0.0f, 0.0f, 0.0f };
					vehicle.facing = vehicle.goalFacing;
					vehicle.angularVelocity = 0.0f;
					vehicle.ticksToTurn = 0;
					ticks -= distanceToGoal / curVelocity;
				}
			}
			// Request new goal
			if (goalPosition == vehicle.position)
			{
				while (!vehicle.missions.empty() && vehicle.missions.front()->isFinished(state, this->vehicle))
				{
					LogInfo("Vehicle mission \"%s\" finished",
						vehicle.missions.front()->getName());
					vehicle.missions.pop_front();
					if (!vehicle.missions.empty())
					{
						LogInfo("Vehicle mission \"%s\" starting",
							vehicle.missions.front()->getName());
						vehicle.missions.front()->start(state, this->vehicle);
					}
				}
				if (vehicle.missions.empty()
					|| !vehicle.missions.front()->getNextDestination(state, this->vehicle, goalPosition) 
					|| goalPosition == vehicle.position)
				{
					break;
				}
				else
				{
					// New goal acquired, set velocity and angles
					float speed = vehicle.getSpeed();
					Vec3<float> vectorToGoal = (goalPosition - vehicle.position) * VELOCITY_SCALE_CITY;
					vehicle.velocity = glm::normalize(vectorToGoal) * speed;
					Vec2<float> targetFacingVector = { vectorToGoal.x, vectorToGoal.y };
					// New facing?
					if (targetFacingVector.x != 0.0f || targetFacingVector.y != 0.0f)
					{
						targetFacingVector = glm::normalize(targetFacingVector);
						float a1 = acosf(-targetFacingVector.y);
						float a2 = asinf(targetFacingVector.x);
						vehicle.goalFacing = a2 >= 0 ? a1 : M_2xPI - a1;
					}
					if (vehicle.goalFacing != vehicle.facing)
					{
						float d1 = vehicle.goalFacing - vehicle.facing;
						if (d1 < 0.0f)
						{
							d1 += M_2xPI;
						}
						float d2 = vehicle.facing - vehicle.goalFacing;
						if (d2 < 0.0f)
						{
							d2 += M_2xPI;
						}
						// FIXME: Proper turning speed
						// This value was hand-made to look proper on annihilators
						float TURNING_MULT = (float)M_PI / (float)TICK_SCALE / VELOCITY_SCALE_CITY.x / 1.5f;
						if (d1 <= d2)
						{
							vehicle.angularVelocity = speed * TURNING_MULT;
							// Nudge vehicle in the other direction to make animation look even
							// (otherwise, first frame is 1/2 of other frames)
							vehicle.facing -= 0.06f * (float)M_PI;
							if (vehicle.facing < 0.0f)
							{
								vehicle.facing += M_2xPI;
							}
						}
						else
						{
							vehicle.angularVelocity = -speed * TURNING_MULT;
							// Nudge vehicle in the other direction to make animation look even
							// (otherwise, first frame is 1/2 of other frames)
							vehicle.facing += 0.06f * (float)M_PI;
							if (vehicle.facing >= M_2xPI)
							{
								vehicle.facing -= M_2xPI;
							}
						}
						// Establish ticks to turn 
						// (turn further than we need, again, for animation purposes)
						float turnDist = std::min(d1, d2);
						turnDist += 0.12f * (float)M_PI;
						vehicle.ticksToTurn = floorf(std::abs(turnDist / vehicle.angularVelocity));

						// FIXME: Introduce proper turning speed
						// Here we just slow down velocity if we're moving too quickly
						int ticksToMove = floorf(glm::length(vectorToGoal) / glm::length(vehicle.velocity) * (float)TICK_SCALE) - 5.0f;
						if (ticksToMove < vehicle.ticksToTurn)
						{
							vehicle.velocity *= (float)ticksToMove / (float)vehicle.ticksToTurn;
						}
					}
				}
			}
		}
		// Update sprite if required
		if (updateSprite)
		{
			vehicle.updateSprite(state);
		}
	}
};

VehicleMover::VehicleMover(Vehicle &v) : vehicle(v) {}

VehicleMover::~VehicleMover() = default;

Vehicle::Vehicle()
    : attackMode(AttackMode::Standard), altitude(Altitude::Standard), position(0, 0, 0),
      velocity(0, 0, 0)
{
}

Vehicle::~Vehicle() = default;

void Vehicle::launch(TileMap &map, GameState &state, Vec3<float> initialPosition)
{
	LogInfo("Launching %s", this->name);
	if (this->tileObject)
	{
		LogError("Trying to launch already-launched vehicle");
		return;
	}
	auto bld = this->currentlyLandedBuilding;
	if (bld)
	{
		bld->landed_vehicles.erase({&state, shared_from_this()});
		this->currentlyLandedBuilding = "";
	}
	this->position = initialPosition;
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

Vec3<float> Vehicle::getMuzzleLocation() const
{
	return Vec3<float>(position.x, position.y,
	                   position.z - tileObject->getVoxelOffset().z + (float)type->height / 16.0f);
}

void Vehicle::update(GameState &state, unsigned int ticks)

{
	if (cloakTicksAccumulated < CLOAK_TICKS_REQUIRED_VEHICLE)
	{
		cloakTicksAccumulated += ticks;
	}
	if (!hasCloak())
	{
		cloakTicksAccumulated = 0;
	}

	if (!this->missions.empty())
		this->missions.front()->update(state, *this, ticks);
	while (!this->missions.empty() && this->missions.front()->isFinished(state, *this))
	{
		LogInfo("Vehicle mission \"%s\" finished", this->missions.front()->getName());
		this->missions.pop_front();
		if (!this->missions.empty())
		{
			LogInfo("Vehicle mission \"%s\" starting", this->missions.front()->getName());
			this->missions.front()->start(state, *this);
			continue;
		}
		else
		{
			LogInfo("No next vehicle mission, going idle");
			break;
		}
	}

	if (this->type->type == VehicleType::Type::UFO && this->missions.empty())
	{
		auto alien_city = state.cities["CITYMAP_ALIEN"];
		// Make UFOs patrol their city if we're looking at it
		if (this->city.getSp() == alien_city && state.current_city == this->city)
		{
			this->missions.emplace_back(VehicleMission::patrol(state, *this));
		}
	}

	if (this->mover)
		this->mover->update(state, ticks);

	auto vehicleTile = this->tileObject;
	if (vehicleTile)
	{
		if (!this->type->animation_sprites.empty())
		{
			vehicleTile->nextFrame(ticks);
		}

		bool has_active_weapon = false;
		for (auto &equipment : this->equipment)
		{
			if (equipment->type->type != EquipmentSlotType::VehicleWeapon)
				continue;
			equipment->update(ticks);
			if (!this->isCrashed() && this->attackMode != Vehicle::AttackMode::Evasive &&
			    equipment->canFire())
			{
				has_active_weapon = true;
			}
		}

		if (has_active_weapon)
		{
			// Find something to shoot at!
			sp<TileObjectVehicle> enemy;
			if (!missions.empty() &&
			    missions.front()->type == VehicleMission::MissionType::AttackVehicle &&
			    vehicleTile->getDistanceTo(missions.front()->targetVehicle->tileObject) <=
			        getFiringRange())
			{
				enemy = missions.front()->targetVehicle->tileObject;
			}
			else
			{
				enemy = findClosestEnemy(state, vehicleTile);
			}

			if (enemy)
			{
				attackTarget(state, vehicleTile, enemy);
			}
		}
	}

	int maxShield = this->getMaxShield();
	if (maxShield)
	{
		this->shieldRecharge += ticks;
		if (this->shieldRecharge > TICKS_PER_SECOND)
		{
			this->shield += this->getShieldRechargeRate() * this->shieldRecharge / TICKS_PER_SECOND;
			this->shieldRecharge %= TICKS_PER_SECOND;
			if (this->shield > maxShield)
			{
				this->shield = maxShield;
			}
		}
	}
}

void Vehicle::updateSprite(GameState & state)
{
	// Set banking
	if (ticksToTurn > 0 && angularVelocity > 0.0f)
	{
		banking = VehicleType::Banking::Right;
	}
	else if (ticksToTurn > 0 && angularVelocity < 0.0f)
	{
		banking = VehicleType::Banking::Left;
	}
	else if (velocity.z > 0.0f)
	{
		banking = VehicleType::Banking::Ascending;
	}
	else  if (velocity.z < 0.0f)
	{
		banking = VehicleType::Banking::Descending;
	}
	else
	{
		banking = VehicleType::Banking::Flat;
	}
	// UFOs don't care about banking and direction being correct
	// Otherwise ensure banking is valid
	if (type->type != VehicleType::Type::UFO)
	{
		if (type->directional_sprites.find(banking) == type->directional_sprites.end())
		{
			banking = VehicleType::Banking::Flat;
		}
	}

	// Set direction
	switch (banking)
	{
		case VehicleType::Banking::Right:
		case VehicleType::Banking::Left:
			direction = getDirectionLarge(facing);
			// UFOs don't care about banking and direction being correct
			// Otherwise ensure direction is valid
			if (type->type == VehicleType::Type::UFO)
			{
				break;
			}
			if (type->directional_sprites.at(banking).find(direction) != type->directional_sprites.at(banking).end())
			{
				break;
			}
			// Fall-through since this direction is not valid
		case VehicleType::Banking::Ascending:
		case VehicleType::Banking::Descending:
		case VehicleType::Banking::Flat:
			direction = getDirectionSmall(facing);
			break;
	}

	// Set shadow direction
	shadowDirection = direction;
	if (type->directional_shadow_sprites.find(shadowDirection) == type->directional_shadow_sprites.end())
	{
		switch (shadowDirection)
		{
			case VehicleType::Direction::NNE:
			case VehicleType::Direction::NEE:
			case VehicleType::Direction::SEE:
			case VehicleType::Direction::SSE:
			case VehicleType::Direction::SSW:
			case VehicleType::Direction::SWW:
			case VehicleType::Direction::NWW:
			case VehicleType::Direction::NNW:
				// If direction is from large set then try small set
				shadowDirection = getDirectionSmall(facing);
				if (type->directional_shadow_sprites.find(shadowDirection) != type->directional_shadow_sprites.end())
				{
					break;
				}
				// Fall-through, we have to settle for north
			default:
				shadowDirection = VehicleType::Direction::N;
				break;
		}
	}
}

bool Vehicle::isCrashed() const { return this->health < this->type->crash_health; }
/* // Test code to make UFOs crash immediately upon hit,
// may be useful in the future as crashing is not yet perfect
 bool Vehicle::isCrashed() const
{ return this->health < this->type->health && this->type->crash_health > 0; }
*/

bool Vehicle::applyDamage(GameState &state, int damage, float armour)
{
	if (this->shield <= damage)
	{
		if (this->shield > 0)
		{
			damage -= this->shield;
			this->shield = 0;

			// destroy the shield modules
			for (auto it = this->equipment.begin(); it != this->equipment.end();)
			{
				if ((*it)->type->type == EquipmentSlotType::VehicleGeneral &&
				    (*it)->type->shielding > 0)
				{
					it = this->equipment.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		damage -= (int)armour;
		if (damage > 0)
		{
			this->health -= damage;
			if (this->health <= 0)
			{
				this->health = 0;
				return true;
			}
			else if (isCrashed())
			{
				this->missions.clear();
				this->missions.emplace_back(VehicleMission::crashLand(state, *this));
				this->missions.front()->start(state, *this);
				return false;
			}
		}
	}
	else
	{
		this->shield -= damage;
	}
	return false;
}

void Vehicle::handleCollision(GameState &state, Collision &c)
{
	if (!this->tileObject)
	{
		LogError("It's possible multiple projectiles hit the same tile in the same tick (?)");
		return;
	}

	auto projectile = c.projectile.get();
	if (projectile)
	{
		auto vehicleDir = glm::round(type->directionToVector(direction));
		auto projectileDir = glm::normalize(projectile->getVelocity());
		auto dir = vehicleDir + projectileDir;
		dir = glm::round(dir);

		auto armourDirection = VehicleType::ArmourDirection::Right;
		if (dir.x == 0 && dir.y == 0 && dir.z == 0)
		{
			armourDirection = VehicleType::ArmourDirection::Front;
		}
		else if (dir * 0.5f == vehicleDir)
		{
			armourDirection = VehicleType::ArmourDirection::Rear;
		}
		// FIXME: vehicle Z != 0
		else if (dir.z < 0)
		{
			armourDirection = VehicleType::ArmourDirection::Top;
		}
		else if (dir.z > 0)
		{
			armourDirection = VehicleType::ArmourDirection::Bottom;
		}
		else if ((vehicleDir.x == 0 && dir.x != dir.y) || (vehicleDir.y == 0 && dir.x == dir.y))
		{
			armourDirection = VehicleType::ArmourDirection::Left;
		}

		float armourValue = 0.0f;
		auto armour = this->type->armour.find(armourDirection);
		if (armour != this->type->armour.end())
		{
			armourValue = armour->second;
		}

		if (applyDamage(state, randDamage050150(state.rng, projectile->damage), armourValue))
		{
			auto doodad = city->placeDoodad(StateRef<DoodadType>{&state, "DOODAD_3_EXPLOSION"},
			                                this->tileObject->getCenter());

			this->shadowObject->removeFromMap();
			this->tileObject->removeFromMap();
			this->shadowObject.reset();
			this->tileObject.reset();
			state.vehicles.erase(this->getId(state, this->shared_from_this()));
			return;
		}
	}
}

sp<TileObjectVehicle> Vehicle::findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile)
{
	// Find the closest enemy within the firing arc
	float closestEnemyRange = std::numeric_limits<float>::max();
	sp<TileObjectVehicle> closestEnemy;
	for (auto &pair : state.vehicles)
	{
		auto otherVehicle = pair.second;
		if (otherVehicle.get() == this)
		{
			/* Can't fire at yourself */
			continue;
		}
		if (otherVehicle->isCrashed())
		{
			// Can't fire at crashed vehicles
			continue;
		}
		if (otherVehicle->city != this->city)
		{
			/* Can't fire on things a world away */
			continue;
		}
		if (this->owner->isRelatedTo(otherVehicle->owner) != Organisation::Relation::Hostile)
		{
			/* Not hostile, skip */
			continue;
		}
		auto otherVehicleTile = otherVehicle->tileObject;
		if (!otherVehicleTile)
		{
			/* Not in the map, ignore */
			continue;
		}
		float distance = vehicleTile->getDistanceTo(otherVehicleTile);

		// FIXME: Check weapon arc against otherVehicle
		if (distance < closestEnemyRange)
		{
			closestEnemyRange = distance;
			closestEnemy = otherVehicleTile;
		}
	}
	return closestEnemy;
}

void Vehicle::attackTarget(GameState &state, sp<TileObjectVehicle> vehicleTile,
                           sp<TileObjectVehicle> enemyTile)
{
	static const std::set<TileObject::Type> scenerySet = {TileObject::Type::Scenery};

	auto firePosition = getMuzzleLocation();
	auto target = enemyTile->getVoxelCentrePosition();
	auto distanceTiles = glm::length(position - target);

	auto distanceVoxels = this->tileObject->getDistanceTo(enemyTile);

	for (auto &eq : this->equipment)
	{
		// Not a weapon
		if (eq->type->type != EquipmentSlotType::VehicleWeapon)
			continue;
		// Out of ammo or on cooldown
		if (eq->canFire() == false)
			continue;
		// Out of range
		if (distanceVoxels > eq->getRange())
			continue;

		// Lead the target
		auto targetPosAdjusted = target;
		auto projectileVelocity = eq->type->speed * PROJECTILE_VELOCITY_MULTIPLIER;
		auto targetVelocity =
		    enemyTile->getVehicle()->velocity;
		targetPosAdjusted += targetVelocity * distanceTiles / projectileVelocity;

		// No sight to target
		if (vehicleTile->map.findCollision(firePosition, targetPosAdjusted, scenerySet))
			continue;

		eq->fire(state, targetPosAdjusted, {&state, enemyTile->getVehicle()});

		cloakTicksAccumulated = 0;
	}
}

float Vehicle::getFiringRange() const
{
	float range = 0;
	for (auto &equipment : this->equipment)
	{
		if (equipment->type->type != EquipmentSlotType::VehicleWeapon)
			continue;

		if (range < equipment->getRange())
		{
			range = equipment->getRange();
		}
	}
	return range;
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
		if (e->type->type != EquipmentSlotType::VehicleEngine)
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
		if (e->type->type != EquipmentSlotType::VehicleGeneral)
			continue;
		maxShield += e->type->shielding;
	}

	return maxShield;
}

int Vehicle::getShieldRechargeRate() const
{
	int shieldRecharge = 0;

	for (auto &e : this->equipment)
	{
		if (e->type->type != EquipmentSlotType::VehicleGeneral)
			continue;
		shieldRecharge += e->type->shielding > 0 ? 1 : 0;
	}

	return shieldRecharge;
}

bool Vehicle::isCloaked() const
{
	// FIXME: Ensure vehicle cloak implemented correctly
	return cloakTicksAccumulated >= CLOAK_TICKS_REQUIRED_VEHICLE;
}

bool Vehicle::hasCloak() const
{
	for (auto &e : this->equipment)
	{
		if (e->type->type != EquipmentSlotType::VehicleGeneral)
			continue;
		if (e->type->cloaking)
		{
			return true;
		}
	}

	return false;
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
	std::priority_queue<int> accModifiers;

	for (auto &e : this->equipment)
	{
		if (e->type->type != EquipmentSlotType::VehicleGeneral || e->type->accuracy_modifier <= 0)
			continue;
		// accuracy percentages are inverted in the data (e.g. 10% module gives 90)
		accModifiers.push(100 - e->type->accuracy_modifier);
	}

	double moduleEfficiency = 1.0;
	while (!accModifiers.empty())
	{
		accuracy += accModifiers.top() * moduleEfficiency;
		moduleEfficiency /= 2;
		accModifiers.pop();
	}
	return accuracy;
}

// FIXME: Check int/float speed conversions
int Vehicle::getTopSpeed() const { return (int)this->getSpeed(); }

int Vehicle::getAcceleration() const
{
	// FIXME: This is somehow related to enginer 'power' and weight
	int weight = this->getWeight();
	int acceleration = this->type->acceleration;
	int power = 0;
	for (auto &e : this->equipment)
	{
		if (e->type->type != EquipmentSlotType::VehicleEngine)
			continue;
		power += e->type->power;
	}
	if (weight == 0)
	{
		LogError("Vehicle %s has zero weight", this->name);
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
		if (e->type->type != EquipmentSlotType::VehicleEngine)
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
		if (e->type->type != EquipmentSlotType::VehicleGeneral)
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
		if (e->type->type != EquipmentSlotType::VehicleGeneral)
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
	//	LogError("Trying to add \"%s\" at {%d,%d} on vehicle \"%s\" failed", type.id, pos.x,
	//	         pos.y, this->name);
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
		LogError("Equipping \"%s\" on \"%s\" at %s failed: No valid slot", type->name, this->name,
		         pos);
		return;
	}

	switch (type->type)
	{
		case EquipmentSlotType::VehicleEngine:
		{
			auto engine = mksp<VEquipment>();
			engine->type = type;
			this->equipment.emplace_back(engine);
			engine->equippedPosition = slotOrigin;
			LogInfo("Equipped \"%s\" with engine \"%s\"", this->name, type->name);
			break;
		}
		case EquipmentSlotType::VehicleWeapon:
		{
			auto thisRef = StateRef<Vehicle>(&state, shared_from_this());
			auto weapon = mksp<VEquipment>();
			weapon->type = type;
			weapon->owner = thisRef;
			weapon->ammo = type->max_ammo;
			this->equipment.emplace_back(weapon);
			weapon->equippedPosition = slotOrigin;
			LogInfo("Equipped \"%s\" with weapon \"%s\"", this->name, type->name);
			break;
		}
		case EquipmentSlotType::VehicleGeneral:
		{
			auto equipment = mksp<VEquipment>();
			equipment->type = type;
			LogInfo("Equipped \"%s\" with general equipment \"%s\"", this->name, type->name);
			equipment->equippedPosition = slotOrigin;
			this->equipment.emplace_back(equipment);
			break;
		}
		default:
			LogError("Equipment \"%s\" for \"%s\" at pos (%d,%d} has invalid type", type->name,
			         this->name, pos.x, pos.y);
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
	LogInfo("Equipping \"%s\" with default equipment", this->type->name);
	for (auto &pair : this->type->initial_equipment_list)
	{
		auto &pos = pair.first;
		auto &etype = pair.second;

		this->addEquipment(state, pos, etype);
	}
}

sp<Vehicle> Vehicle::get(const GameState &state, const UString &id)
{
	auto it = state.vehicles.find(id);
	if (it == state.vehicles.end())
	{
		LogError("No vehicle matching ID \"%s\"", id);
		return nullptr;
	}
	return it->second;
}

sp<Equipment> Vehicle::getEquipmentAt(const Vec2<int> &position) const
{
	Vec2<int> slotPosition = {0, 0};
	for (auto &slot : this->type->equipment_layout_slots)
	{
		if (slot.bounds.within(position))
		{
			slotPosition = slot.bounds.p0;
		}
	}
	for (auto &eq : this->equipment)
	{
		Rect<int> eqBounds{eq->equippedPosition, eq->equippedPosition + eq->type->equipscreen_size};
		if (eqBounds.within(slotPosition))
		{
			return eq;
		}
	}
	return nullptr;
}

const std::list<EquipmentLayoutSlot> &Vehicle::getSlots() const
{
	return this->type->equipment_layout_slots;
}

std::list<std::pair<Vec2<int>, sp<Equipment>>> Vehicle::getEquipment() const
{
	std::list<std::pair<Vec2<int>, sp<Equipment>>> equipmentList;

	for (auto &equipmentObject : this->equipment)
	{
		equipmentList.emplace_back(
		    std::make_pair(equipmentObject->equippedPosition, equipmentObject));
	}

	return equipmentList;
}

}; // namespace OpenApoc
