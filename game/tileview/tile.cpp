#include "library/sp.h"
#include "game/tileview/tile.h"
#include "framework/framework.h"
#include "game/tileview/tileobject_projectile.h"
#include "game/city/projectile.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/city/vehicle.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/city/scenery.h"
#include "game/tileview/tileobject_doodad.h"
#include "game/city/doodad.h"

namespace OpenApoc
{

TileMap::TileMap(Framework &fw, Vec3<int> size) : fw(fw), size(size)
{
	tiles.reserve(size.z * size.y * size.z);
	for (int z = 0; z < size.z; z++)
	{
		for (int y = 0; y < size.y; y++)
		{
			for (int x = 0; x < size.x; x++)
			{
				tiles.emplace_back(*this, Vec3<int>{x, y, z});
			}
		}
	}
}

Tile *TileMap::getTile(int x, int y, int z)
{
	if (x >= size.x || y >= size.y || z >= size.z)
	{
		LogError("Requesting tile {%d,%d,%d} in map of size {%d,%d,%d}", x, y, z, size.x, size.y,
		         size.z);
		return nullptr;
	}
	return &this->tiles[z * size.x * size.y + y * size.x + x];
}

Tile *TileMap::getTile(Vec3<int> pos) { return getTile(pos.x, pos.y, pos.z); }

Tile *TileMap::getTile(Vec3<float> pos) { return getTile(pos.x, pos.y, pos.z); }

TileMap::~TileMap()
{
	for (auto &t : tiles)
	{
		for (auto &obj : t.ownedObjectsNew)
		{
			obj->removeFromMap();
		}
	}
}

Tile::Tile(TileMap &map, Vec3<int> position) : map(map), position(position) {}

class PathComparer
{
  public:
	Vec3<float> dest;
	Vec3<float> origin;
	PathComparer(Vec3<int> d) : dest{d.x, d.y, d.z} {}
	bool operator()(Tile *t1, Tile *t2)
	{
		Vec3<float> t1Pos{t1->position.x, t1->position.y, t1->position.z};
		Vec3<float> t2Pos{t2->position.x, t2->position.y, t2->position.z};

		Vec3<float> t1tod = dest - t1Pos;
		Vec3<float> t2tod = dest - t2Pos;

		float t1cost = glm::length(t1tod);
		float t2cost = glm::length(t2tod);

		t1cost += glm::length(t1Pos - origin);
		t2cost += glm::length(t2Pos - origin);

		return (t1cost < t2cost);
	}
};

static bool
findNextNodeOnPath(PathComparer &comparer, TileMap &map, std::list<Tile *> &currentPath,
                   Vec3<int> destination, unsigned int *iterationsLeft, const Vehicle &v,
                   std::function<bool(const Tile &tile, const Vehicle &v)> canEnterTileFn)
{
	if (currentPath.back()->position == destination)
		return true;
	if (*iterationsLeft == 0)
		return true;
	*iterationsLeft = (*iterationsLeft) - 1;
	std::vector<Tile *> fringe;
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
				if (nextPosition.z < 0 || nextPosition.z >= map.size.z || nextPosition.y < 0 ||
				    nextPosition.y >= map.size.y || nextPosition.x < 0 ||
				    nextPosition.x >= map.size.x)
					continue;
				Tile *tile = map.getTile(nextPosition);
				// FIXME: Make 'blocked' tiles cleverer (e.g. don't plan around objects that will
				// move anyway?)
				if (!canEnterTileFn(*tile, v))
					continue;
				// Check for diagonal routes that the 'corner' tiles we touch are empty
				Vec3<int> cornerPosition = currentPosition;
				cornerPosition += Vec3<int>{0, y, z};
				if (cornerPosition != currentPosition && !canEnterTileFn(*tile, v))
					continue;
				cornerPosition = currentPosition;
				cornerPosition += Vec3<int>{x, 0, z};
				if (cornerPosition != currentPosition && !canEnterTileFn(*tile, v))
					continue;
				cornerPosition = currentPosition;
				cornerPosition += Vec3<int>{x, y, 0};
				if (cornerPosition != currentPosition && !canEnterTileFn(*tile, v))
					continue;
				// Already visited this tile
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
		if (findNextNodeOnPath(comparer, map, currentPath, destination, iterationsLeft, v,
		                       canEnterTileFn))
			return true;
		currentPath.pop_back();
	}
	return false;
}

std::list<Tile *>
TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destination, unsigned int iterationLimit,
                          const Vehicle &v,
                          std::function<bool(const Tile &tile, const Vehicle &v)> canEnterTileFn)
{
	std::list<Tile *> path;
	PathComparer pc(destination);
	unsigned int iterationsLeft = iterationLimit;
	if (origin.x < 0 || origin.x >= this->size.x || origin.y < 0 || origin.y >= this->size.y ||
	    origin.z < 0 || origin.z >= this->size.z)
	{
		LogError("Bad origin {%d,%d,%d}", origin.x, origin.y, origin.z);
		return path;
	}
	if (destination.x < 0 || destination.x >= this->size.x || destination.y < 0 ||
	    destination.y >= this->size.y || destination.z < 0 || destination.z >= this->size.z)
	{
		LogError("Bad destination {%d,%d,%d}", destination.x, destination.y, destination.z);
		return path;
	}
	path.push_back(this->getTile(origin));
	if (!findNextNodeOnPath(pc, *this, path, destination, &iterationsLeft, v, canEnterTileFn))
	{
		LogWarning("No route found from origin {%d,%d,%d} to desination {%d,%d,%d}", origin.x,
		           origin.y, origin.z, destination.x, destination.y, destination.z);
		return {};
	}
	auto &currentHeadPos = path.back()->position;
	if (currentHeadPos != destination)
	{
		/* Step back up the path until we find the node 'closest' to the target
		 * as if we're blocked by something that will move that's probably a
		 * better starting point for the next loop instead of after possibly
		 * starting to move further from the obstacle trying to path round it*/
		Vec3<float> dest_float{destination.x, destination.y, destination.z};
		float closestCost = std::numeric_limits<float>::max();
		Tile *closestTile = path.front();

		for (auto *t : path)
		{
			if (t->position == origin)
			{
				/* We want to move at least /somewhere/ */
				continue;
			}
			float cost =
			    glm::length(Vec3<float>{t->position.x, t->position.y, t->position.z} - dest_float);
			if (cost < closestCost)
			{
				closestCost = cost;
				closestTile = t;
			}
		}

		while (!path.empty() && path.back() != closestTile)
		{
			path.pop_back();
		}

		auto closestPos = closestTile->position;

		LogWarning(
		    "No route found from origin {%d,%d,%d} to desination {%d,%d,%d} in %u iterations, "
		    "closest node {%d,%d,%d} (%d steps)",
		    origin.x, origin.y, origin.z, destination.x, destination.y, destination.z,
		    iterationLimit, closestPos.x, closestPos.y, closestPos.z, path.size());
	}
	return path;
}

sp<TileObjectProjectile> TileMap::addObjectToMap(sp<Projectile> projectile)
{
	if (projectile->tileObject)
	{
		LogError("Projectile already has tile object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectProjectile> obj(new TileObjectProjectile(*this, projectile));
	obj->setPosition(projectile->getPosition());
	projectile->tileObject = obj;

	return obj;
}

sp<TileObjectVehicle> TileMap::addObjectToMap(sp<Vehicle> vehicle)
{
	if (vehicle->tileObject)
	{
		LogError("Vehicle already has tile object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectVehicle> obj(new TileObjectVehicle(*this, vehicle));
	obj->setPosition(vehicle->getPosition());
	vehicle->tileObject = obj;

	return obj;
}

sp<TileObjectScenery> TileMap::addObjectToMap(sp<Scenery> scenery)
{
	if (scenery->tileObject)
	{
		LogError("Scenery already has tile object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectScenery> obj(new TileObjectScenery(*this, scenery));
	obj->setPosition(scenery->getPosition());
	scenery->tileObject = obj;

	return obj;
}

sp<TileObjectDoodad> TileMap::addObjectToMap(sp<Doodad> doodad)
{
	if (doodad->tileObject)
	{
		LogError("Doodad already has tile object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectDoodad> obj(new TileObjectDoodad(*this, doodad));
	obj->setPosition(doodad->getPosition());
	doodad->tileObject = obj;

	return obj;
}
}; // namespace OpenApoc
