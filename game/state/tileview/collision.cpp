#include "game/state/tileview/collision.h"
#include "game/state/battle/battle.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject.h"
#include "library/line.h"
#include "library/sp.h"
#include "library/voxel.h"
#include <iterator>

namespace OpenApoc
{

Collision TileMap::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd,
                                 const std::set<TileObject::Type> validTypes, bool useLOS,
                                 bool check_full_path) const
{
	if (useLOS)
		LogError("Handle LOS checks");
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
			if (!obj->hasVoxelMap())
				continue;
			if (type_checking && validTypes.find(obj->type) == validTypes.end())
				continue;
			// coordinate of the object's voxelmap's min point
			auto objPos = obj->getCenter();
			objPos -= obj->getVoxelOffset();
			objPos *= tileSizef;
			// coordinate of the voxel within object
			Vec3<int> voxelPos = point - Vec3<int>{objPos};
			// voxel map to use
			Vec3<int> voxelMapIndex = voxelPos / tileSize;
			auto voxelMap = obj->getVoxelMap(voxelMapIndex);
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
