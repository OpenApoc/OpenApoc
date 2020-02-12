#include "game/state/tilemap/tileobject.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "game/state/tilemap/tilemap.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace OpenApoc
{

TileObject::TileObject(TileMap &map, Type type, Vec3<float> bounds)
    : map(map), type(type), owningTile(nullptr), name("UNKNOWN_OBJECT")
{
	setBounds(bounds);
}

TileObject::~TileObject() = default;

void TileObject::setBounds(Vec3<float> bounds)
{
	this->bounds = bounds;
	this->bounds_div_2 = bounds / 2.0f;
}

void TileObject::drawTinted(Renderer &r, sp<Image> sprite, Vec2<float> position, bool visible)
{
	if (visible)
	{
		r.draw(sprite, position);
	}
	else
	{
		r.drawTinted(sprite, position, COLOUR_BLACK);
	}
}

void TileObject::removeFromMap()
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
		this->drawOnTile->drawnObjects[layer].erase(
		    std::remove(this->drawOnTile->drawnObjects[layer].begin(),
		                this->drawOnTile->drawnObjects[layer].end(), thisPtr),
		    this->drawOnTile->drawnObjects[layer].end());
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
class TileObjectZComparer
{
  public:
	bool operator()(const sp<TileObject> &lhs, const sp<TileObject> &rhs) const
	{
		float lhsZ = lhs->getCenter().x * lhs->map.velocityScale.x +
		             lhs->getCenter().y * lhs->map.velocityScale.y +
		             lhs->getZOrder() * lhs->map.velocityScale.z;
		float rhsZ = rhs->getCenter().x * rhs->map.velocityScale.x +
		             rhs->getCenter().y * rhs->map.velocityScale.y +
		             rhs->getZOrder() * rhs->map.velocityScale.z;
		return (lhsZ < rhsZ);
	}
};
} // anonymous namespace

float TileObject::getDistanceTo(sp<TileObject> target) const
{
	return getDistanceTo(target->getCenter());
}

float TileObject::getDistanceTo(Vec3<float> target) const
{
	return glm::length((target - this->getCenter()) * map.velocityScale);
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
		LogWarning("Trying to place object at {} in map of size {}", newPosition, map.size);
		newPosition.x = clamp(newPosition.x, 0.0f, (float)map.size.x + 1);
		newPosition.y = clamp(newPosition.y, 0.0f, (float)map.size.y + 1);
		newPosition.z = clamp(newPosition.z, 0.0f, (float)map.size.z + 1);
		LogWarning("Clamped object to {}", newPosition);
	}
	this->removeFromMap();

	this->owningTile = map.getTile(newPosition);
	if (!this->owningTile)
	{
		LogError("Failed to get tile for position {}", newPosition);
		return;
	}

	auto inserted = this->owningTile->ownedObjects.insert(thisPtr);
	if (!inserted.second)
	{
		LogError("Object already in owned object list?");
	}

	Vec3<int> minBounds = {floorf(newPosition.x + getCenterOffset().x - this->bounds_div_2.x),
	                       floorf(newPosition.y + getCenterOffset().y - this->bounds_div_2.y),
	                       floorf(newPosition.z + getCenterOffset().z - this->bounds_div_2.z)};
	Vec3<int> maxBounds = {ceilf(newPosition.x + getCenterOffset().x + this->bounds_div_2.x),
	                       ceilf(newPosition.y + getCenterOffset().y + this->bounds_div_2.y),
	                       ceilf(newPosition.z + getCenterOffset().z + this->bounds_div_2.z)};

	for (int x = minBounds.x; x < maxBounds.x; x++)
	{
		for (int y = minBounds.y; y < maxBounds.y; y++)
		{
			for (int z = minBounds.z; z < maxBounds.z; z++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= map.size.x || y >= map.size.y ||
				    z >= map.size.z)
				{
					// TODO: Decide if having bounds outside the map are really valid?
					continue;
				}
				Tile *intersectingTile = map.getTile(x, y, z);
				if (!intersectingTile)
				{
					LogError("Failed to get intersecting tile at {{{},{},{}}}", x, y, z);
					continue;
				}
				this->intersectingTiles.push_back(intersectingTile);
				intersectingTile->intersectingObjects.insert(thisPtr);
			}
		}
	}
	// Quick sanity check
	for (auto &t : this->intersectingTiles)
	{
		if (t->intersectingObjects.find(shared_from_this()) == t->intersectingObjects.end())
		{
			LogError("Intersecting objects inconsistent");
		}
	}

	addToDrawnTiles(owningTile);
}

void TileObject::addToDrawnTiles(Tile *tile)
{
	this->drawOnTile = tile;
	int layer = map.getLayer(this->type);
	this->drawOnTile->drawnObjects[layer].push_back(shared_from_this());
	std::sort(this->drawOnTile->drawnObjects[layer].begin(),
	          this->drawOnTile->drawnObjects[layer].end(), TileObjectZComparer{});
}

} // namespace OpenApoc
