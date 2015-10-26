#include "game/tileview/tileobject.h"

#include "framework/renderer.h"
#include "game/tileview/tile.h"
#include "framework/logger.h"

#include <cmath>

namespace OpenApoc
{

TileObject::TileObject(TileMap &map, Type type, Vec3<float> position, Vec3<float> bounds)
    : map(map), type(type), owningTile(nullptr), position(position), bounds(bounds)
{
}

TileObject::~TileObject() {}

void TileObject::removeFromMap()
{
	auto thisPtr = shared_from_this();
	/* owner may be NULL as this can be used to set the initial position after creation */
	if (this->owningTile)
	{
		this->owningTile->ownedObjectsNew.erase(thisPtr);
	}
	this->owningTile = nullptr;
	for (auto *tile : this->intersectingTiles)
	{
		tile->intersectingObjectsNew.erase(thisPtr);
	}
	this->intersectingTiles.clear();
}

void TileObject::setPosition(Vec3<float> newPosition)
{
	auto thisPtr = shared_from_this();
	if (newPosition.x < 0 || newPosition.y < 0 || newPosition.z < 0 ||
	    newPosition.x > map.size.x + 1 || newPosition.y > map.size.y + 1 ||
	    newPosition.z > map.size.z + 1)
	{
		LogError("Trying to place object at {%f,%f,%f} in map of size {%d,%d,%d}", newPosition.x,
		         newPosition.y, newPosition.z, map.size.x, map.size.y, map.size.z);
	}
	this->removeFromMap();

	this->owningTile = map.getTile(newPosition);
	if (!this->owningTile)
	{
		LogError("Failed to get tile for position {%f,%f,%f}", newPosition.x, newPosition.y,
		         newPosition.z);
	}

	this->owningTile->ownedObjectsNew.insert(thisPtr);

	Vec3<int> minBounds = {floorf(newPosition.x - this->bounds.x / 2.0f),
	                       floorf(newPosition.y - this->bounds.y / 2.0f),
	                       floorf(newPosition.z - this->bounds.z / 2.0f)};
	Vec3<int> maxBounds = {ceilf(newPosition.x + this->bounds.x / 2.0f),
	                       ceilf(newPosition.y + this->bounds.y / 2.0f),
	                       ceilf(newPosition.z + this->bounds.z / 2.0f)};

	for (int x = minBounds.x; x < maxBounds.x; x++)
	{
		for (int y = minBounds.y; y < maxBounds.y; y++)
		{
			for (int z = minBounds.z; z < maxBounds.z; z++)
			{
				if (x < 0 || y < 0 || z < 0 || x > map.size.x || y > map.size.y || z > map.size.z)
				{
					// TODO: Decide if having bounds outside the map are really valid?
					continue;
				}
				Tile *intersectingTile = map.getTile(x, y, z);
				if (!intersectingTile)
				{
					LogError("Failed to get intersecting tile at {%d,%d,%d}", x, y, z);
					continue;
				}
				this->intersectingTiles.push_back(intersectingTile);
				intersectingTile->intersectingObjectsNew.insert(thisPtr);
			}
		}
	}
	this->position = newPosition;
	// Quick sanity check
	for (auto *t : this->intersectingTiles)
	{
		if (t->intersectingObjectsNew.find(shared_from_this()) == t->intersectingObjectsNew.end())
		{
			LogError("Intersecting objects inconsistent");
		}
	}
}

} // namespace OpenApoc
