#include "game/state/battletile.h"
#include "framework/logger.h"
#include "game/state/battletileobject.h"

namespace OpenApoc
{
BattleTileObject::BattleTileObject(BattleTileMap &map, Type type, Vec3<float> bounds)
    : map(map), type(type), owningTile(nullptr), bounds(bounds), name("UNKNOWN_OBJECT")
{
}

BattleTileObject::~BattleTileObject() = default;

void BattleTileObject::removeFromMap()
{
	auto thisPtr = shared_from_this();
	/* owner may be NULL as this can be used to set the initial position after creation */
	if (this->owningTile)
	{
		auto erased = this->owningTile->ownedObjects.erase(thisPtr);
		if (erased != 1)
		{
			LogError("Nothing erased?");
		}
		int layer = map.getLayer(this->type);
		this->owningTile->drawnObjects[layer].erase(
		    std::remove(this->owningTile->drawnObjects[layer].begin(),
		                this->owningTile->drawnObjects[layer].end(), thisPtr),
		    this->owningTile->drawnObjects[layer].end());
		this->owningTile = nullptr;
	}
	for (auto *tile : this->intersectingTiles)
	{
		tile->intersectingObjects.erase(thisPtr);
	}
	this->intersectingTiles.clear();
}

namespace
{
class BattleTileObjectZComparer
{
  public:
	bool operator()(const sp<BattleTileObject> &lhs, const sp<BattleTileObject> &rhs) const
	{
		// First sort objects based on wehter they belong to the Ground, Left Wall, Right Wall or
		// are something other than that (we don't care what exactly)
		int lhsT = std::min((int)lhs->getType(), 4);
		int rhsT = std::min((int)rhs->getType(), 4);
		if (lhsT != rhsT)
			return lhsT < rhsT;

		// If both objects are of the same mappart type (or are both other type) then proceed to
		// check their Z
		float lhsZ = lhs->getPosition().x * 32.0f + lhs->getPosition().y * 32.0f +
		             lhs->getPosition().z * 16.0f;
		float rhsZ = rhs->getPosition().x * 32.0f + rhs->getPosition().y * 32.0f +
		             rhs->getPosition().z * 16.0f;
		// FIXME: Hack to force 'overlay' objects to be half-a-tile up in Z
		/*if (lhs->getType() == TileObject::Type::Doodad)
		{
		lhsZ += (32.0f + 32.0f + 16.0f) / 2.0f;
		}
		if (rhs->getType() == BattleTileObject::Type::Doodad)
		{
		rhsZ += (32.0f + 32.0f + 16.0f) / 2.0f;
		}*/
		return (lhsZ < rhsZ);
	}
};
} // anonymous namespace

float BattleTileObject::getDistanceTo(sp<BattleTileObject> target)
{
	return getDistanceTo(target->getPosition());
}

float BattleTileObject::getDistanceTo(Vec3<float> target)
{
	return glm::length((target - this->getPosition()) * VELOCITY_SCALE);
}

void BattleTileObject::setPosition(Vec3<float> newPosition)
{
	auto thisPtr = shared_from_this();
	if (!thisPtr)
	{
		LogError("This == null");
	}
	if (newPosition.x < 0 || newPosition.y < 0 || newPosition.z < 0 ||
	    newPosition.x > map.size.x + 1 || newPosition.y > map.size.y + 1 ||
	    newPosition.z > map.size.z + 1)
	{
		LogWarning("Trying to place object at {%f,%f,%f} in map of size {%d,%d,%d}", newPosition.x,
		           newPosition.y, newPosition.z, map.size.x, map.size.y, map.size.z);
		newPosition.x = clamp(newPosition.x, 0.0f, (float)map.size.x + 1);
		newPosition.y = clamp(newPosition.y, 0.0f, (float)map.size.y + 1);
		newPosition.z = clamp(newPosition.z, 0.0f, (float)map.size.z + 1);
		LogWarning("Clamped object to {%f,%f,%f}", newPosition.x, newPosition.y, newPosition.z);
	}
	this->removeFromMap();

	this->owningTile = map.getTile(newPosition);
	if (!this->owningTile)
	{
		LogError("Failed to get tile for position {%f,%f,%f}", newPosition.x, newPosition.y,
		         newPosition.z);
	}

	auto inserted = this->owningTile->ownedObjects.insert(thisPtr);
	if (!inserted.second)
	{
		LogError("Object already in owned object list?");
	}

	int layer = map.getLayer(this->type);

	this->owningTile->drawnObjects[layer].push_back(thisPtr);
	std::sort(this->owningTile->drawnObjects[layer].begin(),
	          this->owningTile->drawnObjects[layer].end(), BattleTileObjectZComparer{});

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
				BattleTile *intersectingTile = map.getTile(x, y, z);
				if (!intersectingTile)
				{
					LogError("Failed to get intersecting tile at {%d,%d,%d}", x, y, z);
					continue;
				}
				this->intersectingTiles.push_back(intersectingTile);
				intersectingTile->intersectingObjects.insert(thisPtr);
			}
		}
	}
	// Quick sanity check
	for (auto *t : this->intersectingTiles)
	{
		if (t->intersectingObjects.find(shared_from_this()) == t->intersectingObjects.end())
		{
			LogError("Intersecting objects inconsistent");
		}
	}
}
}