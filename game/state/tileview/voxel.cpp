#include "game/state/tileview/voxel.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject.h"
#include "library/line.h"
#include "library/sp.h"
#include "library/voxel.h"
#include <iterator>

namespace OpenApoc
{

Collision Tile::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd) const
{
	Collision c;
	c.obj = nullptr;

	Vec3<int> tileSize = {32, 32, 16};
	Vec3<int> tileStart = this->position * tileSize;
	Vec3<int> tileEnd = tileStart + tileSize;

	// FIXME: This aligns the beginning/end to
	Vec3<int> voxelLineStart =
	    Vec3<int>{lineSegmentStart.x * tileSize.x, lineSegmentStart.y * tileSize.y,
	              lineSegmentStart.z * tileSize.z};
	Vec3<int> voxelLineEnd = Vec3<int>{lineSegmentEnd.x * tileSize.x, lineSegmentEnd.y * tileSize.y,
	                                   lineSegmentEnd.z * tileSize.z};
	// Treat voxel collisions as conservative (IE if /any/ part of the voxel
	// touches the line it's a 'hit'
	LineSegment<int, true> line{voxelLineStart, voxelLineEnd};
	for (auto &point : line)
	{
		if (point.x < tileStart.x || point.y < tileStart.y || point.z < tileStart.z ||
		    point.x > tileEnd.x || point.y > tileEnd.y || point.z > tileEnd.z)
			continue;
		for (auto &obj : intersectingObjects)
		{
			auto voxelMap = obj->getVoxelMap();
			if (!voxelMap)
			{
				continue;
			}
			auto objPos = obj->getPosition();
			objPos -= obj->getVoxelOffset();
			Vec3<int> objOffset = point - Vec3<int>{objPos.x * tileSize.x, objPos.y * tileSize.y,
			                                        objPos.z * tileSize.z};
			if (voxelMap->getBit(objOffset))
			{
				c.obj = obj;
				c.position = Vec3<float>{point.x, point.y, point.z};
				c.position /= tileSize;
				return c;
			}
		}
	}
	return c;
}

Collision TileMap::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd) const
{
	Collision c;
	c.obj = nullptr;
	// We can increment by '1f' and still get a point in every affected tile
	LineSegment<float, true> line{lineSegmentStart, lineSegmentEnd};
	for (auto &point : line)
	{
		if (point.x < 0 || point.x >= size.x || point.y < 0 || point.y >= size.y || point.z < 0 ||
		    point.z >= size.z)
		{
			return c;
		}

		const Tile *t = this->getTile(point);
		c = t->findCollision(lineSegmentStart, lineSegmentEnd);
		if (c)
			return c;
	}
	return c;
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
