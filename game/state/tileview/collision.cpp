#include "game/state/tileview/collision.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battleitem.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject.h"
#include "library/line.h"
#include "library/sp.h"
#include "library/voxel.h"
#include <algorithm>
#include <iterator>

namespace OpenApoc
{

Collision TileMap::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd,
                                 const std::set<TileObject::Type> validTypes,
                                 sp<TileObject> ignoredObject, bool useLOS,
                                 bool check_full_path) const
{
	bool type_checking = validTypes.size() > 0;
	Collision c;
	c.obj = nullptr;
	Vec3<int> tileSize = voxelMapSize;
	Vec3<float> tileSizef = voxelMapSize;
	Vec3<int> lineSegmentStartVoxel = lineSegmentStart * tileSizef;
	Vec3<int> lineSegmentEndVoxel = lineSegmentEnd * tileSizef;
	LineSegment<int, true> line{lineSegmentStartVoxel, lineSegmentEndVoxel};
	// "point" is thee corrdinate measured in voxel scale units, meaning,
	// voxel point coordinate within map
	for (auto &point : line)
	{
		auto tile = point / tileSize;
		if (tile.x < 0 || tile.x >= size.x || tile.y < 0 || tile.y >= size.y || tile.z < 0 ||
		    tile.z >= size.z)
		{
			if (check_full_path)
				continue;
			else
				return c;
		}

		const Tile *t = this->getTile(tile);
		for (auto &obj : t->intersectingObjects)
		{
			if ((!obj->hasVoxelMap()) ||
			    (type_checking && validTypes.find(obj->type) == validTypes.end()) ||
			    (obj == ignoredObject))
				continue;
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
				continue;
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
