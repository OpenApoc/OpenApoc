#include "game/state/city/projectile.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_vehicle.h"

namespace OpenApoc
{

const std::map<Projectile::Type, UString> Projectile::TypeMap = {
    {Type::Beam, "beam"}, {Type::Missile, "missile"},
};

Projectile::Projectile(Type type, StateRef<Vehicle> firer, StateRef<Vehicle> target,
                       Vec3<float> position, Vec3<float> velocity, int turnRate,
                       unsigned int lifetime, int damage, unsigned int tail_length,
                       std::list<sp<Image>> projectile_sprites, sp<Sample> impactSfx,
                       StateRef<DoodadType> doodadType)
    : type(type), position(position), velocity(velocity), turnRate(turnRate), age(0),
      lifetime(lifetime), damage(damage), firerVehicle(firer), trackedVehicle(target),
      previousPosition(position), spritePositions({position}), tail_length(tail_length),
      projectile_sprites(projectile_sprites), sprite_distance(1.0f / TILE_Y_CITY),
      impactSfx(impactSfx), doodadType(doodadType), velocityScale(VELOCITY_SCALE_CITY)
{
	// 36 / (velocity length) = enough ticks to pass 1 whole tile
	ownerInvulnerableTicks = (int)ceilf(36.0f / glm::length(velocity / velocityScale)) + 1;
	if (target)
		trackedObject = target->tileObject;
}
// FIXME: Properly add unit projectiles and shit
Projectile::Projectile(Type type, StateRef<BattleUnit> firer, StateRef<BattleUnit> target,
                       Vec3<float> position, Vec3<float> velocity, int turnRate,
                       unsigned int lifetime, int damage, unsigned int tail_length,
                       std::list<sp<Image>> projectile_sprites, sp<Sample> impactSfx,
                       StateRef<DoodadType> doodadType, StateRef<DamageType> damageType)
    : type(type), position(position), velocity(velocity), turnRate(turnRate), age(0),
      lifetime(lifetime), damage(damage), firerUnit(firer), trackedUnit(target),
      previousPosition(position), spritePositions({position}), tail_length(tail_length),
      projectile_sprites(projectile_sprites), sprite_distance(1.0f / TILE_Y_BATTLE),
      impactSfx(impactSfx), doodadType(doodadType), damageType(damageType),
      velocityScale(VELOCITY_SCALE_BATTLE)
{
	// 36 / (velocity length) = enough ticks to pass 1 whole tile
	ownerInvulnerableTicks = (int)ceilf(36.0f / glm::length(velocity / velocityScale)) + 1;
	if (target)
		trackedObject = target->tileObject;
}

Projectile::Projectile()
    : type(Type::Beam), position(0, 0, 0), velocity(0, 0, 0), age(0), lifetime(0), damage(0),
      previousPosition(0, 0, 0), tail_length(0), velocityScale(1, 1, 1)
{
}

void Projectile::update(GameState &state, unsigned int ticks)
{
	if (ownerInvulnerableTicks > 0)
		ownerInvulnerableTicks -= ticks;
	this->age += ticks;
	this->previousPosition = this->position;

	// Tracking
	if (turnRate > 0 && trackedObject)
	{
		// This is a proper tracking algorithm. Turn to target every tick, within our allowance.
		// Howver, it does not seem to work. If tracking like this, missiles hardly ever miss
		// It seems that vanilla had some kind of error or latency introduced here, which allowed
		// missiles to miss if target was trying to dodge. We must implement that otherwise
		// missile weapons are extremely overpowered

		auto targetVector = trackedObject->getVoxelCentrePosition() - position;
		auto cross = glm::cross(velocity, targetVector);
		// Cross product is 0 if we are moving straight on target
		if (cross.x != 0.0f || cross.y != 0.0f || cross.z != 0.0f)
		{
			float maxAngleToTurn = (float)ticks * turnRate * PROJECTILE_TURN_PER_TICK;
			// angle is always > 0, turning direction determined by cross product
			float angleToTarget =
				clamp(glm::angle(glm::normalize(velocity), glm::normalize(targetVector)),
						0.0f, maxAngleToTurn);
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, angleToTarget, cross);
			velocity = glm::vec3(rotationMat * glm::vec4(velocity, 1.0));
		}

	
		
		/*
		// FIXME: Won't work, ignore for now, but in the future we must implement vanilla algorithm
		// Attempt to recreate vanilla algorithm
		// Vanilla algorithm only turns if projectile is not within a 45 degree arc containing target
		// Implement properly, for now implementing only XY
		
		// Vector to target
		auto targetVector = trackedObject->getVoxelCentrePosition() - position;
		// Vector to target with Z=0
		Vec3<float> targetVectorXY = { targetVector.x, targetVector.y, 0.0f };
		// Velocity with Z = 0
		Vec3<float> velocityXY = { velocity.x, velocity.y, 0.0f };

		// Step 01: Turn fully on Z
		
		// Target vector's Z angle
		auto targetVectorAngleZ = (targetVector.z >=0 ? 1 : -1) * glm::angle(glm::normalize(targetVector), glm::normalize(targetVectorXY));
		// Velocity's Z angle
		auto velocityAngleZ = (velocity.z >= 0 ? 1 : -1) * glm::angle(glm::normalize(velocity), glm::normalize(velocityXY));

		// Turning on Z, full angle
		auto angleDiffZ = targetVectorAngleZ - velocityAngleZ;
		auto crossDiffZ = glm::cross(velocity, velocityXY);
		if (crossDiffZ.x != 0 || crossDiffZ.y != 0 || crossDiffZ.z != 0)
		{
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, angleDiffZ, crossDiffZ);
			velocity = glm::vec3(rotationMat * glm::vec4(velocity, 1.0));
		}

		// Step 02: Turn on XY
		
		// Reference vector
		Vec3<float> south = { 0.0f, 1.0f, 0.0f };
		// Determine target's facing relative to south
		// If cross's Z is >0 then velocity is to the right of the south, else to the left
		auto targetCrossXY = glm::cross(targetVectorXY, south);
		// Get arc id's for target,			-4	 4
		// if south is down then it		 -3		    3
		// would look like this:		 -2			2
		//							 		-1   1
		// Since angle returned is always positive, we add sign using cross product's Z
		auto targetArcXY = (targetCrossXY.z >= 0 ? -1 : 1 ) * (int)(glm::angle(glm::normalize(targetVectorXY), south) * 180.0f / M_PI / 45.0f + 1);
		// Same as above but for velocity
		auto velocityCrossXY = glm::cross(velocityXY, south);
		auto velocityArcXY = (targetCrossXY.z >= 0 ? -1 : 1) * (int)(glm::angle(glm::normalize(velocityXY), south) * 180.0f / M_PI / 45.0f + 1);

		// Turning on XY, vanilla turned always by full value
		float angleDiffXY = (float)ticks * turnRate * PROJECTILE_TURN_PER_TICK;
		auto crossDiffXY = glm::cross(velocityXY, targetVectorXY);
		if (crossDiffXY.x != 0 || crossDiffXY.y != 0 || crossDiffXY.z != 0)
		{
			// Vanilla only turned if arcs don't match
			if (velocityArcXY == targetArcXY)
			{
				LogWarning("(%f %f) (%f %f) %f %f",targetVectorXY.x, targetVectorXY.y, velocityXY.x, velocityXY.y,
					glm::angle(glm::normalize(targetVectorXY), south)* 180.0f / M_PI, glm::angle(glm::normalize(velocityXY), south)* 180.0f / M_PI);
 				angleDiffXY = 0.0f;  
			}

			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, angleDiffXY, crossDiffXY);
			velocity = glm::vec3(rotationMat * glm::vec4(velocity, 1.0));
		}
		*/
	}

	// Apply velocity
	auto newPosition = this->position +
	                   ((static_cast<float>(ticks) / TICK_SCALE) * this->velocity) / velocityScale;

	// Remove projectile if it's ran out of life or fell off the end of the world
	auto mapSize = this->tileObject->map.size;
	if (newPosition.x < 0 || newPosition.x >= mapSize.x || newPosition.y < 0 ||
	    newPosition.y >= mapSize.y || newPosition.z < 0 || newPosition.z >= mapSize.z ||
	    this->age >= this->lifetime)
	{
		auto this_shared = shared_from_this();
		if (firerVehicle)
		{
			for (auto &city : state.cities)
				city.second->projectiles.erase(std::dynamic_pointer_cast<Projectile>(this_shared));
		}
		else // firerUnit
		{
			state.current_battle->projectiles.erase(
			    std::dynamic_pointer_cast<Projectile>(this_shared));
		}
		this->tileObject->removeFromMap();
		this->tileObject.reset();
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

Collision Projectile::checkProjectileCollision(TileMap &map)
{
	if (!this->tileObject)
	{
		// It's possible the projectile reached the end of it's lifetime this frame
		// so ignore stuff without a tile
		return {};
	}

	Collision c = map.findCollision(this->previousPosition, this->position);
	if (c && ownerInvulnerableTicks > 0 &&
	    ((c.obj->getType() == TileObject::Type::Vehicle &&
	      this->firerVehicle == std::static_pointer_cast<TileObjectVehicle>(c.obj)->getVehicle()) ||
	     (c.obj->getType() == TileObject::Type::Unit &&
	      this->firerUnit == std::static_pointer_cast<TileObjectBattleUnit>(c.obj)->getUnit())))
	{
		return {};
	}

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
