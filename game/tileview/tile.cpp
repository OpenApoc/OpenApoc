#include "tile.h"
#include "framework/framework.h"

namespace OpenApoc {

TileMap::TileMap(Framework &fw, Vec3<int> size)
	: fw(fw), size(size)
{
	tiles.resize(size.z);
	for (int z = 0; z < size.z; z++)
	{
		tiles[z].resize(size.y);
		for (int y = 0; y < size.y; y++)
		{
			tiles[z][y].reserve(size.x);
			for (int x = 0; x < size.x; x++)
			{
				tiles[z][y].emplace_back(*this, Vec3<int>{x,y,z});
			}
		}
	}
}

void
TileMap::update(unsigned int ticks)
{
	//Default tilemap update calls update(ticks) on all tiles
	//Subclasses can optimise this if they know which tiles might be 'active'
	for (auto& object : this->activeObjects)
		object->update(ticks);
}

TileMap::~TileMap()
{
}

Tile::Tile(TileMap &map, Vec3<int> position)
	: map(map), position(position)
{
}

TileObject::TileObject(Tile *owningTile, Vec3<float> position, Vec3<float> size, bool visible, bool collides, std::shared_ptr<Image> sprite)
	: owningTile(owningTile), position(position), size(size), visible(visible), collides(collides), sprite(sprite)
{

}

TileObject::~TileObject()
{

}

Cubeoid<int>
TileObject::getBoundingBox()
{
	Vec3<int> p1 {(int)floor((float)position.x), (int)floor((float)position.y), (int)floor((float)position.z)};
	Vec3<int> p2 {(int)ceil((float)position.x + size.x), (int)ceil((float)position.y + size.y), (int)ceil((float)position.z + size.z)};
	return Cubeoid<int>{p1,p2};
}

Vec3<float>
TileObject::getSize()
{
	return this->size;
}

Vec3<float>
TileObject::getPosition()
{
	return this->position;
}

Image&
TileObject::getSprite()
{
	return *this->sprite;
}

TileObjectCollisionVoxels&
TileObject::getCollisionVoxels()
{
	return this->collisionVoxels;
}

class PathComparer
{
public:
	Vec3<float> dest;
	Vec3<float> origin;
	PathComparer(Vec3<int> d)
		: dest{d.x, d.y, d.z}{}
	bool operator() (Tile* t1, Tile* t2)
	{
		Vec3<float> t1Pos {t1->position.x, t1->position.y, t1->position.z};
		Vec3<float> t2Pos {t2->position.x, t2->position.y, t2->position.z};

		Vec3<float> t1tod  = dest - t1Pos;
		Vec3<float> t2tod  = dest - t2Pos;

		float t1cost = glm::length(t1tod);
		float t2cost = glm::length(t2tod);

		t1cost += glm::length(t1Pos-origin);
		t2cost += glm::length(t2Pos-origin);

		return (t1cost < t2cost);

	}
};

#define THRESHOLD_ITERATIONS 500

static bool findNextNodeOnPath(PathComparer &comparer, TileMap &map, std::list<Tile*> &currentPath, Vec3<int> destination, volatile unsigned long *numIterations)
{
	if (currentPath.back()->position == destination)
		return true;
	if (*numIterations > THRESHOLD_ITERATIONS)
		return false;
	*numIterations = (*numIterations)+1;
	std::vector<Tile*> fringe;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			for (int z = -1; z <= 1; z++)
			{
				Vec3<int> currentPosition = currentPath.back()->position;
				if (z == 0 && y == 0 && x == 0)
					continue;
				Vec3<int> nextPosition = currentPosition;
				nextPosition.x += x;
				nextPosition.y += y;
				nextPosition.z += z;
				if (nextPosition.z < 0 || nextPosition.z >= map.size.z
					|| nextPosition.y < 0 || nextPosition.y >= map.size.y
					|| nextPosition.x < 0 || nextPosition.x >= map.size.x)
					continue;
				Tile *tile = &map.tiles[nextPosition.z][nextPosition.y][nextPosition.x];
				//FIXME: Make 'blocked' tiles cleverer (e.g. don't plan around objects that will move anyway?)
				if (!tile->objects.empty())
					continue;
				//Already visited this tile
				if (std::find(currentPath.begin(), currentPath.end(), tile) != currentPath.end())
					continue;
				fringe.push_back(tile);
			}
		}
	}
	std::sort(fringe.begin(), fringe.end(), comparer);
	for (auto tile : fringe)
	{
		currentPath.push_back(tile);
		comparer.origin = {tile->position.x, tile->position.y, tile->position.z};
		if (findNextNodeOnPath(comparer, map, currentPath, destination, numIterations))
			return true;
		currentPath.pop_back();
	}
	return false;

}


std::list<Tile*>
TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destination)
{
	volatile unsigned long numIterations = 0;
	std::list<Tile*> path;
	PathComparer pc(destination);
	if (origin.x < 0 || origin.x >= this->size.x
		|| origin.y < 0 || origin.y >= this->size.y
		|| origin.z < 0 || origin.z >= this->size.z)
	{
		std::cerr << __func__ << " Bad origin: {" << origin.x << "," << origin.y << "," << origin.z << "}\n";
		return path;
	}
	if (destination.x < 0 || destination.x >= this->size.x
		|| destination.y < 0 || destination.y >= this->size.y
		|| destination.z < 0 || destination.z >= this->size.z)
	{
		std::cerr << __func__ << " Bad destination: {" << destination.x << "," << destination.y << "," << destination.z << "}\n";
		return path;
	}
	path.push_back(&this->tiles[origin.z][origin.y][origin.x]);
	if (!findNextNodeOnPath(pc, *this, path, destination, &numIterations))
	{
		std::cerr << __func__ << " No route found from origin: {" << origin.x << "," << origin.y << "," << origin.z << "} to destination: {" << destination.x << "," << destination.y << "," << destination.z << "}\n";
		path.clear();
		return path;
	}
	return path;
}


}; //namespace OpenApoc
