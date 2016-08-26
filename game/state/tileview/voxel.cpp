#include "game/state/tileview/voxel.h"
#include "game/state/battle.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject.h"
#include "library/line.h"
#include "library/sp.h"
#include "library/voxel.h"
#include <iterator>

namespace OpenApoc
{

Collision TileMap::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd) const
{
	Collision c;
	c.obj = nullptr;
	Vec3<int> tileSize = {32, 32, 16};
	Vec3<float> tileSizef = {32, 32, 16};
	Vec3<int> lineSegmentStartVoxel = lineSegmentStart * tileSizef;
	Vec3<int> lineSegmentEndVoxel = lineSegmentEnd * tileSizef;
	LineSegment<int, true> line{lineSegmentStartVoxel, lineSegmentEndVoxel};
	for (auto &point : line)
	{
		auto tile = point / tileSize;
		if (tile.x < 0 || tile.x >= size.x || tile.y < 0 || tile.y >= size.y || tile.z < 0 ||
		    tile.z >= size.z)
		{
			return c;
		}

		const Tile *t = this->getTile(tile);
		for (auto &obj : t->intersectingObjects)
		{
			auto voxelMap = obj->getVoxelMap();
			if (!voxelMap)
				continue;
			auto objPos = obj->getPosition();
			objPos -= obj->getVoxelOffset();
			objPos *= tileSize;
			Vec3<int> voxelPos = point - Vec3<int>{objPos};
			if (voxelMap->getBit(voxelPos))
			{
				c.obj = obj;
				c.position = Vec3<float>{point};
				c.position /= tileSize;
				return c;
			}
		}
	}
	return c;
}

CollisionB BattleTileMap::findCollision(Vec3<float> lineSegmentStart,
                                        Vec3<float> lineSegmentEnd) const
{
	CollisionB c;
	c.obj = nullptr;
	Vec3<int> tileSize = {48, 24, 40};
	Vec3<float> tileSizef = {48, 24, 40};
	Vec3<int> lineSegmentStartVoxel = lineSegmentStart * tileSizef;
	Vec3<int> lineSegmentEndVoxel = lineSegmentEnd * tileSizef;
	LineSegment<int, true> line{lineSegmentStartVoxel, lineSegmentEndVoxel};
	for (auto &point : line)
	{
		auto tile = point / tileSize;
		if (tile.x < 0 || tile.x >= size.x || tile.y < 0 || tile.y >= size.y || tile.z < 0 ||
		    tile.z >= size.z)
		{
			continue;
		}

		const BattleTile *t = this->getTile(tile);
		for (auto &obj : t->intersectingObjects)
		{
			auto voxelMap = obj->getVoxelMap();
			if (!voxelMap)
				continue;
			auto objPos = obj->getPosition();
			objPos -= obj->getVoxelOffset();
			objPos *= tileSize;
			Vec3<int> voxelPos = point - Vec3<int>{objPos};
			if (voxelMap->getBit(voxelPos))
			{
				c.obj = obj;
				c.position = Vec3<float>{point};
				c.position /= tileSize;
				return c;
			}
		}
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
