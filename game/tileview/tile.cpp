#include "library/sp.h"
#include "game/tileview/tile.h"
#include "framework/framework.h"

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

void TileMap::update(unsigned int ticks)
{
	// Objects may remove themselves from these lists in their update functions.
	// So we can't just use the foreach iterator stuff as we need to
	// store the next iterator before calling update
	auto activeIt = this->activeObjects.begin();
	while (activeIt != this->activeObjects.end())
	{
		// Post-increment to store a copy of the 'next' object iterator
		// before calling the update, as ->update() may destroy the 'current'
		// iterator if it removes the object from the set
		auto objIt = activeIt++;
		(*objIt)->update(ticks);
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

void TileMap::addObject(sp<TileObject> obj)
{
	if (!obj->getOwningTile())
	{
		LogError("Adding object with no owner");
		assert(0);
	}
	obj->getOwningTile()->ownedObjects.insert(obj);
	if (obj->isSelectable())
		this->selectableObjects.insert(obj);
	if (obj->isVisible())
		obj->getOwningTile()->visibleObjects.insert(obj);
	if (obj->isCollidable())
		obj->addToAffectedTiles();
	if (obj->isProjectile())
		this->projectiles.insert(obj);
}

void TileMap::removeObject(sp<TileObject> obj)
{
	if (!obj->getOwningTile())
	{
		LogError("Removing object with no owner");
		assert(0);
	}
	if (obj->isSelectable())
		this->selectableObjects.erase(obj);
	if (obj->isProjectile())
	{
		obj->getOwningTile()->ownedProjectiles.erase(obj);
		this->projectiles.erase(obj);
	}
	if (obj->isVisible())
		obj->getOwningTile()->visibleObjects.erase(obj);
	if (obj->isCollidable())
		obj->removeFromAffectedTiles();
	auto count = obj->getOwningTile()->ownedObjects.erase(obj);
	if (count != 1)
	{
		LogError("Removed %u objects from owning tile", static_cast<unsigned>(count));
	}
}

TileMap::~TileMap() {}

Tile::Tile(TileMap &map, Vec3<int> position) : map(map), position(position) {}

TileObject::TileObject(TileMap &map, Vec3<float> position, bool collides, bool visible,
                       bool projectile, bool selectable)
    : position(position), owningTile(map.getTile(position)), collides(collides), visible(visible),
      projectile(projectile), selectable(selectable)
{
	// May be called by multiple subclass constructors - don't do anything
	// non-repeatable here
}

TileObject::~TileObject() {}

const Vec3<float> &TileObject::getPosition() const { return this->position; }

void TileObject::setPosition(Vec3<float> newPos)
{
	auto thisPtr = shared_from_this();
	this->position = newPos;
	auto &map = this->owningTile->map;
	auto newOwner = map.getTile(this->position);
	if (newOwner != this->owningTile)
	{
		if (this->isVisible())
		{
			this->owningTile->visibleObjects.erase(thisPtr);
			newOwner->visibleObjects.insert(thisPtr);
		}
		if (this->isProjectile())
		{
			this->owningTile->ownedProjectiles.erase(thisPtr);
			newOwner->ownedProjectiles.insert(thisPtr);
		}
		this->owningTile->ownedObjects.erase(thisPtr);
		newOwner->ownedObjects.insert(thisPtr);
		this->owningTile = newOwner;
	}
}

sp<Image> TileObject::getSprite() const
{
	// Override this for visible objects
	LogWarning("Called on non-visible object");
	assert(!this->isVisible());
	return nullptr;
}

sp<Image> TileObject::getStrategySprite() const
{
	// Override this for visible objects
	LogWarning("Called on non-visible object");
	assert(!this->isVisible());
	return nullptr;
}

Vec3<float> TileObject::getDrawPosition() const
{
	// Override this for visible objects
	LogWarning("Called on non-visible object");
	assert(!this->isVisible());
	return this->getPosition();
}

const Vec3<int> &TileObject::getTileSizeInVoxels() const
{
	static Vec3<int> invalidSize{0, 0, 0};
	// Override this for collidable objects
	LogWarning("Called on non-collidable object");
	assert(!this->isCollidable());
	return invalidSize;
}

const Vec3<int> &TileObject::getBounds() const
{
	static Vec3<int> invalidSize{0, 0, 0};
	// Override this for collidable objects
	LogWarning("Called on non-collidable object");
	assert(!this->isCollidable());
	return invalidSize;
}

bool TileObject::hasVoxelAt(const Vec3<float> &worldPosition) const
{
	// Override this for collidable objects
	LogWarning("Called on non-collidable object");
	assert(!this->isCollidable());
	std::ignore = worldPosition;
	return false;
}

void TileObject::handleCollision(const Collision &c)
{
	// Override this for collidable objects
	LogWarning("Called on non-collidable object");
	assert(!this->isCollidable());
	std::ignore = c;
	return;
}

void TileObject::removeFromAffectedTiles()
{
	// Override this for collidable objects
	LogWarning("Called on non-collidable object");
	assert(!this->isCollidable());
	return;
}

void TileObject::addToAffectedTiles()
{
	// Override this for collidable objects
	LogWarning("Called on non-collidable object");
	assert(!this->isCollidable());
	return;
}

void TileObject::checkProjectileCollision()
{
	// Override this for projectile objects
	LogWarning("Called on non-projectile object");
	assert(!this->isProjectile());
	return;
}

void TileObject::drawProjectile(TileView &v, Renderer &r, Vec2<int> screenPosition)
{
	// Override this for projectile objects
	LogWarning("Called on non-projectile object");
	assert(!this->isProjectile());
	std::ignore = v;
	std::ignore = r;
	std::ignore = screenPosition;
	return;
}

Rect<float> TileObject::getSelectableBounds() const
{
	Rect<float> invalidBounds{0, 0, 0, 0};
	// Override this for selectable objects
	LogWarning("Called on non-selectable object");
	assert(!this->isSelectable());
	return invalidBounds;
}

void TileObject::setSelected(bool selected)
{
	// Override this for selectable objects
	LogWarning("Called on non-selectable object");
	assert(!this->isSelectable());
	std::ignore = selected;
}

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

}; // namespace OpenApoc
