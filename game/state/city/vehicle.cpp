#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/city/vehicle.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/sound.h"
#include "game/state/city/agentmission.h"
#include "game/state/city/base.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/city/vequipment.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/city/citycommonsamplelist.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/rules/city/vammotype.h"
#include "game/state/rules/city/vehicletype.h"
#include "game/state/rules/city/vequipmenttype.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/organisation.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_projectile.h"
#include "game/state/tilemap/tileobject_shadow.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include "library/sp.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <limits>
#include <queue>
#include <random>

namespace OpenApoc
{

namespace
{
static const float M_2xPI = 2.0f * M_PI;
float xyToFacing(const Vec2<float> &xy)
{
	float a1 = acosf(-xy.y);
	float a2 = asinf(xy.x);
	return a2 >= 0 ? a1 : M_2xPI - a1;
}
Vec2<float> facingToXY(float facing) { return {sinf(facing), -cosf(facing)}; }
// return the difference f1-f2 in the range [-pi;pi]
// a negative return value means f1 is CCW from f2.
float facingDistance(float f1, float f2)
{
	float result = f1 - f2;
	while (result > M_PI)
	{
		result -= M_2xPI;
	}
	while (result <= -M_PI)
	{
		result += M_2xPI;
	}
	return result;
}
} // namespace

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
	LogError("No vehicle matching pointer %p", static_cast<void *>(ptr.get()));
	return emptyString;
}

class FlyingVehicleMover : public VehicleMover
{
  public:
	FlyingVehicleMover(Vehicle &v) : VehicleMover(v) {}
	// Vehicle is considered idle whenever it's at goal in its tile, even if it has missions to do
	void updateIdle(GameState &state)
	{
		// Crashed/falling/sliding aren't doing anything
		if (vehicle.crashed || vehicle.falling || vehicle.sliding)
		{
			return;
		}
		// Vehicles on take off mission don't do anything
		if (!vehicle.missions.empty())
		{
			if (vehicle.missions.front()->type == VehicleMission::MissionType::TakeOff ||
			    vehicle.missions.front()->type == VehicleMission::MissionType::Land)
			{
				return;
			}
		}
		// Vehicles below ground don't do anything
		if (vehicle.position.z < 2.0f)
		{
			return;
		}
		// Don't idle every frame
		if (vehicle.ticksAutoActionAvailable > state.gameTime.getTicks())
		{
			return;
		}
		vehicle.ticksAutoActionAvailable = state.gameTime.getTicks() + TICKS_AUTO_ACTION_DELAY;

		// Step 01: Drop carried vehicle if we ever are w/o mission
		if (vehicle.missions.empty() && vehicle.carriedVehicle)
		{
			vehicle.dropCarriedVehicle(state);
		}

		// Step 02: Try to move to preferred altitude if no mission
		if (vehicle.missions.empty() && (int)vehicle.position.z != (int)vehicle.altitude)
		{
			auto targetPos = vehicle.position;
			if (vehicle.position.z < (int)vehicle.altitude)
			{
				targetPos.z += 1.0f;
			}
			else
			{
				targetPos.z -= 1.0f;
			}
			auto tFrom = vehicle.tileObject->getOwningTile();
			auto tTo = tFrom->map.getTile(targetPos);
			if (FlyingVehicleTileHelper{vehicle.tileObject->map, vehicle}.canEnterTile(tFrom, tTo))
			{
				if (!vehicle.missions.empty())
				{
					// Will need new path after moving
					vehicle.missions.front()->currentPlannedPath.clear();
				}
				auto adjustHeightMission = VehicleMission::gotoLocation(state, vehicle, targetPos);
				adjustHeightMission->currentPlannedPath.emplace_front(targetPos);
				adjustHeightMission->currentPlannedPath.emplace_front(vehicle.position);
				vehicle.addMission(state, adjustHeightMission);
				return;
			}
		}

		// Step 03: Find projectiles to dodge
		// FIXME: Read vehicle engagement rules, instead for now chance to dodge is
		// flat 100% / 80% / 50% / 10% depending on behavior
		// and passives don't dodge at all
		int dodge = 0;
		switch (vehicle.attackMode)
		{
			case Vehicle::AttackMode::Aggressive:
				dodge = 10;
				break;
			case Vehicle::AttackMode::Standard:
				dodge = 50;
				break;
			case Vehicle::AttackMode::Defensive:
				dodge = 80;
				break;
			case Vehicle::AttackMode::Evasive:
				dodge = 100;
				break;
		}
		if (vehicle.type->aggressiveness > 0 && randBoundsExclusive(state.rng, 0, 100) < dodge)
		{
			for (auto &p : state.current_city->projectiles)
			{
				// Step 02.01: Figure out if projectile can theoretically hit the vehicle

				// FIXME: Do not dodge projectiles that are too low damage vs us?
				// Is this also in rules of engagement?
				// Do not dodge our own projectiles we just fired
				if (p->age < 32.0f && p->firerVehicle == vehicle.shared_from_this())
				{
					continue;
				}
				// Find distance from vehicle to projectile path
				// Vehicle position relative to projectile
				auto point = vehicle.position - p->position;
				// Final projectile position before expiry (or 1 second passes)
				auto line =
				    glm::normalize(p->velocity) *
				    std::min(glm::length(p->velocity) * (float)TICKS_PER_SECOND / (float)TICK_SCALE,
				             p->lifetime - p->age) /
				    p->velocityScale;
				if (glm::length(line) == 0.0f)
				{
					continue;
				}
				auto lineNorm = glm::normalize(line);
				auto pointNorm = glm::normalize(point);
				float angle = glm::angle(lineNorm, pointNorm);
				float distanceToHit = glm::length(point) * cosf(angle);
				Vec3<float> hitPoint = {0.0f, 0.0f, 0.0f};
				if (angle > M_PI_2)
				{
					// Projectile going the other way, it can only hit us now or never
					// So hit point is where projectile is now, which is 0, 0, 0
					// Which we've set above, so we do nothing
				}
				else if (distanceToHit > glm::length(line))
				{
					// Projectile won't reach us for a full right triangle to be formed
					// So hit point is where projectile will expire
					hitPoint = line;
				}
				else
				{
					// Otherwise hit point forms a right triangle with 0, 0, 0 and our point
					// Which means we need to find one of the sides in a right triangle,
					// where we know angle adjacent and hypotenuse, so it's simple
					hitPoint = lineNorm * distanceToHit;
				}
				// Find furthest distance from vehicle that can be hit
				auto size = vehicle.type->size.begin()->second;
				float maxSize = std::max(size.x, size.y) * 1.41f / 2.0f;
				if (glm::length(point - hitPoint) > maxSize)
				{
					continue;
				}

				// Step 02.02: Figure out which direction to dodge in

				// Rotate space so that we see where the hit point is
				// Calculate change-of-basis matrix
				glm::mat3 transform = {};
				if (pointNorm.x == 0 && pointNorm.z == 0)
				{
					if (pointNorm.y < 0) // rotate 180 degrees
						transform =
						    glm::mat3(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
						              glm::vec3(0.0f, 0.0f, 1.0f));
					// else if lineNorm.y >= 0, leave transform as the identity matrix.
				}
				else
				{
					auto new_y = pointNorm;
					auto new_z = glm::normalize(glm::cross(new_y, Vec3<float>{0.0f, 1.0f, 0.0f}));
					auto new_x = glm::normalize(glm::cross(new_y, new_z));

					transform = glm::mat3(new_x, new_y, new_z);
				}
				// The point on which we hit our craft
				// (Craft at 0,0,0, forward facing matching positive Y axis)
				// So, +x = hit on right side, +z = hit on top etc.)
				auto hitPointRel = (hitPoint - point) * transform;
				// Rule:
				//   - if hit within 0.125f we can dodge either way on this axis
				//   - otherwise it doesn't matter, since projectile is too close to our center
				bool dodgeLeft = hitPointRel.x > -0.125f;
				bool dodgeRight = hitPointRel.x < 0.125f;
				bool dodgeDown = hitPointRel.z > -0.125f;
				bool dodgeUp = hitPointRel.z < 0.125f;

				// Step 02.03: Figure out which tile we can dodge into

				// We need to figure out what's "left" and what's "right" here
				// Rule:
				//   At least Pi/2 should be between vector we'll take and the projectile vector
				//   Otherwise we think we'll be moving too slow to dodge (basically moving
				//   backwards into projectile or forwards alongside it and still getting hit)
				auto point2d = glm::normalize(Vec2<float>{point.x, point.y});
				// Gather all allowed dodge locations according to the rules above
				std::list<Vec3<int>> possibleDodgeLocations;
				for (int x = -1; x <= 1; x++)
				{
					for (int y = -1; y <= 1; y++)
					{
						// Check if this is left/right unless going up/down strictly
						if (x != 0 || y != 0)
						{
							// Angle between vector to us and vector towards location
							auto angle = glm::angle(point2d, glm::normalize(Vec2<float>{x, y}));
							// Whether this location lies to our right side
							bool right =
							    asinf(glm::angle(point2d,
							                     glm::normalize(point2d + Vec2<float>{x, y}))) >= 0;
							// Can't dodge this way at all
							if ((right && !dodgeRight) || (!right && !dodgeLeft))
							{
								continue;
							}
							// Angle too small
							if (angle < M_PI_2 || angle > 1.5f * (float)M_PI_2)
							{
								continue;
							}
							// Dodging horizontally
							possibleDodgeLocations.emplace_back(
							    vehicle.position.x + x, vehicle.position.y + y, vehicle.position.z);
						}
						// Dodging vertically
						if (dodgeUp)
						{
							possibleDodgeLocations.emplace_back(vehicle.position.x + x,
							                                    vehicle.position.y + y,
							                                    vehicle.position.z + 1.0f);
						}
						if (dodgeDown)
						{
							possibleDodgeLocations.emplace_back(vehicle.position.x + x,
							                                    vehicle.position.y + y,
							                                    vehicle.position.z + -1.0f);
						}
					}
				}

				// Step 02.04: Pick a dodge location that is valid for our vehicle to move into
				// right now

				std::list<Vec3<int>> dodgeLocations;
				for (auto &targetPos : possibleDodgeLocations)
				{
					if (!vehicle.tileObject->map.tileIsValid(targetPos))
					{
						continue;
					}
					auto tFrom = vehicle.tileObject->getOwningTile();
					auto tTo = tFrom->map.getTile(targetPos);
					if (FlyingVehicleTileHelper{vehicle.tileObject->map, vehicle}.canEnterTile(
					        tFrom, tTo))
					{
						dodgeLocations.emplace_back(targetPos);
					}
				}
				if (!dodgeLocations.empty())
				{
					auto targetPos = pickRandom(state.rng, dodgeLocations);

					if (!vehicle.missions.empty())
					{
						// Will need new path after moving
						vehicle.missions.front()->currentPlannedPath.clear();
					}
					auto dodgeMission = VehicleMission::gotoLocation(state, vehicle, targetPos);
					dodgeMission->currentPlannedPath.emplace_front(targetPos);
					dodgeMission->currentPlannedPath.emplace_front(vehicle.position);
					vehicle.addMission(state, dodgeMission);
					return;
				}
			}
		}
	}

	void update(GameState &state, unsigned int ticks) override
	{
		if (vehicle.falling)
		{
			updateFalling(state, ticks);
			return;
		}
		if (vehicle.sliding)
		{
			updateSliding(state, ticks);
			return;
		}
		if (vehicle.carriedByVehicle)
		{
			auto newPos = vehicle.carriedByVehicle->position;
			newPos.z = std::max(0.0f, newPos.z - 0.5f);
			vehicle.setPosition(newPos);
			vehicle.facing = vehicle.carriedByVehicle->facing;
			vehicle.updateSprite(state);
			return;
		}
		if (vehicle.crashed)
		{
			if (vehicle.smokeDoodad)
			{
				vehicle.smokeDoodad->update(state, ticks);
			}
			if (vehicle.type->type != VehicleType::Type::UFO)
			{
				updateCrashed(state, ticks);
				return;
			}
		}
		auto ticksToTurn = ticks;
		auto ticksToMove = ticks;

		unsigned lastTicksToTurn = 0;
		unsigned lastTicksToMove = 0;

		// This is amount of ticks (on average) per 1 tile travelled
		// a units travels 4 speed per second in tiles, where 4 is ticks_per_sec/tick_scale
		// so this = ticks_per_sec / (speed * ticks_per_sec / tick_scale / city_scale )
		// which simplifies to 1 / (speed / tick_scale / city_scale)
		// or to tick_scale * city_scale / speed
		int ticksPerTile = TICK_SCALE * VELOCITY_SCALE_CITY.x / vehicle.getSpeed();

		// Flag whether we need to update banking and direction
		bool updateSprite = false;
		// Move until we become idle or run out of ticks
		while (ticksToMove != lastTicksToMove || ticksToTurn != lastTicksToTurn)
		{
			lastTicksToMove = ticksToMove;
			lastTicksToTurn = ticksToTurn;

			// We may have left the map and need to cease to move
			auto vehicleTile = this->vehicle.tileObject;
			if (!vehicleTile)
			{
				break;
			}

			// Advance vehicle facing to goal
			if (ticksToTurn > 0 && vehicle.facing != vehicle.goalFacing)
			{
				updateSprite = true;
				if (vehicle.ticksToTurn > 0)
				{
					if (vehicle.ticksToTurn > ticksToTurn)
					{
						vehicle.ticksToTurn -= ticksToTurn;
						vehicle.facing += vehicle.angularVelocity * (float)ticksToTurn;
						ticksToTurn = 0;
						if (vehicle.facing < 0.0f)
						{
							vehicle.facing += M_2xPI;
						}
						if (vehicle.facing >= M_2xPI)
						{
							vehicle.facing -= M_2xPI;
						}
					}
					else
					{
						ticksToTurn -= vehicle.ticksToTurn;
						vehicle.facing = vehicle.goalFacing;
						vehicle.ticksToTurn = 0;
						vehicle.angularVelocity = 0.0f;
						vehicle.ticksAutoActionAvailable = 0;
					}
				}
			}

			// Advance vehicle position to goal
			if (ticksToMove > 0 && vehicle.goalPosition != vehicle.position)
			{
				updateSprite = true;
				Vec3<float> vectorToGoal = vehicle.goalPosition - vehicle.position;
				int distanceToGoal =
				    glm::length(vectorToGoal * VELOCITY_SCALE_CITY) /
				    std::max(0.00001f, glm::length(vehicle.velocity / (float)TICK_SCALE));
				// Cannot reach in one go
				if (distanceToGoal > ticksToMove)
				{
					auto newPos = vehicle.position;
					newPos += vehicle.velocity * (float)ticksToMove / VELOCITY_SCALE_CITY /
					          (float)TICK_SCALE;
					vehicle.setPosition(newPos);
					ticksToMove = 0;
				}
				else
				// Can reach in one go
				{
					vehicle.setPosition(vehicle.goalPosition);
					vehicle.velocity = {0.0f, 0.0f, 0.0f};
					ticksToMove -= distanceToGoal;
				}
			}

			// Request new goal
			if (vehicle.position == vehicle.goalPosition && vehicle.facing == vehicle.goalFacing)
			{
				// Need to pop before checking idle
				vehicle.popFinishedMissions(state);
				// Vehicle is considered idle if at goal even if there's more missions to do
				updateIdle(state);
				int turboTiles = state.skipTurboCalculations ? ticksToMove / ticksPerTile : 0;
				int turboTilesBefore = turboTiles;
				// Get new goal from mission
				if (!vehicle.getNewGoal(state, turboTiles))
				{
					if (!vehicle.tileObject)
					{
						return;
					}
					break;
				}
				// Turbo movement
				int turboTilesMoved = turboTilesBefore - turboTiles;
				if (turboTilesMoved > 0)
				{
					// Figure out facing
					Vec3<float> vectorToGoal = vehicle.goalPosition - vehicle.position;
					Vec2<float> targetFacingVector = {vectorToGoal.x, vectorToGoal.y};
					// New facing as well?
					if (targetFacingVector.x != 0.0f || targetFacingVector.y != 0.0f)
					{
						targetFacingVector = glm::normalize(targetFacingVector);
						vehicle.goalFacing = xyToFacing(targetFacingVector);
					}
					// Move and turn instantly
					vehicle.position = vehicle.goalPosition;
					vehicle.facing = vehicle.goalFacing;
					vehicle.ticksToTurn = 0;
					vehicle.angularVelocity = 0.0f;
					updateSprite = true;
					ticksToMove -= ticksPerTile * turboTilesMoved;
				}
				float speed = vehicle.getSpeed();
				// New position goal acquired, set velocity and angles
				if (vehicle.position != vehicle.goalPosition)
				{
					Vec3<float> vectorToGoal =
					    (vehicle.goalPosition - vehicle.position) * VELOCITY_SCALE_CITY;
					vehicle.velocity = glm::normalize(vectorToGoal) * speed;
					Vec2<float> targetFacingVector = {vectorToGoal.x, vectorToGoal.y};
					// New facing as well?
					if (targetFacingVector.x != 0.0f || targetFacingVector.y != 0.0f)
					{
						targetFacingVector = glm::normalize(targetFacingVector);
						vehicle.goalFacing = xyToFacing(targetFacingVector);
					}
				}
				// If new position requires new facing or we acquired new facing only
				if (vehicle.facing != vehicle.goalFacing)
				{
					float d = facingDistance(vehicle.goalFacing, vehicle.facing);
					if (d > 0.0f)
					{
						// Rotate CW towards goal
						vehicle.angularVelocity = std::min(vehicle.getAngularSpeed(), d);
					}
					else
					{
						// Rotate CCW towards goal
						vehicle.angularVelocity = std::max(-vehicle.getAngularSpeed(), d);
					}
					// Establish ticks to turn
					// (turn further than we need, again, for animation purposes)
					// d += 0.12f * (float)M_PI;
					vehicle.ticksToTurn = floorf(d / vehicle.angularVelocity);

					// FIXME: Introduce proper turning speed
					// Here we just slow down velocity if we're moving too quickly
					if (vehicle.position != vehicle.goalPosition)
					{
						Vec3<float> vectorToGoal =
						    (vehicle.goalPosition - vehicle.position) * VELOCITY_SCALE_CITY;
						int ticksToMove =
						    std::max(floorf(glm::length(vectorToGoal) /
						                    glm::length(vehicle.velocity) * (float)TICK_SCALE) -
						                 5.0f,
						             1.0f);
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

class GroundVehicleMover : public VehicleMover
{
  public:
	GroundVehicleMover(Vehicle &v) : VehicleMover(v) {}
	// Vehicle is considered idle whenever it's at goal in its tile, even if it has missions to do
	void updateIdle(GameState &state)
	{
		if (vehicle.ticksAutoActionAvailable > state.gameTime.getTicks())
		{
			return;
		}
		vehicle.ticksAutoActionAvailable = state.gameTime.getTicks() + TICKS_AUTO_ACTION_DELAY;

		// Do ground vehicles even do anything when idle? Do they dodge?
	}

	void update(GameState &state, unsigned int ticks) override
	{
		if (vehicle.falling)
		{
			updateFalling(state, ticks);
			return;
		}
		if (vehicle.sliding)
		{
			updateSliding(state, ticks);
			return;
		}
		if (vehicle.carriedByVehicle)
		{
			auto newPos = vehicle.carriedByVehicle->position;
			newPos.z = std::max(0.0f, newPos.z - 0.5f);
			vehicle.setPosition(newPos);
			vehicle.facing = vehicle.carriedByVehicle->facing;
			vehicle.updateSprite(state);
			return;
		}
		if (vehicle.crashed)
		{
			if (vehicle.smokeDoodad)
			{
				vehicle.smokeDoodad->update(state, ticks);
			}
			updateCrashed(state, ticks);
			return;
		}

		auto ticksToMove = ticks;
		// This is amount of ticks (on average) per 1 tile travelled
		// a units travels 4 speed per second in tiles, where 4 is ticks_per_sec/tick_scale
		// so this = ticks_per_sec / (speed * ticks_per_sec / tick_scale / city_scale )
		// which simplifies to 1 / (speed / tick_scale / city_scale)
		// or to tick_scale * city_scale / speed
		int ticksPerTile = TICK_SCALE * VELOCITY_SCALE_CITY.x / vehicle.getSpeed();

		unsigned lastTicksToMove = 0;

		// See that we're not in the air
		if (vehicle.tileObject && !vehicle.tileObject->getOwningTile()->presentScenery &&
		    !vehicle.city->map->getTile(vehicle.goalPosition)->presentScenery &&
		    (vehicle.goalWaypoints.empty() ||
		     !vehicle.city->map->getTile(vehicle.goalWaypoints.back())->presentScenery))
		{
			if (config().getBool("OpenApoc.NewFeature.CrashingGroundVehicles"))
			{
				vehicle.startFalling(state);
			}
			else
			{
				vehicle.die(state);
			}
			return;
		}

		// Flag whether we need to update banking and direction
		bool updateSprite = false;
		// Move until we become idle or run out of ticks
		while (ticksToMove != lastTicksToMove)
		{
			lastTicksToMove = ticksToMove;

			// We may have left the map and need to cease to move
			auto vehicleTile = this->vehicle.tileObject;
			if (!vehicleTile)
			{
				break;
			}

			// Advance vehicle position to goal
			if (ticksToMove > 0 && vehicle.goalPosition != vehicle.position)
			{
				updateSprite = true;
				Vec3<float> vectorToGoal = vehicle.goalPosition - vehicle.position;
				int distanceToGoal =
				    glm::length(vectorToGoal * VELOCITY_SCALE_CITY) /
				    std::max(0.00001f, glm::length(vehicle.velocity / (float)TICK_SCALE));
				// Cannot reach in one go
				if (distanceToGoal > ticksToMove)
				{
					auto newPos = vehicle.position;
					newPos += vehicle.velocity * (float)ticksToMove / VELOCITY_SCALE_CITY /
					          (float)TICK_SCALE;
					vehicle.setPosition(newPos);
					ticksToMove = 0;
				}
				else
				// Can reach in one go
				{
					vehicle.setPosition(vehicle.goalPosition);
					vehicle.velocity = {0.0f, 0.0f, 0.0f};
					ticksToMove -= distanceToGoal;
				}
			}

			// Request new goal
			if (vehicle.position == vehicle.goalPosition)
			{
				bool waypoint = false;
				if (!vehicle.goalWaypoints.empty())
				{
					vehicle.goalPosition = vehicle.goalWaypoints.front();
					vehicle.goalWaypoints.pop_front();
					waypoint = true;
				}
				else
				{
					// Need to pop before checking idle
					vehicle.popFinishedMissions(state);
					// Vehicle is considered idle if at goal even if there's more missions to do
					updateIdle(state);
					int turboTiles = state.skipTurboCalculations ? ticksToMove / ticksPerTile : 0;
					int turboTilesBefore = turboTiles;
					// Get new goal from mission
					if (!vehicle.getNewGoal(state, turboTiles))
					{
						if (!vehicle.tileObject)
						{
							return;
						}
						break;
					}
					// Turbo movement
					int turboTilesMoved = turboTilesBefore - turboTiles;
					if (turboTilesMoved > 0)
					{
						vehicle.position = vehicle.goalPosition;
						vehicle.facing = vehicle.goalFacing;
						updateSprite = true;
						ticksToMove -= ticksPerTile * turboTilesMoved;
					}
				}
				float speed = vehicle.getSpeed();
				// New position goal acquired, set velocity and angles
				if (vehicle.position != vehicle.goalPosition)
				{
					// Set up waypoint if got new position from mission
					if (!waypoint)
					{
						// If changing height
						if (vehicle.position.z != vehicle.goalPosition.z)
						{
							auto heightCurrent = vehicle.position.z - floorf(vehicle.position.z);
							auto heightGoal =
							    vehicle.goalPosition.z - floorf(vehicle.goalPosition.z);
							bool fromFlat = heightCurrent < 0.25f || heightCurrent > 0.75f;
							bool toFlat = heightGoal < 0.25f || heightGoal > 0.75f;
							// If we move from flat to flat then we're changing from into to onto
							// Change Z in the middle of the way
							if (fromFlat && toFlat)
							{
								vehicle.goalWaypoints.push_back(vehicle.goalPosition);
								// Add waypoint after midpoint at target z level
								Vec3<float> waypoint = {
								    vehicle.position.x * 0.45f + vehicle.goalPosition.x * 0.55f,
								    vehicle.position.y * 0.45f + vehicle.goalPosition.y * 0.55f,
								    vehicle.goalPosition.z};
								vehicle.goalWaypoints.push_front(waypoint);
								// Add waypoint before midpoint at current z level
								vehicle.goalPosition.x =
								    vehicle.position.x * 0.55f + vehicle.goalPosition.x * 0.45f;
								vehicle.goalPosition.y =
								    vehicle.position.y * 0.55f + vehicle.goalPosition.y * 0.45f;
								vehicle.goalPosition.z = vehicle.position.z;
							}
							else
							    // If we're on flat surface then first move to midpoint then start
							    // to
							    // change Z
							    if (fromFlat)
							{
								vehicle.goalWaypoints.push_back(vehicle.goalPosition);
								// Add midpoint waypoint at target z level
								vehicle.goalPosition.x =
								    (vehicle.position.x + vehicle.goalPosition.x) / 2.0f;
								vehicle.goalPosition.y =
								    (vehicle.position.y + vehicle.goalPosition.y) / 2.0f;
								vehicle.goalPosition.z = vehicle.position.z;
							}
							// Else if we end on flat surface first change Z then move flat
							else if (toFlat)
							{
								vehicle.goalWaypoints.push_back(vehicle.goalPosition);
								// Add midpoint waypoint at current z level
								vehicle.goalPosition.x =
								    (vehicle.position.x + vehicle.goalPosition.x) / 2.0f;
								vehicle.goalPosition.y =
								    (vehicle.position.y + vehicle.goalPosition.y) / 2.0f;
							}
							// If we're moving from nonflat to nonflat then we need no midpoint at
							// all
						}
					}

					Vec3<float> vectorToGoal =
					    (vehicle.goalPosition - vehicle.position) * VELOCITY_SCALE_CITY;
					vehicle.velocity = glm::normalize(vectorToGoal) * speed;
					Vec2<float> targetFacingVector = {vectorToGoal.x, vectorToGoal.y};
					// New facing as well?
					if (targetFacingVector.x != 0.0f || targetFacingVector.y != 0.0f)
					{
						targetFacingVector = glm::normalize(targetFacingVector);
						vehicle.goalFacing = xyToFacing(targetFacingVector);
					}
				}
				// If new position requires new facing or we acquired new facing only
				if (vehicle.facing != vehicle.goalFacing)
				{
					vehicle.facing = vehicle.goalFacing;
					updateSprite = true;
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

namespace
{
float inline reduceAbsValue(float value, float by)
{
	if (value > 0)
	{
		if (value > by)
		{
			value -= by;
		}
		else
		{
			value = 0;
		}
	}
	else
	{
		if (value < -by)
		{
			value += by;
		}
		else
		{
			value = 0;
		}
	}
	return value;
}
} // namespace

void VehicleMover::updateFalling(GameState &state, unsigned int ticks)
{
	auto fallTicksRemaining = ticks;

	auto &map = *vehicle.city->map;

	if (vehicle.angularVelocity != 0)
	{
		vehicle.facing += vehicle.angularVelocity * (float)ticks;
		if (vehicle.facing < 0.0f)
		{
			vehicle.facing += M_2xPI;
		}
		if (vehicle.facing >= M_2xPI)
		{
			vehicle.facing -= M_2xPI;
		}
	}

	while (fallTicksRemaining-- > 0)
	{
		auto newPosition = vehicle.position;

		// Random doodads 2% chance if low health
		auto vehicleHealth = vehicle.getHealth();
		// Check that vehicle health is not zero or we try to divide by zero
		if (vehicleHealth != 0 && vehicle.getMaxHealth() / vehicle.getHealth() >= 3 &&
		    randBoundsExclusive(state.rng, 0, 100) < 2)
		{
			LogWarning("Doodads");
			UString doodadId = randBool(state.rng) ? "DOODAD_1_AUTOCANNON" : "DOODAD_2_AIRGUARD";
			auto doodadPos = vehicle.position;
			doodadPos.x += (float)randBoundsInclusive(state.rng, -3, 3) / 10.0f;
			doodadPos.y += (float)randBoundsInclusive(state.rng, -3, 3) / 10.0f;
			doodadPos.z += (float)randBoundsInclusive(state.rng, -3, 3) / 10.0f;
			vehicle.city->placeDoodad({&state, doodadId}, doodadPos);
			fw().soundBackend->playSample(state.city_common_sample_list->vehicleExplosion,
			                              vehicle.position, 0.25f);
		}

		vehicle.velocity.z -= FV_ACCELERATION;
		vehicle.velocity.x = reduceAbsValue(vehicle.velocity.x, FV_ACCELERATION / 8);
		vehicle.velocity.y = reduceAbsValue(vehicle.velocity.y, FV_ACCELERATION / 8);
		newPosition += vehicle.velocity / (float)TICK_SCALE / VELOCITY_SCALE_BATTLE;

		// If we fell downwards see if we went from a tile with into scenery
		if ((int)vehicle.position.z != (int)newPosition.z)
		{
			if (vehicle.tileObject && vehicle.tileObject->getOwningTile() &&
			    vehicle.tileObject->getOwningTile()->presentScenery)
			{
				auto presentScenery = vehicle.tileObject->getOwningTile()->presentScenery;
				if (presentScenery &&
				    presentScenery->type->getATVMode() == SceneryTileType::WalkMode::Into)
				{
					// We went through "Into" scenery, force landing on it
					newPosition = vehicle.position;
					newPosition.z = floorf(vehicle.position.z);
				}
			}
		}

		// Fell outside map
		if (!map.tileIsValid(newPosition))
		{
			vehicle.die(state, false, nullptr);
			return;
		}

		// Check tile we're in (or falling into)
		auto tile = map.getTile(newPosition);
		bool newTile = (Vec3<int>)newPosition != (Vec3<int>)vehicle.position;
		if (tile->presentScenery)
		{
			auto collisionDamage =
			    std::max((float)vehicle.type->health * FV_COLLISION_DAMAGE_MIN,
			             (float)std::min(FV_COLLISION_DAMAGE_LIMIT,
			                             (float)tile->presentScenery->type->constitution *
			                                 FV_COLLISION_DAMAGE_CONSTITUTION_MULTIPLIER));
			auto atvMode = tile->presentScenery->type->getATVMode();
			bool tryPlowThrough = tile->presentScenery->initialPosition.z != -1;
			if (tryPlowThrough)
			{
				switch (atvMode)
				{
					case SceneryTileType::WalkMode::Into:
					case SceneryTileType::WalkMode::Onto:
						if (newPosition.z >= tile->getRestingPosition(false, true).z)
						{
							tryPlowThrough = false;
						}
						break;
					case SceneryTileType::WalkMode::None:
						if (tile->presentScenery->type->height >= 12)
						{
							// Only try to plow through high Nones once
							tryPlowThrough = newTile;
						}
						else if (newPosition.z >= tile->getRestingPosition(false, true).z)
						{
							tryPlowThrough = false;
						}
						break;
				}
			}
			bool plowedThrough = false;
			if (tryPlowThrough)
			{
				// Crash chance depends on velocity and weight, scenery resists with constitution
				// Weight of Annihilator is >4500, Hawk >5300, Valkyrie >3300, Phoenix ~900
				// Provided:
				// - Velocity mult of 1.5 for high speed, divisor of 125 and flat value of 50
				// - Constitution of 20
				// Typical results are:
				// - Fast Hawk			: 113%/73% plow chance before/after reduction
				// - Fast Annihilator	: 104%/64% plow chance before/after reduction
				// - Fast Valkyrie		: 90%/50% plow chance before/after reduction
				// - Fast Phoenix		: 61%/21% plow chance before/after reduction
				float velocityMult =
				    (glm::length(vehicle.velocity) > FV_PLOW_CHANCE_HIGH_SPEED_THRESHOLD)
				        ? FV_PLOW_CHANCE_HIGH_SPEED_MULTIPLIER
				        : 1.0f;
				int plowThroughChance =
				    FV_PLOW_CHANCE_FLAT +
				    velocityMult * vehicle.getWeight() * FV_PLOW_CHANCE_WEIGHT_MULTIPLIER -
				    (float)tile->presentScenery->type->constitution *
				        FV_PLOW_CHANCE_CONSTITUTION_MULTIPLIER;
				plowedThrough = randBoundsExclusive(state.rng, 0, 100) < plowThroughChance;
				if (plowedThrough)
				{
					// Allow "into" to remain damaged, kill others outright
					tile->presentScenery->die(state, atvMode != SceneryTileType::WalkMode::Into);

					// "None" scenery damages our face if we plowed through it
					// Otherwise (Into/Onto) no damage as we will still get damage on landing
					// A 12.5% chance to evade damage
					if (atvMode == SceneryTileType::WalkMode::None &&
					    randBoundsExclusive(state.rng, 0,
					                        FV_COLLISION_DAMAGE_ONE_IN_CHANCE_TO_EVADE) > 0 &&
					    vehicle.applyDamage(state, collisionDamage, 0))
					{
						// Died
						return;
					}
				}
			}
			if (!plowedThrough && atvMode == SceneryTileType::WalkMode::None &&
			    tile->presentScenery->type->height >= 12)
			{
				// Didn't plow through
				bool movedToTheSide = false;
				if ((int)vehicle.position.x != (int)newPosition.x)
				{
					movedToTheSide = true;
					newPosition.x = vehicle.position.x;
					vehicle.velocity.x = 0.0f;
				}
				if ((int)vehicle.position.y != (int)newPosition.y)
				{
					movedToTheSide = true;
					newPosition.y = vehicle.position.y;
					vehicle.velocity.y = 0.0f;
				}
				// If we moved to the side of other tile into scenery then
				// cancel movement on this tick and try again with capped velocity
				if (movedToTheSide)
				{
					// "None" scenery gives half damage for bouncing off
					// A 12.5% chance to evade damage
					if (randBoundsExclusive(state.rng, 0,
					                        FV_COLLISION_DAMAGE_ONE_IN_CHANCE_TO_EVADE) > 0 &&
					    vehicle.applyDamage(state, collisionDamage / 2, 0))
					{
						// Died
						return;
					}
					// Cancel movement
					continue;
				}
				// If we moved only downwards then we land on the scenery, so force it
				newPosition.z = floorf(newPosition.z);
			}
		}
		vehicle.setPosition(newPosition);

		// See if we've landed
		tile = vehicle.tileObject->getOwningTile();
		auto presentScenery = tile->presentScenery;
		if (presentScenery)
		{
			auto atvMode = presentScenery->type->getATVMode();
			switch (atvMode)
			{
				case SceneryTileType::WalkMode::None:
				case SceneryTileType::WalkMode::Onto:
				case SceneryTileType::WalkMode::Into:
				{
					if (newPosition.z <= tile->getRestingPosition(false, true).z)
					{
						vehicle.falling = false;
					}
					break;
				}
			}
			// Landed
			if (!vehicle.falling)
			{
				// A 12.5% chance to evade damage
				auto collisionDamage =
				    std::max((float)vehicle.type->health * FV_COLLISION_DAMAGE_MIN,
				             std::min(FV_COLLISION_DAMAGE_LIMIT,
				                      (float)presentScenery->type->constitution *
				                          FV_COLLISION_DAMAGE_CONSTITUTION_MULTIPLIER));
				if (randBoundsExclusive(state.rng, 0, FV_COLLISION_DAMAGE_ONE_IN_CHANCE_TO_EVADE) >
				        0 &&
				    vehicle.applyDamage(state, collisionDamage / 2, 0))
				{
					// Died
					return;
				}
				// Move to resting position in the tile
				Vec3<float> newGoal = tile->getRestingPosition(false, true);
				vehicle.goalWaypoints.push_back(newGoal);
				newPosition.z = newGoal.z;
				// Translate Z velocity into XY velocity
				Vec2<float> vel2d = {vehicle.velocity.x, vehicle.velocity.y};
				if (vel2d.x != 0.0f || vel2d.y != 0.0f)
				{
					vel2d = glm::normalize(vel2d) * vehicle.velocity.z / 3.0f;
					vehicle.velocity.x -= vel2d.x;
					vehicle.velocity.y -= vel2d.y;
				}
				vehicle.velocity.z = 0;
				vehicle.setPosition(newPosition);
				vehicle.goalPosition = vehicle.position;
				vehicle.angularVelocity /= 2.0f;
				// Start sliding and eventually crash if:
				// - Flying vehicle
				// - Ground vehicle fell into an unpassable tile
				// - Ground vehicle fell into a tile which has different resting height and vehicle
				// height
				//   (which is something like a tunnel or terminus at base)
				if (!vehicle.type->isGround() || atvMode == SceneryTileType::WalkMode::None ||
				    (vehicle.type->type == VehicleType::Type::Road &&
				     presentScenery->type->tile_type != SceneryTileType::TileType::Road) ||
				    vehicle.position.z != tile->getRestingPosition(false, true).z)
				{
					vehicle.sliding = true;
					vehicle.goalWaypoints.clear();
				}
				// Fell into passable tile -> will move to resting position
				else
				{
					// Stop residual movement
					vehicle.velocity = {0.0f, 0.0f, 0.0f};
					vehicle.angularVelocity = 0.0f;
				}
				break;
			}
		}
	}

	vehicle.updateSprite(state);
}

void VehicleMover::updateCrashed(GameState &state, unsigned int ticks [[maybe_unused]])
{
	// Tile underneath us is dead?
	if (vehicle.tileObject && vehicle.tileObject->getOwningTile() &&
	    vehicle.tileObject->getOwningTile()->presentScenery)
	{
		auto presentScenery = vehicle.tileObject->getOwningTile()->presentScenery;
		if (!presentScenery)
		{
			vehicle.setCrashed(state, false);
			vehicle.startFalling(state);
		}
	}
}

void VehicleMover::updateSliding(GameState &state, unsigned int ticks)
{
	// Slided off?
	auto presentScenery =
	    vehicle.tileObject ? vehicle.tileObject->getOwningTile()->presentScenery : nullptr;
	if (!presentScenery)
	{
		vehicle.sliding = false;
		vehicle.startFalling(state);
		return;
	}

	auto crashTicksRemaining = ticks;

	auto &map = *vehicle.city->map;

	if (vehicle.angularVelocity != 0)
	{
		vehicle.facing += vehicle.angularVelocity * (float)ticks;
		if (vehicle.facing < 0.0f)
		{
			vehicle.facing += M_2xPI;
		}
		if (vehicle.facing >= M_2xPI)
		{
			vehicle.facing -= M_2xPI;
		}
	}

	while (crashTicksRemaining-- > 0)
	{
		auto newPosition = vehicle.position;

		// Random doodads 2% chance if low health
		if (vehicle.getMaxHealth() / vehicle.getHealth() >= 3 &&
		    randBoundsExclusive(state.rng, 0, 100) < 2)
		{
			UString doodadId = randBool(state.rng) ? "DOODAD_1_AUTOCANNON" : "DOODAD_2_AIRGUARD";
			auto doodadPos = vehicle.position;
			doodadPos.x += (float)randBoundsInclusive(state.rng, -3, 3) / 10.0f;
			doodadPos.y += (float)randBoundsInclusive(state.rng, -3, 3) / 10.0f;
			doodadPos.z += (float)randBoundsInclusive(state.rng, -3, 3) / 10.0f;
			vehicle.city->placeDoodad({&state, doodadId}, doodadPos);
			fw().soundBackend->playSample(state.city_common_sample_list->vehicleExplosion,
			                              vehicle.position, 0.25f);
		}

		vehicle.velocity.x = reduceAbsValue(vehicle.velocity.x, FV_ACCELERATION);
		vehicle.velocity.y = reduceAbsValue(vehicle.velocity.y, FV_ACCELERATION);
		if (vehicle.velocity.x == 0.0f && vehicle.velocity.y == 0.0f)
		{
			vehicle.angularVelocity = 0.0f;
			vehicle.sliding = false;
			vehicle.crash(state, nullptr);
			vehicle.updateSprite(state);
			break;
		}

		newPosition += vehicle.velocity / (float)TICK_SCALE / VELOCITY_SCALE_BATTLE;

		// Fell outside map
		if (!map.tileIsValid(newPosition))
		{
			vehicle.die(state, false, nullptr);
			return;
		}

		// If moved do a different tile it must not have higher resting position than us and be
		// valid
		if ((Vec3<int>)newPosition != (Vec3<int>)vehicle.position)
		{
			// If no scenery in toTile that means we've slided off
			auto toTile = map.getTile(newPosition);
			if (!toTile->presentScenery)
			{
				vehicle.setPosition(newPosition);
				vehicle.updateSprite(state);
				vehicle.sliding = false;
				vehicle.startFalling(state);
				return;
			}
			// Expecting to have scenery in fromTile as checked above
			auto fromTile = map.getTile(newPosition);

			auto fromATVMode = fromTile->presentScenery->type->getATVMode();
			auto toATVMode = toTile->presentScenery->type->getATVMode();
			switch (fromATVMode)
			{
				case SceneryTileType::WalkMode::Into:
					// Bumped into something, stop
					if (toATVMode != SceneryTileType::WalkMode::Into)
					{
						// Stop moving and cancel this tick movement (will crash on next update)
						vehicle.velocity = {0.0f, 0.0f, 0.0f};
						continue;
					}
					break;
				case SceneryTileType::WalkMode::None:
				case SceneryTileType::WalkMode::Onto:
					// Went from high enough onto/none to into, falling
					if (toATVMode == SceneryTileType::WalkMode::Into &&
					    newPosition.z - floorf(newPosition.z) > 0.15f)
					{
						vehicle.setPosition(newPosition);
						vehicle.updateSprite(state);
						vehicle.sliding = false;
						vehicle.startFalling(state);
						return;
					}
					// Otherwise see that nothing blocks us
					auto upPos = newPosition;
					upPos.z += 1.0f;
					if (map.tileIsValid(upPos) && map.getTile(upPos)->presentScenery)
					{
						// Stop moving and cancel this tick movement (will crash on next update)
						vehicle.velocity = {0.0f, 0.0f, 0.0f};
						continue;
					}
					break;
			}
		}
		vehicle.setPosition(newPosition);
	}
	vehicle.updateSprite(state);
}

VehicleMover::~VehicleMover() = default;

Vehicle::~Vehicle() = default;

void Vehicle::leaveDimensionGate(GameState &state)
{
	// No portals to leave from. return here
	if (city->portals.empty())
	{
		return;
	}
	auto portal = city->portals.begin();
	std::uniform_int_distribution<int> portal_rng(0, city->portals.size() - 1);
	std::advance(portal, portal_rng(state.rng));
	auto initialPosition = (*portal)->getPosition();
	auto initialFacing = 0.0f;

	LogInfo("Leaving dimension gate %s", this->name);
	LogAssert(this->betweenDimensions == true);
	if (this->tileObject)
	{
		LogError("Trying to launch already-launched vehicle");
		return;
	}
	if (this->currentBuilding)
	{
		LogError("Vehicle leaving dimension gate from a building?");
		return;
	}
	this->position = initialPosition;
	this->goalPosition = initialPosition;
	this->facing = initialFacing;
	this->goalFacing = initialFacing;
	if (city->map)
	{
		city->map->addObjectToMap(state, shared_from_this());
	}
	if (state.current_city == city)
	{
		fw().soundBackend->playSample(state.city_common_sample_list->dimensionShiftOut, position);
		if (owner == state.getAliens())
		{
			fw().pushEvent(
			    new GameVehicleEvent(GameEventType::UfoSpotted, {&state, shared_from_this()}));
		}
	}
	this->betweenDimensions = false;
}

void Vehicle::enterDimensionGate(GameState &state)
{
	LogAssert(this->betweenDimensions == false);
	carriedByVehicle.clear();
	crashed = false;
	if (this->currentBuilding)
	{
		LogError("Vehicle entering dimension gate from a building?");
		return;
	}
	if (carriedVehicle)
	{
		dropCarriedVehicle(state);
	}
	removeFromMap(state);
	if (state.current_city == city)
	{
		fw().soundBackend->playSample(state.city_common_sample_list->dimensionShiftIn, position);
	}
	this->position = {-9001, -9001, -9001};
	this->facing = 0.0f;
	this->goalFacing = 0.0f;
	this->ticksToTurn = 0;
	this->angularVelocity = 0.0f;
	this->betweenDimensions = true;
}

void Vehicle::leaveBuilding(GameState &state, Vec3<float> initialPosition, float initialFacing)
{
	LogInfo("Launching %s", this->name);
	if (this->tileObject)
	{
		LogError("Trying to launch already-launched vehicle");
		return;
	}
	auto bld = this->currentBuilding;
	if (bld)
	{
		bld->currentVehicles.erase({&state, shared_from_this()});
		this->currentBuilding = "";
	}
	this->position = initialPosition;
	this->goalPosition = initialPosition;
	this->facing = initialFacing;
	this->goalFacing = initialFacing;
	if (city->map)
	{
		city->map->addObjectToMap(state, shared_from_this());
	}
}

void Vehicle::enterBuilding(GameState &state, StateRef<Building> b)
{
	carriedByVehicle.clear();
	crashed = false;
	if (this->currentBuilding)
	{
		LogError("Vehicle already in a building?");
		return;
	}
	this->currentBuilding = b;
	b->currentVehicles.insert({&state, shared_from_this()});
	if (carriedVehicle)
	{
		carriedVehicle->enterBuilding(state, b);
		carriedVehicle->processRecoveredVehicle(state);
		carriedVehicle.clear();
	}

	removeFromMap(state);

	this->position = type->isGround() ? b->carEntranceLocation : *b->landingPadLocations.begin();
	this->position += Vec3<float>{0.5f, 0.5f, 0.5f};
	this->facing = 0.0f;
	this->goalFacing = 0.0f;
	this->ticksToTurn = 0;
	this->angularVelocity = 0.0f;
	// If spent some time outside then spend a unit of fuel
	if (fuelSpentTicks > 0 && currentBuilding == homeBuilding)
	{
		fuelSpentTicks = 0;
		auto engine = getEngine();
		if (engine && engine->type->max_ammo > 0)
		{
			if (engine->ammo > 0)
			{
				engine->ammo--;
			}
		}
	}
}

void Vehicle::setupMover()
{
	if (type->isGround())
	{
		this->mover.reset(new GroundVehicleMover(*this));
	}
	else
	{
		this->mover.reset(new FlyingVehicleMover(*this));
	}
	animationDelay = 0;
	animationFrame = type->animation_sprites.begin();
}

/**
 * Remove all tile objects that belongs to vehicle.
 */
void Vehicle::removeFromMap(GameState &state)
{
	if (smokeDoodad)
	{
		smokeDoodad->remove(state);
		smokeDoodad = nullptr;
	}
	if (shadowObject)
	{
		shadowObject->removeFromMap();
		shadowObject = nullptr;
	}
	if (tileObject)
	{
		tileObject->removeFromMap();
		tileObject = nullptr;
	}
}

/**
 * Set the vehicle crashed (or not).
 */
void Vehicle::setCrashed(GameState &state, bool crashed)
{
	if (smokeDoodad)
	{
		smokeDoodad->remove(state);
		smokeDoodad = nullptr;
	}
	if (crashed)
	{
		sp<Doodad> smoke = mksp<Doodad>(position + SMOKE_DOODAD_SHIFT,
		                                StateRef<DoodadType>{&state, "DOODAD_13_SMOKE_FUME"});
		city->map->addObjectToMap(smoke);
		smokeDoodad = smoke;
	}
	this->crashed = crashed;
}

void Vehicle::processRecoveredVehicle(GameState &state)
{
	std::list<sp<VEquipment>> scrappedEquipment;
	for (auto &e : equipment)
	{
		if (randBoundsExclusive(state.rng, 0, 100) >= FV_CHANCE_TO_RECOVER_EQUIPMENT)
		{
			scrappedEquipment.push_back(e);
		}
	}
	for (auto &e : scrappedEquipment)
	{
		removeEquipment(e);
		if (state.economy.find(e->type.id) != state.economy.end())
		{
			auto &economy = state.economy[e->type.id];
			owner->balance += economy.currentPrice * FV_SCRAPPED_COST_PERCENT / 100;
		}
		if (e->ammo > 0 && e->type->ammo_type &&
		    state.economy.find(e->type->ammo_type.id) != state.economy.end())
		{
			auto &economy = state.economy[e->type->ammo_type.id];
			owner->balance += e->ammo * economy.currentPrice * FV_SCRAPPED_COST_PERCENT / 100;
		}
	}
	if (randBoundsExclusive(state.rng, 0, 100) > FV_CHANCE_TO_RECOVER_VEHICLE)
	{
		while (!currentAgents.empty())
		{
			auto agent = *currentAgents.begin();
			agent->enterBuilding(state, currentBuilding);
		}
		if (currentBuilding->base)
		{
			// Base, de-equip
			for (auto &e : equipment)
			{
				e->unequipToBase(state, currentBuilding->base);
			}
		}
		else
		{
			// No base, sell
			for (auto &e : equipment)
			{
				int price = 0;
				if (state.economy.find(e->type.id) != state.economy.end())
				{
					auto &economy = state.economy[e->type.id];
					price = economy.currentPrice;
				}
				owner->balance += price * FV_SCRAPPED_COST_PERCENT / 100;
				if (e->ammo > 0)
				{
					price = 0;
					if (state.economy.find(e->type->ammo_type.id) != state.economy.end())
					{
						auto &economy = state.economy[e->type->ammo_type.id];
						price = economy.currentPrice;
					}
					owner->balance += e->ammo * price * FV_SCRAPPED_COST_PERCENT / 100;
				}
			}
		}
		int price = 0;
		if (state.economy.find(type.id) != state.economy.end())
		{
			auto &economy = state.economy[type.id];
			price = economy.currentPrice;
		}
		owner->balance += price * FV_SCRAPPED_COST_PERCENT / 100;
		if (owner == state.getPlayer())
		{
			fw().pushEvent(
			    new GameSomethingDiedEvent(GameEventType::VehicleRecovered, name, "", position));
		}
		die(state, true);
	}
	else
	{
		if (owner == state.getPlayer())
		{
			fw().pushEvent(new GameVehicleEvent(GameEventType::VehicleRecovered,
			                                    {&state, shared_from_this()}));
		}
	}
}

void Vehicle::dropCarriedVehicle(GameState &state)
{
	carriedVehicle->crashed = false;
	carriedVehicle->startFalling(state);
	carriedVehicle->carriedByVehicle.clear();
	carriedVehicle.clear();
}

void Vehicle::provideService(GameState &state, bool otherOrg)
{
	if (!currentBuilding)
	{
		LogError("Called provideService when not in building, wtf?");
		return;
	}
	bool agentPriority = type->provideFreightAgent;
	if (agentPriority)
	{
		provideServicePassengers(state, otherOrg);
		if (type->provideFreightBio || !otherOrg)
		{
			provideServiceCargo(state, true, otherOrg);
		}
		if (type->provideFreightCargo || !otherOrg)
		{
			provideServiceCargo(state, false, otherOrg);
		}
	}
	else
	{
		if (type->provideFreightBio || !otherOrg)
		{
			provideServiceCargo(state, true, otherOrg);
		}
		if (type->provideFreightCargo || !otherOrg)
		{
			provideServiceCargo(state, false, otherOrg);
		}
		if (type->provideFreightAgent || !otherOrg)
		{
			provideServicePassengers(state, otherOrg);
		}
	}
}

void Vehicle::provideServiceCargo(GameState &state, bool bio, bool otherOrg)
{
	StateRef<Building> destination = getServiceDestination(state);
	int spaceRemaining = bio ? getMaxBio() - getBio() : getMaxCargo() - getCargo();
	for (auto &c : currentBuilding->cargo)
	{
		// No space left
		if (spaceRemaining == 0)
		{
			break;
		}
		// Cargo spent
		if (c.count == 0)
		{
			continue;
		}
		// Won't ferry other orgs
		if (c.destination->owner != owner && !otherOrg)
		{
			continue;
		}
		// Won't ferry because dislikes
		if (otherOrg &&
		    (config().getBool("OpenApoc.NewFeature.FerryChecksRelationshipWhenBuying") ||
		     c.cost == 0))
		{
			if (owner->isRelatedTo(c.destination->owner) == Organisation::Relation::Hostile)
			{
				continue;
			}
			if (c.originalOwner &&
			    owner->isRelatedTo(c.originalOwner) == Organisation::Relation::Hostile)
			{
				continue;
			}
		}
		// Won't ferry different kind of cargo
		if ((c.type == Cargo::Type::Bio) != bio)
		{
			continue;
		}
		// Won't ferry if already picked destination and doesn't match
		if (destination && c.destination != destination)
		{
			continue;
		}
		// How much can we pick up
		int maxAmount = std::min(spaceRemaining / c.space * c.divisor, c.count);
		if (maxAmount == 0)
		{
			continue;
		}
		// Here's where we're going
		if (!destination)
		{
			destination = c.destination;
		}
		// Split cargo and load up
		auto newCargo = c;
		newCargo.count = maxAmount;
		c.count -= maxAmount;
		cargo.push_back(newCargo);
		spaceRemaining -= maxAmount * c.space / c.divisor;
	}
}

void Vehicle::provideServicePassengers(GameState &state, bool otherOrg)
{
	StateRef<Building> destination = getServiceDestination(state);
	int spaceRemaining = getMaxPassengers() - getPassengers();
	bool pickedUpPassenger = false;
	do
	{
		pickedUpPassenger = false;
		for (auto a : currentBuilding->currentAgents)
		{
			// No space left
			if (spaceRemaining == 0)
			{
				break;
			}
			// Agent doesn't want pickup
			if (a->missions.empty() ||
			    a->missions.front()->type != AgentMission::MissionType::AwaitPickup)
			{
				continue;
			}
			// Won't ferry other orgs
			if (a->missions.front()->targetBuilding->owner != owner && !otherOrg)
			{
				continue;
			}
			// Won't ferry because dislikes
			if (otherOrg && owner->isRelatedTo(a->owner) == Organisation::Relation::Hostile)
			{
				continue;
			}
			// Won't ferry if already picked destination and doesn't match
			if (destination && a->missions.front()->targetBuilding != destination)
			{
				continue;
			}
			// Here's where we're going
			if (!destination)
			{
				destination = a->missions.front()->targetBuilding;
			}
			// Load up
			a->enterVehicle(state, {&state, shared_from_this()});
			spaceRemaining--;
			pickedUpPassenger = true;
			break;
		}
	} while (pickedUpPassenger);
}

StateRef<Building> Vehicle::getServiceDestination(GameState &state)
{
	bool cargoArrived = false;
	bool bioArrived = false;
	bool recoveryArrived = false;
	bool transferArrived = false;
	std::set<StateRef<Organisation>> suppliers;
	StateRef<Building> destination;

	// Step 01: Find first cargo destination and remove arrived cargo
	for (auto it = cargo.begin(); it != cargo.end();)
	{
		if (it->destination == currentBuilding)
		{
			it->arrive(state, cargoArrived, bioArrived, recoveryArrived, transferArrived,
			           suppliers);
			it = cargo.erase(it);
		}
		else
		{
			// Only force-ferry non-loot cargoes
			if (it->count != 0 && !destination && it->originalOwner)
			{
				destination = it->destination;
			}
			it++;
		}
	}
	// Step 02: Find first agent destination and remove arrived agents
	std::list<StateRef<Agent>> agentsToRemove;
	for (auto a : currentAgents)
	{
		// Remove agents if wounded and this is their home base
		if (a->modified_stats.health < a->current_stats.health &&
		    a->homeBuilding == currentBuilding)
		{
			agentsToRemove.push_back(a);
		}
		// Skip agents that are just on this craft without a mission
		if (a->missions.empty() ||
		    a->missions.front()->type != AgentMission::MissionType::AwaitPickup)
		{
			continue;
		}
		if (a->missions.front()->targetBuilding == currentBuilding)
		{
			agentsToRemove.push_back(a);
			continue;
		}
		if (!destination)
		{
			destination = a->missions.front()->targetBuilding;
		}
	}
	for (auto &a : agentsToRemove)
	{
		a->enterBuilding(state, currentBuilding);
	}

	// Step 03: Arrival events
	// Transfer
	if (transferArrived)
	{
		fw().pushEvent(new GameBaseEvent(GameEventType::TransferArrived, currentBuilding->base,
		                                 nullptr, false));
	}
	if (bioArrived)
	{
		fw().pushEvent(new GameBaseEvent(GameEventType::TransferArrived, currentBuilding->base,
		                                 nullptr, true));
	}
	// Loot
	if (recoveryArrived)
	{
		fw().pushEvent(new GameBaseEvent(GameEventType::RecoveryArrived, currentBuilding->base));
	}
	// Purchase
	if (cargoArrived)
	{
		for (auto &o : suppliers)
		{
			fw().pushEvent(
			    new GameBaseEvent(GameEventType::CargoArrived, currentBuilding->base, o));
		}
	}
	return destination;
}

void Vehicle::die(GameState &state, bool silent, StateRef<Vehicle> attacker)
{
	health = 0;
	if (!silent)
	{
		if (this->tileObject)
		{
			auto doodad = city->placeDoodad(StateRef<DoodadType>{&state, "DOODAD_3_EXPLOSION"},
			                                this->tileObject->getCenter());
			fw().soundBackend->playSample(state.city_common_sample_list->vehicleExplosion,
			                              position);
		}
		else
		{
			LogWarning("Tileobject is nullpointer");
		}
	}
	auto id = getId(state, shared_from_this());
	if (carriedVehicle)
	{
		dropCarriedVehicle(state);
	}
	if (carriedByVehicle)
	{
		carriedByVehicle->carriedVehicle.clear();
		carriedByVehicle.clear();
	}
	// Clear projectiles
	for (auto &p : city->projectiles)
	{
		if (p->trackedVehicle && p->trackedVehicle.id == id)
		{
			p->turnRate = 0;
			p->trackedVehicle.clear();
			p->trackedObject = nullptr;
		}
	}
	// Clear targets
	for (auto &v : state.vehicles)
	{
		for (auto &m : v.second->missions)
		{
			if (m->targetVehicle.id == id)
			{
				m->targetVehicle.clear();
			}
			for (auto it = m->targets.begin(); it != m->targets.end();)
			{
				if ((*it).id == id)
				{
					it = m->targets.erase(it);
				}
				else
				{
					it++;
				}
			}
		}
	}
	removeFromMap(state);

	// Dying will remove agent from current agents list
	for (auto agent : currentAgents)
	{
		agent->die(state, true);
	}

	// Adjust relationships
	if (attacker && !crashed)
	{
		adjustRelationshipOnDowned(state, attacker);
	}

	if (!silent && city == state.current_city)
	{
		fw().pushEvent(new GameSomethingDiedEvent(GameEventType::VehicleDestroyed, name,
		                                          attacker ? attacker->name : "", position));
	}
	state.vehiclesDeathNote.insert(id);
}

void Vehicle::crash(GameState &state, StateRef<Vehicle> attacker)
{
	// Dislike attacker
	if (attacker)
	{
		adjustRelationshipOnDowned(state, attacker);
	}
	// Drop carried vehicle
	if (carriedVehicle)
	{
		dropCarriedVehicle(state);
	}
	// Actually crash
	crashed = true;
	health = std::min(health, (type->crash_health > 0) ? type->crash_health : type->health / 10);
	switch (type->type)
	{
		case VehicleType::Type::UFO:
		{
			setMission(state, VehicleMission::crashLand(state, *this));
			addMission(state, VehicleMission::selfDestruct(state, *this), true);
			break;
		}
		case VehicleType::Type::Flying:
		case VehicleType::Type::ATV:
		case VehicleType::Type::Road:
		{
			bool found = false;
			for (auto &m : missions)
			{
				if (m->type == VehicleMission::MissionType::SelfDestruct)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				setMission(state, VehicleMission::selfDestruct(state, *this));
			}
			break;
		}
	}
}

void Vehicle::startFalling(GameState &state, StateRef<Vehicle> attacker)
{
	// Dislike attacker
	if (attacker)
	{
		adjustRelationshipOnDowned(state, attacker);
	}
	// Drop carried vehicle
	if (carriedVehicle)
	{
		dropCarriedVehicle(state);
	}
	// Actually start falling
	falling = true;
	if (angularVelocity == 0.0f)
	{
		float vel = getSpeed() * (float)M_PI / (float)TICK_SCALE / VELOCITY_SCALE_CITY.x / 1.5f;

		switch (randBoundsInclusive(state.rng, -1, 1))
		{
			case -1:
				angularVelocity = -vel;
				break;
			case 0:
				break;
			case 1:
				angularVelocity = vel;
				break;
		}
	}
}

void Vehicle::adjustRelationshipOnDowned(GameState &state, StateRef<Vehicle> attacker)
{
	// Give score if downing alien craft
	if (attacker->owner == state.getPlayer() && owner == state.getAliens())
	{
		state.totalScore.craftShotDownUFO += type->score;
		state.weekScore.craftShotDownUFO += type->score;
	}
	// Subtract score if x-com craft
	else if (owner == state.getPlayer())
	{
		state.totalScore.craftShotDownXCom -= type->score;
		state.weekScore.craftShotDownXCom -= type->score;
	}
	// If we're hostile to attacker - lose 5 points
	if (owner->isRelatedTo(attacker->owner) == Organisation::Relation::Hostile)
	{
		owner->adjustRelationTo(state, attacker->owner, -5.0f);
	}
	// If we're not hostile to attacker - lose 30 points
	else
	{
		owner->adjustRelationTo(state, attacker->owner, -30.0f);
	}
	// Our allies lose 15 points, enemies gain 5 points
	// Otherwise 20+ relationship is +-3 points, 10+ is +-1 points
	for (auto &org : state.organisations)
	{
		if (org.first != attacker->owner.id && org.first != state.getCivilian().id)
		{
			if (org.second->isRelatedTo(owner) == Organisation::Relation::Hostile)
			{
				org.second->adjustRelationTo(state, attacker->owner, 5.0f);
			}
			else if (org.second->isRelatedTo(owner) == Organisation::Relation::Allied)
			{
				org.second->adjustRelationTo(state, attacker->owner, -15.0f);
			}
			else
			{
				auto rel = org.second->getRelationTo(owner);
				if (rel > 20.0f)
				{
					org.second->adjustRelationTo(state, attacker->owner, -3.0f);
				}
				else if (rel > 10.0f)
				{
					org.second->adjustRelationTo(state, attacker->owner, -1.0f);
				}
				else if (rel < -10.0f)
				{
					org.second->adjustRelationTo(state, attacker->owner, 1.0f);
				}
				else if (rel < -20.0f)
				{
					org.second->adjustRelationTo(state, attacker->owner, 3.0f);
				}
			}
		}
	}
}

bool Vehicle::isDead() const { return health <= 0; }

Vec3<float> Vehicle::getMuzzleLocation() const
{
	return type->isGround()
	           ? Vec3<float>(position.x, position.y, position.z + (float)type->height / 16.0f)
	           : Vec3<float>(position.x, position.y,
	                         position.z - tileObject->getVoxelOffset().z +
	                             (float)type->height / 16.0f);
}

void Vehicle::update(GameState &state, unsigned int ticks)
{
	if (isDead() && status == VehicleStatus::Operational)
	{
		status = VehicleStatus::Destroyed;

		// Remove from building
		if (currentBuilding)
		{
			currentBuilding->currentVehicles.erase({&state, shared_from_this()});
		}
	}

	if (isDead())
	{
		return;
	}

	bool turbo = ticks > TICKS_PER_SECOND;
	bool IsIdle;

	if (stunTicksRemaining >= ticks)
	{
		stunTicksRemaining -= ticks;
		return;
	}
	else if (stunTicksRemaining > 0)
	{
		ticks -= stunTicksRemaining;
		stunTicksRemaining = 0;
	}

	if (cloakTicksAccumulated < CLOAK_TICKS_REQUIRED_VEHICLE)
	{
		cloakTicksAccumulated += ticks;
	}
	if (!hasCloak())
	{
		cloakTicksAccumulated = 0;
	}
	if (teleportTicksAccumulated < TELEPORT_TICKS_REQUIRED_VEHICLE)
	{
		teleportTicksAccumulated += ticks;
	}
	if (!hasTeleporter())
	{
		teleportTicksAccumulated = 0;
	}

	IsIdle = this->missions.empty();
	if (!IsIdle)
	{
		this->missions.front()->update(state, *this, ticks);
	}

	popFinishedMissions(state);

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

	// Moving
	if (this->mover)
	{
		this->mover->update(state, ticks);
	}

	// Animation and firing (not on turbo)
	auto vehicleTile = this->tileObject;
	if (vehicleTile && !turbo)
	{
		if (!this->type->animation_sprites.empty())
		{
			nextFrame(ticks);
		}

		// Vehicles that are taking off or landing don't attempt to fire
		bool attemptFire = true;
		if (!type->isGround() && !missions.empty())
		{
			if (missions.front()->type == VehicleMission::MissionType::TakeOff ||
			    missions.front()->type == VehicleMission::MissionType::Land)
			{
				attemptFire = false;
			}
		}
		if (attemptFire)
		{
			bool has_active_weapon = false;
			bool has_active_pd = false;
			Vec2<int> arc = {0, 0};
			Vec2<int> arcPD = {0, 0};
			for (auto &equipment : this->equipment)
			{
				if (equipment->type->type != EquipmentSlotType::VehicleWeapon)
					continue;
				equipment->update(ticks);
				if (!crashed && !falling && this->attackMode != Vehicle::AttackMode::Evasive &&
				    equipment->canFire())
				{
					has_active_weapon = true;
					if (arc.x < equipment->type->firing_arc_1)
					{
						// FIXME: Are vertical firing arcs actually working in vanilla? I think
						// not..
						arc = {equipment->type->firing_arc_1, equipment->type->firing_arc_2};
					}
					has_active_pd = equipment->type->point_defence;
					if (has_active_pd && arcPD.x < equipment->type->firing_arc_1)
					{
						// FIXME: Are vertical firing arcs actually working in vanilla? I think
						// not..
						arcPD = {equipment->type->firing_arc_1, equipment->type->firing_arc_2};
					}
				}
			}

			if (has_active_weapon)
			{
				// First fire where told to manually
				if (manualFire)
				{
					fireWeaponsManual(state, arc);
				}
				// Try firing point defense weapons
				else if (!has_active_pd || !fireWeaponsPointDefense(state, arcPD))
				{
					// Fire at building (if ordered to)
					if (!fireAtBuilding(state, arc))
					{
						// If we don't fire at buildings then try to fire at enemy (if ordered to)
						auto firingRange = getFiringRange();
						sp<TileObjectVehicle> enemy;
						if (!missions.empty() &&
						    missions.front()->type == VehicleMission::MissionType::AttackVehicle &&
						    tileObject->getDistanceTo(
						        missions.front()->targetVehicle->tileObject) <= firingRange)
						{
							enemy = missions.front()->targetVehicle->tileObject;
							if (enemy)
							{
								attackTarget(state, enemy);
							}
						}
						else
						{
							// No orders. Must think ourselves. Find what we can fire at.
							enemy = findClosestEnemy(state, tileObject, arc);
							if (enemy)
							{
								// Try to attack. Attack won't happen if enemy is out of range.
								attackTarget(state, enemy);
							}
							// Search for closest target to face towards (if not already rotating
							// for some reason)
							if (IsIdle && this->angularVelocity == 0.0f)
							{
								// If not moving anywhere then search for closest target around.
								enemy = findClosestEnemy(state, tileObject, {8, 8});
								if (enemy)
								{
									// Determine target position.
									const Vec3<float> enemypos = enemy->getPosition();
									const Vec2<float> facing2d = facingToXY(this->facing);
									const Vec2<float> target2d = glm::normalize(Vec2<float>{
									    enemypos.x - position.x, enemypos.y - position.y});
									const float angleXY = glm::angle(facing2d, target2d);
									// Check if target is in our firing arc.
									if (angleXY > (float)arc.x * (float)M_PI / 8.0f)
									{
										this->goalFacing = xyToFacing(target2d);
										float d = facingDistance(this->goalFacing, this->facing);
										// TODO: Should this nudge CCW/CW for animation purposes?
										if (d > 0.0f)
										{
											// Rotate CW towards goal
											this->angularVelocity =
											    std::min(this->getAngularSpeed(), d);
										}
										else
										{
											// Rotate CCW towards goal
											this->angularVelocity =
											    std::max(-this->getAngularSpeed(), d);
										}
										this->ticksToTurn = floorf(d / this->angularVelocity);
										this->updateSprite(state);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	manualFire = false;
}

void Vehicle::updateEachSecond(GameState &state)
{
	// Consume fuel if out in city
	if (tileObject && !crashed && !falling && !sliding)
	{
		// Only consume fuel if flying or moving (parked ground vehicles don't consume fuel)
		if (!this->isIdle() || !this->type->isGround())
		{
			fuelSpentTicks += FUEL_TICKS_PER_SECOND;
		}
		if (fuelSpentTicks > FUEL_TICKS_PER_UNIT)
		{
			fuelSpentTicks -= FUEL_TICKS_PER_UNIT;
			sp<VEquipment> engine = getEngine();
			if (engine && engine->type->max_ammo > 0)
			{
				if (engine->ammo > 0)
				{
					engine->ammo--;
				}
				// Low fuel
				if (engine->ammo == 2)
				{
					if (owner == state.getPlayer())
					{
						fw().pushEvent(new GameVehicleEvent(GameEventType::VehicleLowFuel,
						                                    {&state, shared_from_this()}));
					}
				}
				// Out of fuel, drop
				if (engine->ammo == 0)
				{
					if (config().getBool("OpenApoc.NewFeature.CrashingOutOfFuel"))
					{
						if (owner == state.getPlayer())
						{
							fw().pushEvent(new GameVehicleEvent(GameEventType::VehicleNoFuel,
							                                    {&state, shared_from_this()}));
						}
						if (type->isGround())
						{
							crash(state, nullptr);
						}
						else
						{
							startFalling(state);
						}
					}
					else
					{
						if (owner == state.getPlayer())
						{
							fw().pushEvent(new GameSomethingDiedEvent(GameEventType::VehicleNoFuel,
							                                          name, "", position));
						}
						die(state, true);
					}
				}
			}
		}
	}
	// Update cargo
	updateCargo(state);
	// Automated missions
	if (missions.empty() && !currentBuilding && owner != state.getPlayer())
	{
		if (owner == state.getAliens())
		{
			if (city.id == "CITYMAP_HUMAN")
			{
				setMission(state, VehicleMission::gotoPortal(state, *this));
			}
			else // Alien city
			{
				setMission(state, VehicleMission::patrol(state, *this));
			}
		}
		else
		{
			setMission(state, VehicleMission::gotoBuilding(state, *this));
		}
	}
}

void Vehicle::updateCargo(GameState &state)
{
	if (crashed || falling)
	{
		return;
	}
	// Cannot order to ferry if aggressive and in city
	if (!currentBuilding && attackMode == AttackMode::Aggressive)
	{
		return;
	}
	// Already ferrying
	if (!missions.empty() && missions.back()->type == VehicleMission::MissionType::OfferService)
	{
		return;
	}
	// See if need to ferry
	bool needFerry = false;
	for (auto &c : cargo)
	{
		// Either this is non-combat loot, or this loot belongs to this building
		// Don't keep trying to ferry combat loot to other building as we're NOT going to move
		// when we check it in getServiceDestination method!
		if (c.originalOwner || c.destination == currentBuilding)
		{
			needFerry = true;
			break;
		}
	}
	if (!needFerry)
	{
		for (auto &a : currentAgents)
		{
			if (!a->missions.empty() &&
			    a->missions.front()->type == AgentMission::MissionType::AwaitPickup)
			{
				needFerry = true;
				break;
			}
		}
	}
	if (needFerry)
	{
		setMission(state, VehicleMission::offerService(state, *this));
	}
}

void Vehicle::updateSprite(GameState &state [[maybe_unused]])
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
	else if ((velocity.x != 0.0f || velocity.y != 0.0f) && velocity.z > 0.0f)
	{
		banking = VehicleType::Banking::Ascending;
	}
	else if ((velocity.x != 0.0f || velocity.y != 0.0f) && velocity.z < 0.0f)
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
			direction = VehicleType::getDirectionLarge(facing);
			// UFOs don't care about banking and direction being correct
			// Otherwise ensure direction is valid
			if (type->type == VehicleType::Type::UFO)
			{
				break;
			}
			if (type->directional_sprites.at(banking).find(direction) !=
			    type->directional_sprites.at(banking).end())
			{
				break;
			}
		// Fall-through since this direction is not valid
		case VehicleType::Banking::Ascending:
		case VehicleType::Banking::Descending:
		case VehicleType::Banking::Flat:
			direction = VehicleType::getDirectionSmall(facing);
			// UFOs don't care about banking and direction being correct
			// Otherwise ensure direction is valid
			if (type->type == VehicleType::Type::UFO)
			{
				break;
			}
			// If still invalid we must cancel banking (can happen for grounds)
			if (type->directional_sprites.at(banking).find(direction) !=
			    type->directional_sprites.at(banking).end())
			{
				break;
			}
			banking = VehicleType::Banking::Flat;
			break;
	}

	// Set shadow direction
	shadowDirection = direction;
	if (!type->directional_shadow_sprites.empty() &&
	    type->directional_shadow_sprites.find(shadowDirection) ==
	        type->directional_shadow_sprites.end())
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
				shadowDirection = VehicleType::getDirectionSmall(facing);
				if (type->directional_shadow_sprites.find(shadowDirection) !=
				    type->directional_shadow_sprites.end())
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

sp<VEquipment> Vehicle::getEngine() const
{
	for (auto &e : equipment)
	{
		if (e->type->type == EquipmentSlotType::VehicleEngine)
		{
			return e;
		}
	}
	return nullptr;
}

bool Vehicle::hasEngine() const
{
	bool hasSlot = false;
	for (auto &s : type->equipment_layout_slots)
	{
		if (s.type == EquipmentSlotType::VehicleEngine)
		{
			hasSlot = true;
			break;
		}
	}
	if (hasSlot)
	{
		return (bool)getEngine();
	}
	else
	{
		return true;
	}
}

bool Vehicle::applyDamage(GameState &state, int damage, float armour)
{
	bool soundHandled = false;
	return applyDamage(state, damage, armour, soundHandled, nullptr);
}

bool Vehicle::applyDamage(GameState &state, int damage, float armour, bool &soundHandled,
                          StateRef<Vehicle> attacker)
{
	if (this->owner == state.getPlayer())
	{
		damage = (double)damage * config().getFloat("OpenApoc.Cheat.DamageReceivedMultiplier");
	}
	if (attacker && attacker->owner == state.getPlayer())
	{
		damage = (double)damage * config().getFloat("OpenApoc.Cheat.DamageInflictedMultiplier");
	}

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
			bool wasBelowCrashThreshold = health <= type->crash_health;
			this->health -= damage;
			if (this->health <= 0)
			{
				die(state, false, attacker);
				soundHandled = true;
				return true;
			}
			else if (!wasBelowCrashThreshold && health <= type->crash_health && !crashed)
			{
				if (type->type == VehicleType::Type::UFO)
				{
					crash(state, attacker);
				}
				else
				{
					if (!falling)
					{
						startFalling(state, attacker);
					}
				}
				return false;
			}
		}
	}
	else
	{
		this->shield -= damage;
		if (config().getBool("OpenApoc.NewFeature.AlternateVehicleShieldSound"))
		{
			fw().soundBackend->playSample(state.city_common_sample_list->shieldHit, position);
			soundHandled = true;
		}
	}
	return false;
}

bool Vehicle::handleCollision(GameState &state, Collision &c, bool &soundHandled)
{
	if (!this->tileObject)
	{
		LogError("It's possible multiple projectiles hit the same tile in the same tick (?)");
		return false;
	}

	auto projectile = c.projectile.get();
	if (projectile)
	{
		stunTicksRemaining += projectile->stunTicks;
		auto vehicleDir = glm::round(type->directionToVector(direction));
		auto projectileDir = glm::normalize(projectile->velocity);
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

		return applyDamage(state, randDamage050150(state.rng, projectile->damage), armourValue,
		                   soundHandled, projectile->firerVehicle);
	}
	return false;
}

sp<TileObjectVehicle> Vehicle::findClosestEnemy(GameState &state, sp<TileObjectVehicle> vehicleTile,
                                                Vec2<int> arc)
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
		if (otherVehicle->crashed || otherVehicle->falling || otherVehicle->sliding)
		{
			// Can't auto-fire at crashed vehicles
			continue;
		}
		if (otherVehicle->type->aggressiveness == 0 && this->owner == state.getPlayer())
		{
			// No auto-acquiring of non-aggressive vehicles
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
		// Check firing arc
		if (type->type != VehicleType::Type::UFO && (arc.x < 8 || arc.y < 8))
		{
			auto facing = type->directionToVector(direction);
			auto vecToTarget = otherVehicleTile->getPosition() - position;
			float angleXY = glm::angle(glm::normalize(Vec2<float>{facing.x, facing.y}),
			                           glm::normalize(Vec2<float>{vecToTarget.x, vecToTarget.y}));
			float vecToTargetXY =
			    sqrtf(vecToTarget.x * vecToTarget.x + vecToTarget.y * vecToTarget.y);
			float angleZ = glm::angle(Vec2<float>{1.0f, 0.0f},
			                          glm::normalize(Vec2<float>{vecToTargetXY, vecToTarget.z}));
			if (angleXY > (float)arc.x * (float)M_PI / 8.0f ||
			    angleZ > (float)arc.y * (float)M_PI / 8.0f)
			{
				continue;
			}
		}
		// Finally add closest
		float distance = vehicleTile->getDistanceTo(otherVehicleTile);
		if (distance < closestEnemyRange)
		{
			closestEnemyRange = distance;
			closestEnemy = otherVehicleTile;
		}
	}
	return closestEnemy;
}

sp<TileObjectProjectile> Vehicle::findClosestHostileMissile(GameState &state,
                                                            sp<TileObjectVehicle> vehicleTile,
                                                            Vec2<int> arc)
{
	// Find the closest missile within the firing arc
	float closestEnemyRange = std::numeric_limits<float>::max();
	sp<TileObjectProjectile> closestEnemy;
	for (auto &projectile : state.current_city->projectiles)
	{
		// Can't shoot down projectiles w/o voxelMap
		if (!projectile->voxelMapLof)
		{
			continue;
		}
#ifndef DEBUG_ALLOW_PROJECTILE_ON_PROJECTILE_FRIENDLY_FIRE
		// Can't fire at friendly projectiles
		if (projectile->firerVehicle->owner == owner ||
		    owner->isRelatedTo(projectile->firerVehicle->owner) != Organisation::Relation::Hostile)
		{
			continue;
		}
#endif // ! DEBUG_ALLOW_PROJECTILE_ON_PROJECTILE_FRIENDLY_FIRE
       // Check firing arc
		if (type->type != VehicleType::Type::UFO && (arc.x < 8 || arc.y < 8))
		{
			auto facing = type->directionToVector(direction);
			auto vecToTarget = projectile->getPosition() - position;
			float angleXY = glm::angle(glm::normalize(Vec2<float>{facing.x, facing.y}),
			                           glm::normalize(Vec2<float>{vecToTarget.x, vecToTarget.y}));
			float vecToTargetXY =
			    sqrtf(vecToTarget.x * vecToTarget.x + vecToTarget.y * vecToTarget.y);
			float angleZ = glm::angle(Vec2<float>{1.0f, 0.0f},
			                          glm::normalize(Vec2<float>{vecToTargetXY, vecToTarget.z}));
			if (angleXY > (float)arc.x * (float)M_PI / 8.0f ||
			    angleZ > (float)arc.y * (float)M_PI / 8.0f)
			{
				continue;
			}
		}
		// Finally add closest
		float distance = vehicleTile->getDistanceTo(projectile->position);
		if (distance < closestEnemyRange)
		{
			closestEnemyRange = distance;
			closestEnemy = projectile->tileObject;
		}
	}
	return closestEnemy;
}

bool Vehicle::fireWeaponsPointDefense(GameState &state, Vec2<int> arc)
{
	// Find something to shoot at!
	sp<TileObjectProjectile> missile = findClosestHostileMissile(state, tileObject, arc);
	if (missile)
	{
		return attackTarget(state, missile);
	}
	return false;
}

bool Vehicle::fireAtBuilding(
    GameState &state,
    Vec2<int> arc [[maybe_unused]]) // TODO: this function must return target only, not fire
{
	auto firingRange = getFiringRange();

	// Attack buildings
	if (!missions.empty() && missions.front()->type == VehicleMission::MissionType::AttackBuilding)
	{
		auto target = missions.front()->targetBuilding;
		if (!target->buildingParts.empty())
		{
			bool inRange = target->bounds.within(Vec2<int>{position.x, position.y});
			if (!inRange)
			{
				auto targetVector =
				    Vec3<float>{std::min(std::abs(position.x - target->bounds.p0.x),
				                         std::abs(position.x - target->bounds.p1.x)),
				                std::min(std::abs(position.y - target->bounds.p0.y),
				                         std::abs(position.x - target->bounds.p1.y)),
				                0};
				inRange = tileObject->getDistanceTo(position + targetVector) <= firingRange;
			}
			// Look for a tile in front of us so that we can hit it easily
			auto forwardPos = position;
			if (velocity.x != 0 || velocity.y != 0)
			{
				forwardPos += glm::normalize(Vec3<float>{velocity.x, velocity.y, 0.0f}) * 4.0f;
			}
			if (inRange)
			{
				float closestDistance = FLT_MAX;
				Vec3<float> closestPart;
				for (auto &p : target->buildingParts)
				{
					auto distance = glm::length((Vec3<float>)p - forwardPos);
					if (distance < closestDistance)
					{
						closestDistance = distance;
						closestPart = p;
					}
				}
				closestPart += Vec3<float>{0.5f, 0.5f, 0.5f};
				// Expecting to have a part ready
				if (tileObject->getDistanceTo(closestPart) <= firingRange)
				{
					attackTarget(state, closestPart);
				}
			}
		}
		return true;
	}
	return false;
}

void Vehicle::fireWeaponsManual(GameState &state, Vec2<int> arc [[maybe_unused]])
{
	attackTarget(state, manualFirePosition);
}

bool Vehicle::attackTarget(GameState &state, sp<TileObjectVehicle> enemyTile)
{
	auto target = enemyTile->getVoxelCentrePosition();
	auto targetVelocity = enemyTile->getVehicle()->velocity;
	bool checkLOF = true;
	auto eq = getFirstFiringWeapon(state, target, checkLOF, targetVelocity, enemyTile);

	if (eq)
	{
		// Cancel cloak
		cloakTicksAccumulated = 0;

		// Let the enemy dodge us
		auto enemyVehicle = enemyTile->getVehicle();
		enemyVehicle->ticksAutoActionAvailable = 0;

		// Fire
		eq->fire(state, target, {&state, enemyVehicle});
		return true;
	}

	return false;
}

bool Vehicle::attackTarget(GameState &state, sp<TileObjectProjectile> projectileTile)
{
	auto target = projectileTile->getPosition();
	auto initialTarget = target;
	auto targetVelocity = projectileTile->getProjectile()->velocity;
	bool checkLOF = true;
	auto eq = getFirstFiringWeapon(state, target, checkLOF, targetVelocity, nullptr, true);

	if (eq)
	{
		// Cancel cloak
		cloakTicksAccumulated = 0;

		// Fire
		eq->fire(state, target, initialTarget);
		return true;
	}

	return false;
}

bool Vehicle::attackTarget(GameState &state, Vec3<float> target)
{
	auto initialTarget = target;
	auto eq = getFirstFiringWeapon(state, target);

	if (eq)
	{
		// Cancel cloak
		cloakTicksAccumulated = 0;

		// Fire
		eq->fire(state, target, initialTarget, nullptr, true);
		return true;
	}

	return false;
}

sp<VEquipment> Vehicle::getFirstFiringWeapon(GameState &state [[maybe_unused]], Vec3<float> &target,
                                             bool checkLOF, Vec3<float> targetVelocity,
                                             sp<TileObjectVehicle> enemyTile, bool pd)
{
	static const std::set<TileObject::Type> sceneryVehicleSet = {TileObject::Type::Scenery,
	                                                             TileObject::Type::Vehicle};

	sp<VEquipment> firingWeapon;

	auto firePosition = getMuzzleLocation();
	auto distanceVoxels = this->tileObject->getDistanceTo(target);
	bool outsideArc = false;

	for (auto &eq : this->equipment)
	{
		// Not a weapon
		if (eq->type->type != EquipmentSlotType::VehicleWeapon)
		{
			continue;
		}
		// Not a PD weapon (if need to be)
		if (pd && !eq->type->point_defence)
		{
			continue;
		}
		// Out of ammo or on cooldown
		if (eq->canFire() == false)
		{
			continue;
		}
		// Out of range
		if (distanceVoxels > eq->getRange())
		{
			continue;
		}
		// Check firing arc
		if (type->type != VehicleType::Type::UFO &&
		    (eq->type->firing_arc_1 < 8 || eq->type->firing_arc_2 < 8))
		{
			Vec2<int> arc = {eq->type->firing_arc_1, eq->type->firing_arc_2};
			auto facing = type->directionToVector(direction);
			auto vecToTarget = target - position;
			float angleXY = glm::angle(glm::normalize(Vec2<float>{facing.x, facing.y}),
			                           glm::normalize(Vec2<float>{vecToTarget.x, vecToTarget.y}));
			float vecToTargetXY =
			    sqrtf(vecToTarget.x * vecToTarget.x + vecToTarget.y * vecToTarget.y);
			float angleZ = glm::angle(
			    Vec2<float>{1.0f,
			                banking == VehicleType::Banking::Ascending
			                    ? 0.5f
			                    : (banking == VehicleType::Banking::Descending ? -0.5f : 0.0f)},
			    glm::normalize(Vec2<float>{vecToTargetXY, vecToTarget.z}));
			if (angleXY > (float)arc.x * (float)M_PI / 8.0f ||
			    angleZ > (float)arc.y * (float)M_PI / 8.0f)
			{
				if (eq->type->guided)
				{
					outsideArc = true;
				}
				else
				{
					continue;
				}
			}
		}
		firingWeapon = eq;
		break;
	}

	if (!firingWeapon)
	{
		return nullptr;
	}

	int attepmpt = 1;
	auto originalTarget = target;
	if (!checkLOF)
	{
		attepmpt = 2;
	}
	else if (targetVelocity.x != 0.0f || targetVelocity.y != 0.0f || targetVelocity.z != 0.0f)
	{
		attepmpt = 0;
		// Lead the target
		auto projectileVelocity = firingWeapon->type->speed * PROJECTILE_VELOCITY_MULTIPLIER;
		float timeToImpact = distanceVoxels * (float)TICK_SCALE / projectileVelocity;
		target +=
		    Collision::getLeadingOffset(target - firePosition, projectileVelocity * timeToImpact,
		                                targetVelocity * timeToImpact);
	}
	// Check if have sight to target
	// Two attempts, at second attempt try to fire at target itself
	bool hitSomethingBad = false;
	for (int i = attepmpt; i < 2; i++)
	{
		hitSomethingBad = false;
		// Checking los as otherwise we're colliding with ground when firing at bogus voxelmaps like
		// bikes
		auto hitObject = tileObject->map.findCollision(firePosition, target, sceneryVehicleSet,
		                                               tileObject, true);
		if (hitObject)
		{
			if (hitObject.obj->getType() == TileObject::Type::Vehicle)
			{
				auto vehicle = std::static_pointer_cast<TileObjectVehicle>(hitObject.obj);
				if (owner->isRelatedTo(vehicle->getVehicle()->owner) !=
				        Organisation::Relation::Hostile &&
				    vehicle != enemyTile)
				{
					hitSomethingBad = true;
				}
			}
			else if (hitObject.obj->getType() == TileObject::Type::Scenery)
			{
				hitSomethingBad = true;
			}
		}
		if (hitSomethingBad)
		{
			// Can't fire at where it will be so at least fire at where it's now
			target = originalTarget;
		}
		else
		{
			break;
		}
	}
	if (hitSomethingBad)
	{
		return nullptr;
	}

	// Fire guided weapons that are outside their firing arc in front of us
	if (outsideArc)
	{
		auto facing = type->directionToVector(direction);
		facing.z += banking == VehicleType::Banking::Ascending
		                ? 0.5f
		                : (banking == VehicleType::Banking::Descending ? -0.5f : 0.0f);
		target = position + facing * 5.0f;
	}

	return firingWeapon;
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
	if (this->shadowObject)
	{
		this->shadowObject->setPosition(pos);
	}
	if (this->smokeDoodad)
	{
		this->smokeDoodad->setPosition(pos + SMOKE_DOODAD_SHIFT);
	}
}

void Vehicle::setManualFirePosition(const Vec3<float> &pos)
{
	manualFirePosition = pos;
	manualFire = true;
}

typename decltype(Vehicle::missions)::iterator
Vehicle::addMission(GameState &state, VehicleMission *mission, bool toBack)
{
	if (!hasEngine())
	{
		if (owner == state.getPlayer())
		{
			fw().pushEvent(
			    new GameVehicleEvent(GameEventType::VehicleNoEngine, {&state, shared_from_this()}));
		}
		delete mission;
		return missions.end();
	}
	bool canPlaceInFront = false;
	switch (mission->type)
	{
		// - Can place in front
		// - Can place on crashed vehicles
		// - Can place on carrying vehicles
		case VehicleMission::MissionType::Snooze:
		case VehicleMission::MissionType::RestartNextMission:
			canPlaceInFront = true;
			break;
		// - Cannot place in front
		// - Can place on crashed vehicles
		// - Can place on carrying vehicles
		case VehicleMission::MissionType::Crash:
		case VehicleMission::MissionType::SelfDestruct:
			break;
		// - Cannot place in front
		// - Cannot place on crashed vehicles
		// - Can place on carrying vehicles
		case VehicleMission::MissionType::GotoLocation:
		case VehicleMission::MissionType::Land:
			if (crashed || sliding || falling)
			{
				delete mission;
				return missions.end();
			}
			break;
		// - Cannot place in front
		// - Cannot place on crashed vehicles
		// - Cannot place on carrying vehicles
		case VehicleMission::MissionType::GotoBuilding:
		case VehicleMission::MissionType::InvestigateBuilding:
		case VehicleMission::MissionType::FollowVehicle:
		case VehicleMission::MissionType::RecoverVehicle:
		case VehicleMission::MissionType::AttackVehicle:
		case VehicleMission::MissionType::AttackBuilding:
		case VehicleMission::MissionType::TakeOff:
		case VehicleMission::MissionType::Patrol:
		case VehicleMission::MissionType::GotoPortal:
		case VehicleMission::MissionType::InfiltrateSubvert:
		case VehicleMission::MissionType::OfferService:
		case VehicleMission::MissionType::Teleport:
		case VehicleMission::MissionType::DepartToSpace:
		case VehicleMission::MissionType::ArriveFromDimensionGate:
			if (crashed || sliding || falling || carriedVehicle)
			{
				delete mission;
				return missions.end();
			}
			break;
	}
	if (!toBack && !canPlaceInFront && !missions.empty() &&
	    (missions.front()->type == VehicleMission::MissionType::Land ||
	     missions.front()->type == VehicleMission::MissionType::TakeOff))
	{
		return missions.emplace(++missions.begin(), mission);
	}
	else if (!toBack || missions.empty())
	{
		missions.emplace_front(mission);
		missions.front()->start(state, *this);
		return missions.begin();
	}
	else
	{
		missions.emplace_back(mission);
		return --missions.end();
	}
}

bool Vehicle::setMission(GameState &state, VehicleMission *mission)
{
	if (!hasEngine())
	{
		if (owner == state.getPlayer())
		{
			fw().pushEvent(
			    new GameVehicleEvent(GameEventType::VehicleNoEngine, {&state, shared_from_this()}));
		}
		delete mission;
		return false;
	}

	bool forceClear = false;
	switch (mission->type)
	{
		case VehicleMission::MissionType::Crash:
			forceClear = true;
			break;
		case VehicleMission::MissionType::Snooze:
		case VehicleMission::MissionType::SelfDestruct:
			break;
		case VehicleMission::MissionType::GotoLocation:
		case VehicleMission::MissionType::GotoBuilding:
		case VehicleMission::MissionType::InvestigateBuilding:
		case VehicleMission::MissionType::FollowVehicle:
		case VehicleMission::MissionType::RecoverVehicle:
		case VehicleMission::MissionType::AttackVehicle:
		case VehicleMission::MissionType::AttackBuilding:
		case VehicleMission::MissionType::RestartNextMission:
		case VehicleMission::MissionType::TakeOff:
		case VehicleMission::MissionType::Land:
		case VehicleMission::MissionType::Patrol:
		case VehicleMission::MissionType::GotoPortal:
		case VehicleMission::MissionType::InfiltrateSubvert:
		case VehicleMission::MissionType::OfferService:
		case VehicleMission::MissionType::Teleport:
		case VehicleMission::MissionType::DepartToSpace:
		case VehicleMission::MissionType::ArriveFromDimensionGate:
			if (crashed || sliding || falling || carriedVehicle)
			{
				delete mission;
				return false;
			}
			break;
	}
	clearMissions(state, forceClear);
	auto it = this->addMission(state, mission, true);
	if (it != missions.begin() && missions.size() > 0)
	{
		// A mission couldn't be cleared and the new mission was inserted behind it
		// need to manually start the mission at the front
		missions.front()->start(state, *this);
	}
	return true;
}

bool Vehicle::clearMissions(GameState &state, bool forced)
{
	for (auto it = missions.begin(); it != missions.end();)
	{
		if (((*it)->type == VehicleMission::MissionType::Land ||
		     (*it)->type == VehicleMission::MissionType::TakeOff) &&
		    !forced)
		{
			it++;
		}
		else
		{
			// if we're removing an InvestigateBuilding mission
			// decrease the investigate count so the other investigating vehicles won't dangle
			if ((*it)->type == VehicleMission::MissionType::InvestigateBuilding)
			{
				if (!(*it)->isFinished(state, *this))
				{
					(*it)->targetBuilding->decreasePendingInvestigatorCount(state);
				}
			}
			it = missions.erase(it);
		}
	}
	return missions.empty();
}

bool Vehicle::popFinishedMissions(GameState &state)
{
	bool popped = false;
	while (missions.size() > 0 && missions.front()->isFinished(state, *this))
	{
		if (isDead())
		{
			return false;
		}
		LogInfo("Vehicle %s mission \"%s\" finished", name, missions.front()->getName());
		missions.pop_front();
		popped = true;
		if (!missions.empty())
		{
			LogInfo("Vehicle %s mission \"%s\" starting", name, missions.front()->getName());
			missions.front()->start(state, *this);
			continue;
		}
		else
		{
			LogInfo("No next vehicle mission, going idle");
			break;
		}
	}
	return popped;
}

bool Vehicle::getNewGoal(GameState &state, int &turboTiles)
{
	bool popped = false;
	bool acquired = false;
	// Pop finished missions if present
	popped = popFinishedMissions(state);
	int debug_deadlock_preventor = 1000;
	do
	{
		debug_deadlock_preventor--;
		// Try to get new destination
		if (!missions.empty())
		{
			acquired = missions.front()->getNextDestination(state, *this, goalPosition, goalFacing,
			                                                turboTiles);
		}
		// Pop finished missions if present
		popped = popFinishedMissions(state);
	} while (popped && !acquired && debug_deadlock_preventor > 0);
	if (debug_deadlock_preventor <= 0)
	{
		LogWarning("Vehicle %s at %s", name, position);
		for (auto &m : missions)
		{
			LogWarning("Mission %s", m->getName());
		}
		LogError("Vehicle %s deadlocked, please send log to developers. Vehicle will self-destruct "
		         "now...",
		         name);
		die(state);
		return false;
	}
	return acquired;
}

float Vehicle::getSpeed() const
{
	auto et = getEquipmentTypes();
	return this->type->getSpeed(et.begin(), et.end());
}
float Vehicle::getAngularSpeed() const
{
	// FIXME: Proper turning speed
	// This value was hand-made to look proper on annihilators
	constexpr float TURNING_MULT = (float)M_PI / (float)TICK_SCALE / VELOCITY_SCALE_CITY_X / 1.5f;
	return getSpeed() * TURNING_MULT;
}

int Vehicle::getMaxConstitution() const
{
	auto et = getEquipmentTypes();
	return this->type->getMaxConstitution(et.begin(), et.end());
}

int Vehicle::getConstitution() const { return this->getHealth() + this->getShield(); }

int Vehicle::getMaxHealth() const
{
	auto et = getEquipmentTypes();
	return this->type->getMaxHealth(et.begin(), et.end());
}

int Vehicle::getHealth() const { return this->health; }

int Vehicle::getMaxShield() const
{
	auto et = getEquipmentTypes();
	return this->type->getMaxShield(et.begin(), et.end());
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

bool Vehicle::canTeleport() const
{
	return teleportTicksAccumulated >= TELEPORT_TICKS_REQUIRED_VEHICLE;
}

bool Vehicle::hasTeleporter() const
{
	// Ground can't use teleporter
	if (type->isGround())
	{
		return false;
	}
	for (auto &e : this->equipment)
	{
		if (e->type->type != EquipmentSlotType::VehicleGeneral)
			continue;
		if (e->type->teleporting)
		{
			return true;
		}
	}

	return false;
}

bool Vehicle::hasDimensionShifter() const
{
	if (type->canEnterDimensionGate)
	{
		return true;
	}
	for (auto &e : this->equipment)
	{
		if (e->type->type != EquipmentSlotType::VehicleGeneral)
			continue;
		if (e->type->dimensionShifting)
		{
			return true;
		}
	}
	return false;
}

bool Vehicle::isIdle() const { return this->goalWaypoints.empty(); }

std::list<sp<VEquipmentType>> Vehicle::getEquipmentTypes() const
{
	std::list<sp<VEquipmentType>> et;
	for (auto &eq : equipment)
	{
		et.push_back(eq->type);
	}
	return et;
}

int Vehicle::getShield() const { return this->shield; }

int Vehicle::getArmor() const
{
	auto et = getEquipmentTypes();
	return this->type->getArmor(et.begin(), et.end());
}

int Vehicle::getAccuracy() const
{
	auto et = getEquipmentTypes();
	return this->type->getAccuracy(et.begin(), et.end());
}

// FIXME: Check int/float speed conversions
int Vehicle::getTopSpeed() const
{
	auto et = getEquipmentTypes();
	return this->type->getTopSpeed(et.begin(), et.end());
}

int Vehicle::getAcceleration() const
{
	auto et = getEquipmentTypes();
	return this->type->getAcceleration(et.begin(), et.end());
}

int Vehicle::getWeight() const
{
	auto et = getEquipmentTypes();
	return this->type->getWeight(et.begin(), et.end());
}

int Vehicle::getMaxFuel() const
{
	auto et = getEquipmentTypes();
	return this->type->getMaxFuel(et.begin(), et.end());
}

int Vehicle::getFuel() const
{
	int fuel = 0;
	for (auto eq : equipment)
	{
		if (eq->type->type == EquipmentSlotType::VehicleEngine)
		{
			fuel += eq->ammo;
		}
	}
	return fuel;
}

int Vehicle::getMaxPassengers() const
{
	auto et = getEquipmentTypes();
	return this->type->getMaxPassengers(et.begin(), et.end());
}

int Vehicle::getPassengers() const { return (int)currentAgents.size(); }

int Vehicle::getMaxCargo() const
{
	auto et = getEquipmentTypes();
	return this->type->getMaxCargo(et.begin(), et.end());
}

int Vehicle::getCargo() const
{
	int cargoAmount = 0;
	for (auto &c : cargo)
	{
		if (c.type == Cargo::Type::Bio)
		{
			continue;
		}
		cargoAmount += c.count * c.space / c.divisor;
	}
	return cargoAmount;
}

int Vehicle::getMaxBio() const
{
	auto et = getEquipmentTypes();
	return this->type->getMaxBio(et.begin(), et.end());
}

int Vehicle::getBio() const
{
	int cargoAmount = 0;
	for (auto &c : cargo)
	{
		if (c.type != Cargo::Type::Bio)
		{
			continue;
		}
		cargoAmount += c.count * c.space;
	}
	return cargoAmount;
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

sp<VEquipment> Vehicle::addEquipment(GameState &state, Vec2<int> pos,
                                     StateRef<VEquipmentType> equipmentType)
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
		LogError("Equipping \"%s\" on \"%s\" at %s failed: No valid slot", equipmentType->name,
		         this->name, pos);
		return nullptr;
	}

	switch (equipmentType->type)
	{
		case EquipmentSlotType::VehicleEngine:
		{
			auto engine = mksp<VEquipment>();
			engine->type = equipmentType;
			this->equipment.emplace_back(engine);
			engine->equippedPosition = slotOrigin;
			LogInfo("Equipped \"%s\" with engine \"%s\"", this->name, equipmentType->name);
			return engine;
		}
		case EquipmentSlotType::VehicleWeapon:
		{
			auto thisRef = StateRef<Vehicle>(&state, shared_from_this());
			auto weapon = mksp<VEquipment>();
			weapon->type = equipmentType;
			weapon->owner = thisRef;
			this->equipment.emplace_back(weapon);
			weapon->equippedPosition = slotOrigin;
			LogInfo("Equipped \"%s\" with weapon \"%s\"", this->name, equipmentType->name);
			return weapon;
		}
		case EquipmentSlotType::VehicleGeneral:
		{
			auto equipment = mksp<VEquipment>();
			equipment->type = equipmentType;
			LogInfo("Equipped \"%s\" with general equipment \"%s\"", this->name,
			        equipmentType->name);
			equipment->equippedPosition = slotOrigin;
			this->equipment.emplace_back(equipment);
			return equipment;
		}
		default:
			LogError("Equipment \"%s\" for \"%s\" at pos (%d,%d} has invalid type",
			         equipmentType->name, this->name, pos.x, pos.y);
			return nullptr;
	}
}

sp<VEquipment> Vehicle::addEquipment(GameState &state, StateRef<VEquipmentType> equipmentType)
{
	Vec2<int> pos;
	bool slotFound = false;
	for (auto &slot : type->equipment_layout_slots)
	{
		if (canAddEquipment(slot.bounds.p0, equipmentType))
		{
			pos = slot.bounds.p0;
			slotFound = true;
			break;
		}
	}
	if (!slotFound)
	{
		return nullptr;
	}

	return addEquipment(state, pos, equipmentType);
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

Vec3<int> Vehicle::getPreferredPosition(Vec3<int> position) const
{
	if (!type->isGround())
	{
		position.z = (int)altitude;
	}
	return position;
}

Vec3<int> Vehicle::getPreferredPosition(int x, int y, int z) const
{
	return {x, y, type->isGround() ? z : (int)altitude};
}

void Vehicle::equipDefaultEquipment(GameState &state)
{
	equipment.clear();
	loot.clear();
	LogInfo("Equipping \"%s\" with default equipment", this->type->name);
	auto alien = owner == state.getAliens();
	for (auto &pair : this->type->initial_equipment_list)
	{
		auto &pos = pair.first;
		auto &etype = pair.second;

		if (alien && state.totalScore.craftShotDownUFO < etype->scoreRequirement)
		{
			continue;
		}
		loot.push_back(etype);
		auto eq = this->addEquipment(state, pos, etype);
		eq->ammo = eq->type->max_ammo;
	}
	shield = getMaxShield();
	health = getMaxHealth();
}

void Vehicle::nextFrame(int ticks)
{
	animationDelay += ticks;
	if (animationDelay > 10)
	{
		animationDelay = 0;
		animationFrame++;
		if (animationFrame == type->animation_sprites.end())
		{
			animationFrame = type->animation_sprites.begin();
		}
	}
}

template <> sp<Vehicle> StateObject<Vehicle>::get(const GameState &state, const UString &id)
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

Cargo::Cargo(GameState &state, StateRef<AEquipmentType> equipment, int count, int price,
             StateRef<Organisation> originalOwner, StateRef<Building> destination)
    : Cargo(state, equipment->bioStorage ? Type::Bio : Type::Agent, equipment.id, count,
            equipment->type == AEquipmentType::Type::Ammo ? equipment->max_ammo : 1,
            equipment->store_space, price, originalOwner, destination)
{
}

Cargo::Cargo(GameState &state, StateRef<VEquipmentType> equipment, int count, int price,
             StateRef<Organisation> originalOwner, StateRef<Building> destination)
    : Cargo(state, Type::VehicleEquipment, equipment.id, count, 1, equipment->store_space, price,
            originalOwner, destination)
{
}

Cargo::Cargo(GameState &state, StateRef<VAmmoType> equipment, int count, int price,
             StateRef<Organisation> originalOwner, StateRef<Building> destination)
    : Cargo(state, Type::VehicleAmmo, equipment.id, count, 1, equipment->store_space, price,
            originalOwner, destination)
{
}

Cargo::Cargo(GameState &state, Type type, UString id, int count, int divisor, int space, int cost,
             StateRef<Organisation> originalOwner, StateRef<Building> destination)
    : type(type), id(id), count(count), divisor(divisor), space(space), cost(cost),
      originalOwner(originalOwner), destination(destination)
{
	suppressEvents = count == 0;
	expirationDate = state.gameTime.getTicks() + TICKS_CARGO_TTL;
}

bool Cargo::checkExpiryDate(GameState &state, StateRef<Building> currentBuilding)
{
	if (expirationDate == 0)
	{
		return false;
	}
	if (expirationDate < state.gameTime.getTicks())
	{
		refund(state, currentBuilding);
		return false;
	}
	if (warned)
	{
		return false;
	}
	if (expirationDate - state.gameTime.getTicks() < TICKS_CARGO_WARNING)
	{
		warned = true;
		return true;
	}
	return false;
}

void Cargo::refund(GameState &state, StateRef<Building> currentBuilding)
{
	if (cost > 0)
	{
		destination->owner->balance += cost * count / divisor;
		if (!originalOwner)
		{
			LogError("Bought cargo from nobody!? WTF?");
			return;
		}
		originalOwner->balance -= cost * count / divisor;
		if (destination->owner == state.getPlayer())
		{
			fw().pushEvent(
			    new GameBaseEvent(GameEventType::CargoExpired, destination->base, originalOwner));
		}
		switch (type)
		{
			case Type::Bio:
				if (state.economy.find(id) != state.economy.end())
				{
					LogError("Economy found for bio item!?", id);
				}
				break;
			case Type::Agent:
			case Type::VehicleAmmo:
			case Type::VehicleEquipment:
				if (state.economy.find(id) != state.economy.end())
				{
					auto &economy = state.economy[id];
					economy.currentStock += count / divisor;
				}
				break;
		}
	}
	else if (currentBuilding && originalOwner == destination->owner)
	{
		destination = currentBuilding;
		if (destination->base && destination->owner == state.getPlayer())
		{
			fw().pushEvent(
			    new GameBaseEvent(GameEventType::CargoExpired, destination->base, originalOwner));
		}
		arrive(state);
	}
	// If no currentBuilding means cargo expired since base destroyed
	else if (currentBuilding)
	{
		if (destination->base && destination->owner == state.getPlayer())
		{
			fw().pushEvent(new GameBaseEvent(GameEventType::CargoExpired, destination->base));
		}
	}
	clear();
}

void Cargo::arrive(GameState &state)
{
	bool cargoArrived;
	bool bioArrived;
	bool recoveryArrived;
	bool transferArrived;
	std::set<StateRef<Organisation>> suppliers;
	arrive(state, cargoArrived, bioArrived, recoveryArrived, transferArrived, suppliers);
}

void Cargo::arrive(GameState &state, bool &cargoArrived, bool &bioArrived, bool &recoveryArrived,
                   bool &transferArrived, std::set<StateRef<Organisation>> &suppliers)
{
	if (destination->base)
	{
		switch (type)
		{
			case Type::Bio:
				destination->base->inventoryBioEquipment[id] += count;
				break;
			case Type::Agent:
				destination->base->inventoryAgentEquipment[id] += count;
				break;
			case Type::VehicleAmmo:
				destination->base->inventoryVehicleAmmo[id] += count;
				break;
			case Type::VehicleEquipment:
				destination->base->inventoryVehicleEquipment[id] += count;
				break;
		}
		// Transfer
		if (originalOwner == state.getPlayer())
		{
			if (type == Type::Bio)
			{
				bioArrived = true;
			}
			else
			{
				transferArrived = true;
			}
		}
		// Loot
		else if (!originalOwner)
		{
			recoveryArrived = true;
		}
		// Purchase
		else
		{
			cargoArrived = true;
			suppliers.insert(originalOwner);
		}
	}
	destination.clear();
	count = 0;
}

void Cargo::seize(GameState &state, StateRef<Organisation> org [[maybe_unused]])
{
	switch (type)
	{
		case Type::Bio:
			if (state.economy.find(id) != state.economy.end())
			{
				LogError("Economy found for bio item!?", id);
			}
			break;
		case Type::Agent:
		case Type::VehicleAmmo:
		case Type::VehicleEquipment:
			if (state.economy.find(id) != state.economy.end())
			{
				auto &economy = state.economy[id];
				if (cost == 0)
				{
					cost = economy.currentPrice;
				}
				economy.currentStock += count / divisor;
			}
			break;
	}
	int worth = cost * count / divisor;
	// FIXME: Adjust relationship accordingly to seized cargo's worth
	LogWarning("Adjust relationship accordingly to worth: %d", worth);
	if (destination->owner == state.getPlayer())
	{
		fw().pushEvent(
		    new GameBaseEvent(GameEventType::CargoSeized, destination->base, originalOwner));
	}
	clear();
}

void Cargo::clear() { count = 0; }

}; // namespace OpenApoc
