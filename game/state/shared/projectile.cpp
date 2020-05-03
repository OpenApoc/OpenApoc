#define _USE_MATH_DEFINES
#include "game/state/shared/projectile.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/city.h"
#include "game/state/city/vehicle.h"
#include "game/state/gamestate.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/state/tilemap/tileobject_projectile.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

Projectile::Projectile(Type type, StateRef<Vehicle> firer, StateRef<Vehicle> target,
                       Vec3<float> targetPosition, Vec3<float> position, Vec3<float> velocity,
                       int turnRate, float lifetime, int damage, unsigned int delay,
                       int depletionRate, unsigned int tail_length,
                       std::list<sp<Image>> projectile_sprites, sp<Sample> impactSfx,
                       StateRef<DoodadType> doodadType, sp<VoxelMap> voxelMap, int stunTicks,
                       std::list<StateRef<VEquipmentType>> splitIntoTypes, bool manualFire)
    : type(type), position(position), velocity(velocity), turnRate(turnRate), age(0),
      lifetime(lifetime), damage(damage), delay_ticks_remaining(delay),
      depletionRate(depletionRate), firerVehicle(firer), firerPosition(firer->position),
      trackedVehicle(target), targetPosition(targetPosition), previousPosition(position),
      spritePositions({position}), tail_length(tail_length), projectile_sprites(projectile_sprites),
      sprite_distance(1.0f / TILE_Y_CITY), voxelMapLof(turnRate > 0 ? voxelMap : nullptr),
      voxelMapLos(voxelMap), manualFire(manualFire), impactSfx(impactSfx), doodadType(doodadType),
      velocityScale(VELOCITY_SCALE_CITY), stunTicks(stunTicks), splitIntoTypesCity(splitIntoTypes)
{
	// enough ticks to pass 1 tile diagonally and some more since vehicles can move quite quickly
	ownerInvulnerableTicks =
	    (int)ceilf(1.33f * 1.5f * (float)TICK_SCALE / glm::length(velocity / velocityScale)) + 1;
	if (target)
		trackedObject = target->tileObject;
}
Projectile::Projectile(Type type, StateRef<BattleUnit> firer, StateRef<BattleUnit> target,
                       Vec3<float> targetPosition, Vec3<float> position, Vec3<float> velocity,
                       int turnRate, float lifetime, int damage, unsigned int delay,
                       int depletionRate, unsigned int tail_length,
                       std::list<sp<Image>> projectile_sprites, sp<Sample> impactSfx,
                       StateRef<DoodadType> doodadType, StateRef<DamageType> damageType,
                       sp<VoxelMap> voxelMap, int stunTicks,
                       std::list<StateRef<AEquipmentType>> splitIntoTypes, bool manualFire)
    : type(type), position(position), velocity(velocity), turnRate(turnRate), age(0),
      lifetime(lifetime), damage(damage), delay_ticks_remaining(delay),
      depletionRate(depletionRate), firerUnit(firer), firerPosition(firer->position),
      trackedUnit(target), targetPosition(targetPosition), previousPosition(position),
      spritePositions({position}), tail_length(tail_length), projectile_sprites(projectile_sprites),
      sprite_distance(1.0f / TILE_Y_BATTLE), voxelMapLof(turnRate > 0 ? voxelMap : nullptr),
      voxelMapLos(voxelMap), manualFire(manualFire), impactSfx(impactSfx), doodadType(doodadType),
      damageType(damageType), velocityScale(VELOCITY_SCALE_BATTLE), stunTicks(stunTicks),
      splitIntoTypesBattle(splitIntoTypes)
{
	// enough ticks to pass 1 tile diagonally
	ownerInvulnerableTicks =
	    (int)ceilf(1.5f * (float)TICK_SCALE / glm::length(velocity / velocityScale)) + 1;
	if (target)
		trackedObject = target->tileObject;
}

Projectile::Projectile()
    : type(Type::Beam), position(0, 0, 0), velocity(0, 0, 0), previousPosition(0, 0, 0),
      velocityScale(1, 1, 1)
{
}

void Projectile::update(GameState &state, unsigned int ticks)
{
	// Delay the projectile accordingly
	if (delay_ticks_remaining > ticks)
	{
		delay_ticks_remaining -= ticks;
		return;
	}
	else
	{
		ticks -= delay_ticks_remaining;
		delay_ticks_remaining = 0;
		if (ticks == 0)
		{
			return;
		}
	}

	if (ownerInvulnerableTicks > ticks)
	{
		ownerInvulnerableTicks -= ticks;
	}
	else
	{
		ownerInvulnerableTicks = 0;
	}
	this->previousPosition = this->position;

	// Tracking
	if (turnRate > 0)
	{
		if (trackedObject)
		{
			targetPosition = trackedObject->getVoxelCentrePosition();
		}
		else
		{
			// Stop tracking if arrived
			if ((Vec3<int>)position == (Vec3<int>)targetPosition)
			{
				turnRate = 0;
			}
		}
		auto targetVector = targetPosition - position;
		auto cross = glm::cross(velocity, targetVector);
		// Cross product is 0 if we are moving straight on target
		if (cross.x != 0.0f || cross.y != 0.0f || cross.z != 0.0f)
		{
			float maxAngleToTurn = (float)ticks * turnRate * PROJECTILE_TURN_PER_TICK;
			// angle is always > 0, turning direction determined by cross product
			float angleToTarget =
			    clamp(glm::angle(glm::normalize(velocity), glm::normalize(targetVector)), 0.0f,
			          maxAngleToTurn);
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, angleToTarget, cross);
			velocity = glm::vec3(rotationMat * glm::vec4(velocity, 1.0));
		}
	}

	// Apply velocity
	auto newPosition = this->position +
	                   ((static_cast<float>(ticks) / TICK_SCALE) * this->velocity) / velocityScale;

	// Increase travelled distance
	this->age += glm::length(((float)ticks / TICK_SCALE) * this->velocity);

	// Remove projectile if it's ran out of life or fell off the end of the world
	auto mapSize = this->tileObject->map.size;
	if (newPosition.x < 0 || newPosition.x >= mapSize.x || newPosition.y < 0 ||
	    newPosition.y >= mapSize.y || newPosition.z < 0 || newPosition.z >= mapSize.z ||
	    this->age >= this->lifetime)
	{
		die(state, true, true, true);
		return;
	}

	// Move projectile to new position
	this->position = newPosition;
	this->tileObject->setPosition(newPosition);

	// Spawn projectile sprite points
	while (glm::length(spritePositions.front() - position) > sprite_distance)
	{
		spritePositions.push_front(spritePositions.front() +
		                           glm::normalize(position - spritePositions.front()) *
		                               sprite_distance);
		if (spritePositions.size() > tail_length)
		{
			spritePositions.pop_back();
		}
	}
}

void Projectile::die(GameState &state, bool displayDoodad, bool playSound, bool expired)
{
	auto this_shared = shared_from_this();
	if (firerVehicle)
	{
		state.current_city->handleProjectileHit(state, this_shared, displayDoodad, playSound,
		                                        expired);
	}
	else // firerUnit or stray battle projectile
	{
		state.current_battle->handleProjectileHit(state, this_shared, displayDoodad, playSound,
		                                          expired);
	}
	if (this->tileObject)
	{
		this->tileObject->removeFromMap();
		this->tileObject.reset();
	}
}

Collision Projectile::checkProjectileCollision(TileMap &map)
{
	if (!this->tileObject)
	{
		// It's possible the projectile reached the end of it's lifetime this frame
		// so ignore stuff without a tile
		return {};
	}

	sp<TileObject> ignoredObject;
	StateRef<Organisation> firer;
	if (ownerInvulnerableTicks > 0)
	{
		if (firerVehicle)
		{
			ignoredObject = firerVehicle->tileObject;
		}
		else if (firerUnit)
		{
			ignoredObject = firerUnit->tileObject;
		}
	}
	if (firerVehicle)
	{
		firer = firerVehicle->owner;
	}
	else if (firerUnit)
	{
		firer = firerUnit->owner;
	}
#ifdef DEBUG_ALLOW_PROJECTILE_ON_PROJECTILE_FRIENDLY_FIRE
	// Missiles should not shoot down non-missiles even when friendly firing
	// otherwise they kill themselves immediately
	if (type == Type::Beam)
	{
		firer = nullptr;
	}
#endif // DEBUG_ALLOW_PROJECTILE_ON_PROJECTILE_FRIENDLY_FIRE

	Collision c = map.findCollision(this->previousPosition, this->position, {}, ignoredObject,
	                                false, false, 0, false, firer);
	if (!c)
		return {};

	c.projectile = shared_from_this();
	return c;
}

Projectile::~Projectile()
{
	if (this->tileObject)
	{
		this->tileObject->removeFromMap();
	}
}

}; // namespace OpenApoc
