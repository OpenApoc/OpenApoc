#include "game/state/tileview/tileobject.h"
#include "framework/logger.h"
#include "game/state/tileview/tile.h"
#include <algorithm>
#include <cmath>

namespace OpenApoc
{

TileObject::TileObject(TileMap &map, Type type, Vec3<float> bounds)
    : map(map), type(type), owningTile(nullptr), tileOffset(0.0f,0.0f,0.0f),
	name("UNKNOWN_OBJECT")
{
	setBounds(bounds);
}

TileObject::~TileObject() = default;

void TileObject::setBounds(Vec3<float> bounds)
{
	this->bounds = bounds;
	this->bounds_div_2 = bounds / 2.0f;
}

void TileObject::removeFromMap()
{
	auto thisPtr = shared_from_this();
	/* owner may be NULL as this can be used to set the initial position after creation */
	if (this->owningTile)
	{
		auto t = owningTile;
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
		if (type == Type::Ground || type == Type::LeftWall || type == Type::RightWall || type == Type::Feature)
			t->updateHeightAndPassability();
	}
	for (auto *tile : this->intersectingTiles)
	{
		tile->intersectingObjects.erase(thisPtr);
	}
	this->intersectingTiles.clear();
}

namespace
{
class TileObjectZComparer
{
  public:
	bool operator()(const sp<TileObject> &lhs, const sp<TileObject> &rhs) const
	{
		// First sort objects based on wehter they belong to the Ground, Left Wall, Right Wall or
		// are something other than that (we don't care what exactly)
		int lhsT = std::min((int)lhs->getType(), 4);
		int rhsT = std::min((int)rhs->getType(), 4);
		if (lhsT != rhsT)
			return lhsT < rhsT;

		// If both objects are of the same mappart type (or are both other type) 
		// then proceed to check their Z. However, type remains the tiebreaker
		float lhsZ = lhs->getPosition().x * lhs->map.velocityScale.x +
		             lhs->getPosition().y * lhs->map.velocityScale.y +
		             lhs->getPosition().z * lhs->map.velocityScale.z + (float)lhs->getType() / 1000.0f;
		float rhsZ = rhs->getPosition().x * rhs->map.velocityScale.x +
		             rhs->getPosition().y * rhs->map.velocityScale.y +
		             rhs->getPosition().z * rhs->map.velocityScale.z + (float)rhs->getType() / 1000.0f;
		// FIXME: Hack to force 'overlay' objects to be half-a-tile up in Z
		if (lhs->getType() == TileObject::Type::Doodad)
		{
			lhsZ +=
			    (lhs->map.velocityScale.x + lhs->map.velocityScale.y + lhs->map.velocityScale.z) /
			    2.0f;
		}
		if (rhs->getType() == TileObject::Type::Doodad)
		{
			rhsZ +=
			    (rhs->map.velocityScale.x + rhs->map.velocityScale.y + rhs->map.velocityScale.z) /
			    2.0f;
		}
		return (lhsZ < rhsZ);
	}
};
} // anonymous namespace

float TileObject::getDistanceTo(sp<TileObject> target)
{
	return getDistanceTo(target->getPosition());
}

float TileObject::getDistanceTo(Vec3<float> target)
{
	return glm::length((target - this->getPosition()) * map.velocityScale);
}

void TileObject::setPosition(Vec3<float> newPosition)
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

	// This makes sure object is always assigned the bottom-most, right-most tile it occupies
	this->owningTile = map.getTile(newPosition + this->tileOffset);
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
	          this->owningTile->drawnObjects[layer].end(), TileObjectZComparer{});

	Vec3<int> minBounds = {floorf(newPosition.x - this->bounds_div_2.x),
	                       floorf(newPosition.y - this->bounds_div_2.y),
	                       floorf(newPosition.z - this->bounds_div_2.z)};
	Vec3<int> maxBounds = {ceilf(newPosition.x + this->bounds_div_2.x),
	                       ceilf(newPosition.y + this->bounds_div_2.y),
	                       ceilf(newPosition.z + this->bounds_div_2.z)};

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
	if (type == Type::Ground || type == Type::LeftWall || type == Type::RightWall || type == Type::Feature)
		owningTile->updateHeightAndPassability();
}

} // namespace OpenApoc
