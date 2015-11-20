#include "library/sp.h"
#include "game/tileview/voxel.h"
#include "game/tileview/tile.h"
#include "game/tileview/tileobject.h"
#include "framework/logger.h"

#include <iterator>

namespace OpenApoc
{

VoxelSlice::VoxelSlice(Vec2<int> size) : size(size), bits(size.x * size.y) {}

bool VoxelSlice::getBit(Vec2<int> pos) const
{
	if (pos.x < 0 || pos.x >= this->size.x || pos.y < 0 || pos.y >= this->size.y)
	{
		LogError("Invalid position {%d,%d} in slice sized {%d,%d}", pos.x, pos.x, size.x, size.y);
		return false;
	}

	return this->bits[pos.y * this->size.x + pos.x];
}

void VoxelSlice::setBit(Vec2<int> pos, bool b)
{
	if (pos.x < 0 || pos.x >= this->size.x || pos.y < 0 || pos.y >= this->size.y)
	{
		LogError("Invalid position {%d,%d} in slice sized {%d,%d}", pos.x, pos.x, size.x, size.y);
		return;
	}
	this->bits[pos.y * this->size.x + pos.x] = b;
}

VoxelMap::VoxelMap(Vec3<int> size) : size(size) { slices.resize(size.z); }

bool VoxelMap::getBit(Vec3<int> pos) const
{
	if (pos.x < 0 || pos.x >= this->size.x || pos.y < 0 || pos.y >= this->size.y || pos.z < 0 ||
	    pos.z >= this->size.z)
	{
		return false;
	}

	if (slices.size() <= static_cast<unsigned>(pos.z))
		return false;
	if (!slices[pos.z])
		return false;

	return slices[pos.z]->getBit({pos.x, pos.y});
}

void VoxelMap::setSlice(int z, sp<VoxelSlice> slice)
{
	if (z < 0 || static_cast<unsigned>(z) >= this->slices.size())
	{
		LogError("Trying to set slice %d in a {%d,%d,%d} sized voxelMap", z, this->size.x,
		         this->size.y, this->size.z);
		return;
	}
	if (slice->getSize() != Vec2<int>{this->size.x, this->size.y})
	{
		LogError("Trying to set slice of size {%d,%d} in {%d,%d,%d} sized voxelMap",
		         slice->getSize().x, slice->getSize().y, this->size.x, this->size.y, this->size.z);
		return;
	}
	this->slices[z] = slice;
}

namespace
{

template <typename T, bool conservative> class LineSegmentIterator;

template <typename T, bool conservative> class LineSegment
{
  public:
	Vec3<T> startPoint;
	Vec3<T> endPoint;
	T increment;
	LineSegment(Vec3<T> start, Vec3<T> end, T increment = 1)
	    : startPoint(start), endPoint(end), increment(increment)
	{
	}
	LineSegmentIterator<T, conservative> begin();
	LineSegmentIterator<T, conservative> end();
};

template <typename T, bool conservative>
class LineSegmentIterator : public std::iterator<std::forward_iterator_tag, Vec3<T>>
{
  private:
	Vec3<T> point;
	Vec3<T> err;
	Vec3<T> inc;
	Vec3<T> step;
	Vec3<T> d2;
	T dstep2;

	LineSegment<T, conservative> &line;

  public:
	LineSegmentIterator(Vec3<T> start, LineSegment<T, conservative> &l) : point(start), line(l)
	{
		err = {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
		step = {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
		Vec3<T> d = l.endPoint - l.startPoint;
		inc.x = (d.x < static_cast<T>(0)) ? -l.increment : l.increment;
		inc.y = (d.y < static_cast<T>(0)) ? -l.increment : l.increment;
		inc.z = (d.z < static_cast<T>(0)) ? -l.increment : l.increment;

		Vec3<T> absd = glm::abs(d);
		d2 = absd * static_cast<T>(2);
		if (absd.x >= absd.y && absd.x >= absd.z)
		{
			step.x = inc.x;
			dstep2 = d2.x;
			d2.x = static_cast<T>(0);
		}
		else if (absd.y >= absd.x && absd.y >= absd.z)
		{
			step.y = inc.y;
			dstep2 = d2.y;
			d2.y = static_cast<T>(0);
		}
		else if (absd.z >= absd.x && absd.z >= absd.y)
		{
			step.z = inc.z;
			dstep2 = d2.z;
			d2.z = static_cast<T>(0);
		}
	}

	LineSegmentIterator &operator++()
	{
		if (conservative)
		{
			if (err.x > static_cast<T>(0))
			{
				point.x += inc.x;
				err.x -= dstep2;
			}
			else if (err.y > static_cast<T>(0))
			{
				point.y += inc.y;
				err.y -= dstep2;
			}
			else if (err.z > static_cast<T>(0))
			{
				point.z += inc.z;
				err.z -= dstep2;
			}
			else
			{
				err += d2;
				point += step;
			}
		}
		else
		{
			if (err.x > static_cast<T>(0))
			{
				point.x += inc.x;
				err.x -= dstep2;
			}
			if (err.y > static_cast<T>(0))
			{
				point.y += inc.y;
				err.y -= dstep2;
			}
			if (err.z > static_cast<T>(0))
			{
				point.z += inc.z;
				err.z -= dstep2;
			}
			err += d2;
			point += step;
		}
		return *this;
	}

	bool operator==(const LineSegmentIterator &other)
	{
		return (this->point * step == other.point * step);
	}

	bool operator!=(const LineSegmentIterator &other) { return !(*this == other); }

	Vec3<T> &operator*() { return point; }
};

template <typename T, bool conservative>
LineSegmentIterator<T, conservative> LineSegment<T, conservative>::begin()
{
	return LineSegmentIterator<T, conservative>(this->startPoint, *this);
}

template <typename T, bool conservative>
LineSegmentIterator<T, conservative> LineSegment<T, conservative>::end()
{
	return LineSegmentIterator<T, conservative>(this->endPoint + Vec3<T>{1, 1, 1}, *this);
}

}; // anonymous namespace

Collision Tile::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd)
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

Collision TileMap::findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd)
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

		Tile *t = this->getTile(point);
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
