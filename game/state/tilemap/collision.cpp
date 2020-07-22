#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/tilemap/collision.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleitem.h"
#include "game/state/city/vehicle.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject.h"
#include "game/state/tilemap/tileobject_projectile.h"
#include "library/line.h"
#include "library/sp.h"
#include "library/voxel.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <iterator>

namespace OpenApoc
{

Collision TileMap::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd,
                                 const std::set<TileObject::Type> validTypes,
                                 sp<TileObject> ignoredObject, bool useLOS, bool check_full_path,
                                 unsigned maxRange, bool recordPassedTiles,
                                 StateRef<Organisation> ignoreOwnedProjectiles) const
{
	bool typeChecking = validTypes.size() > 0;
	bool rangeChecking = maxRange > 0.0f;
	const Tile *lastT = nullptr;
	// We apply a median value accumulated in all tiles passed every time we pass a tile
	// This makes it so that we do not over or under-apply smoke when going diagonally
	float blockageAccumulatedSoFar = 0.0f;
	int distanceToLastTile = 0;
	float accumulatedSinceLastTile = 0;
	int numberTilesWithBlockage = 0;
	// Init collision parameters
	Collision c;
	c.obj = nullptr;
	Vec3<int> tileSize = voxelMapSize;
	Vec3<float> tileSizef = voxelMapSize;
	Vec3<int> lineSegmentStartVoxel = lineSegmentStart * tileSizef;
	Vec3<int> lineSegmentEndVoxel = lineSegmentEnd * tileSizef;
	LineSegment<int, true> line{lineSegmentStartVoxel, lineSegmentEndVoxel};

	// "point" is the coordinate measured in voxel scale units, meaning,
	// voxel point coordinate within map
	for (auto &point : line)
	{
		auto tile = point / tileSize;
		// We need to check if `point` is negative instead of `tile` because negative values
		// will round towards zero
		if (point.x < 0 || tile.x >= size.x || point.y < 0 || tile.y >= size.y || point.z < 0 ||
		    tile.z >= size.z)
		{
			if (check_full_path)
			{
				continue;
			}
			else
			{
				return c;
			}
		}
		const Tile *t = this->getTile(tile);
		if (rangeChecking)
		{
			if (!lastT)
			{
				lastT = t;
				if (recordPassedTiles)
				{
					c.passedTiles.push_back(tile);
				}
			}
			else
			{
				if (t != lastT)
				{
					lastT = t;
					if (recordPassedTiles)
					{
						c.passedTiles.push_back(tile);
					}
					auto vec = t->position;
					// Apply vision blockage if we passed at least 1 tile
					auto thisDistance =
					    sqrtf((vec.x - lineSegmentStart.x) * (vec.x - lineSegmentStart.x) +
					          (vec.y - lineSegmentStart.y) * (vec.y - lineSegmentStart.y) +
					          (vec.z - lineSegmentStart.z) * (vec.z - lineSegmentStart.z));
					if ((int)thisDistance > distanceToLastTile)
					{
						if (numberTilesWithBlockage > 0)
						{
							blockageAccumulatedSoFar += accumulatedSinceLastTile *
							                            ((int)thisDistance - distanceToLastTile) /
							                            numberTilesWithBlockage;
						}
						distanceToLastTile = thisDistance;
						accumulatedSinceLastTile = 0;
						numberTilesWithBlockage = 0;
					}

					// Reached end of LOS with accumulated blockage
					if ((unsigned)(thisDistance + blockageAccumulatedSoFar) > maxRange)
					{
						c.outOfRange = true;
						c.position = Vec3<float>{point};
						c.position /= tileSizef;
						return c;
					}

					// Add this tile's vision blockage to accumulated since last tile blockage
					auto thisBlockage = t->visionBlockValue;
					if (thisBlockage > 0)
					{
						accumulatedSinceLastTile += thisBlockage;
						numberTilesWithBlockage++;
					}
				}
			}
		}

		for (auto &obj : t->intersectingObjects)
		{
			if ((!obj->hasVoxelMap(useLOS)) ||
			    (typeChecking && validTypes.find(obj->type) == validTypes.end()) ||
			    (obj == ignoredObject))
			{
				continue;
			}
			if (ignoreOwnedProjectiles && obj->type == TileObject::Type::Projectile)
			{
				auto projectile =
				    std::static_pointer_cast<TileObjectProjectile>(obj)->getProjectile();
				auto owner =
				    projectile->firerUnit
				        ? projectile->firerUnit->owner
				        : (projectile->firerVehicle ? projectile->firerVehicle->owner : nullptr);
				if (owner == ignoreOwnedProjectiles)
				{
					continue;
				}
			}

			// coordinate of the object's voxelmap's min point
			auto objPos = obj->getCenter();
			objPos -= obj->getVoxelOffset();
			objPos *= tileSizef;
			// coordinate of the voxel within object
			Vec3<int> voxelPos = point - Vec3<int>{objPos};
			// voxel map to use
			Vec3<int> voxelMapIndex = voxelPos / tileSize;
			auto voxelMap = obj->getVoxelMap(voxelMapIndex, useLOS);
			if (!voxelMap)
			{
				continue;
			}
			// coordinate of the voxel within map
			Vec3<int> voxelPosWithinMap = voxelPos % tileSize;
			if (voxelMap->getBit(voxelPosWithinMap))
			{
				c.obj = obj;
				c.position = Vec3<float>{point};
				c.position /= tileSizef;
				return c;
			}
		}
	}

	return c;
}

// Checks if, while going along the trajectory, we reach target tile or get first collision within
// it's boundaries
bool TileMap::checkThrowTrajectory(const sp<TileObject> thrower, Vec3<float> start, Vec3<int> end,
                                   Vec3<float> targetVectorXY, float velocityXY,
                                   float velocityZ) const
{
	int iterationsPerCollision = 10;
	Vec3<float> curPos = start;
	Vec3<float> newPos;
	Vec3<float> velocity =
	    (glm::normalize(targetVectorXY) * velocityXY + Vec3<float>{0.0, 0.0, velocityZ}) *
	    VELOCITY_SCALE_BATTLE;
	int collisionIgnoredTicks =
	    thrower ? 0 : (int)ceilf(36.0f / glm::length(velocity / VELOCITY_SCALE_BATTLE)) + 1;
	Collision c;
	do
	{
		newPos = curPos;
		for (int i = 0; i < iterationsPerCollision &&
		                (collisionIgnoredTicks == 0 || i < collisionIgnoredTicks);
		     i++)
		{
			velocity.z -= FALLING_ACCELERATION_ITEM;
			newPos += velocity / (float)TICK_SCALE / VELOCITY_SCALE_BATTLE;
		}
		if (collisionIgnoredTicks > 0)
		{
			collisionIgnoredTicks = std::max(0, collisionIgnoredTicks - iterationsPerCollision);
		}
		else
		{
			c = findCollision(curPos, newPos, {});
		}
		if (c && c.obj == thrower)
		{
			c = {};
		}
		curPos = c ? c.position : newPos;
	} while (!c && ((Vec3<int>)curPos) != end && curPos.z < size.z);
	return (Vec3<int>)curPos == end;
}

// Figure out where to fire on a moving target
Vec3<float> Collision::getLeadingOffset(Vec3<float> tarPosRelative, float ourVelocity,
                                        Vec3<float> tarVelocity)
{
	Vec3<float> tarHeading = glm::normalize(tarVelocity);
	float tarVelocityLen = glm::length(tarVelocity);
	// Alexey Andronov:
	// 1) Given triangle with sides ABC and angles opposite to these sides abc we know that:
	//   sina/A = sinb/B = sinc/C
	// 2) In our case, let a = desired point, b = tarPos, c = ourPos
	// 3) We know:
	//	  - angle b = angle between -tarPos and tarHeading
	//    - side A = len(tarPos)
	//    - B/C = ourVelocity / tarVelocityLen
	// 4) Multiplying both sides in last equation by B we get
	//   B = C * ourVelocity / tarVelocityLen
	// 5) Eq 1 becomes:
	//   sina/A = sinb/(C * ourVelocity / tarVelocity) = sinc/C
	// 6) Multiplying both sides of last two by B we get:
	//   sinc = sinb * tarVelocityLen / ourVelocity
	// 7) Now we know sinb and sinc, and we can find a, since a+b+c = pi, and thus sina
	// 8) Now we know sina, sinb, sinc and A, and we can find B and C
	//   B = A * sinb / sina
	//   C = A * sinc / sina

	if (tarVelocityLen == 0.0f)
	{
		// Target isn't moving, no leading required
		return {0.0f, 0.0f, 0.0f};
	}

	float A = glm::length(tarPosRelative);
	float b = glm::angle(glm::normalize(-tarPosRelative), tarHeading);
	float sinc = sinf(b) * tarVelocityLen / ourVelocity;
	if (sinc > 1.0f)
	{
		// Target is moving too fast, we can't catch it
		return tarHeading * tarVelocityLen;
	}
	float c = asinf(sinc);
	float a = M_PI - b - c;
	if (a < 0.0f)
	{
		// Target is moving too fast, we can't catch it
		return tarHeading * tarVelocityLen;
	}
	float C = A / sinf(a) * sinf(c);
	return tarHeading * C;
}

}; // namespace OpenApoc

#if 0

#include <iostream>
using namespace OpenApoc;
void printLine(Vec3<int> start, Vec3<int> end)
{
	LineSegment<false> line{start, end};
	std::cout << "Line from {" << start.x << "," << start.y << "," << start.z << "} to {"
		<< end.x << "," << end.y << "," << end.z << "}\n";
	int count = 0;
	for (auto &point : line)
	{
		std::cout << "\tpoint " << count++ << " {" << point.x << "," << point.y << "," << point.z << "}\n";
	}
	LineSegment<true> lineConservative{start, end};
	
	std::cout << "Line conservative from {" << start.x << "," << start.y << "," << start.z << "} to {"
	<< end.x << "," << end.y << "," << end.z << "}\n";
	count = 0;
	for (auto &point : lineConservative)
	{
		std::cout << "\tpoint " << count++ << " {" << point.x << "," << point.y << "," << point.z << "}\n";
	}
}

int main(int argc, char **argv)
{
	printLine({0,0,0},{10,10,0});
	printLine({0,0,0},{10,-10,0});
	printLine({0,0,0},{10,-10,10});

}
#endif
