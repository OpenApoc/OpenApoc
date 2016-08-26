#pragma once
#include "framework/includes.h"
#include "framework/logger.h"
#include "game/state/battletileobject.h"
#include "game/state/tileview/tile.h"

namespace OpenApoc
{
class Image;
class BattleTileMap;
class CollisionB;
class VoxelMap;
class Renderer;
class BattleMapPart;
class BattleTileObjectMapPart;

class BattleTile
{
  public:
	BattleTileMap &map;
	Vec3<int> position;

	std::set<sp<BattleTileObject>> ownedObjects;
	std::set<sp<BattleTileObject>> intersectingObjects;

	// FIXME: This is effectively a z-sorted list of ownedObjects - can this be merged somehow?
	std::vector<std::vector<sp<BattleTileObject>>> drawnObjects;

	BattleTile(BattleTileMap &map, Vec3<int> position, int layerCount);
};

class BattleTileMap
{
  private:
	std::vector<BattleTile> tiles;
	std::vector<std::set<BattleTileObject::Type>> layerMap;

  public:
	const BattleTile *getTile(int x, int y, int z) const
	{
		LogAssert(x >= 0);
		LogAssert(x < size.x);
		LogAssert(y >= 0);
		LogAssert(y < size.y);
		LogAssert(z >= 0);
		LogAssert(z < size.z);
		return &this->tiles[z * size.x * size.y + y * size.x + x];
	}
	BattleTile *getTile(int x, int y, int z)
	{
		LogAssert(x >= 0);
		LogAssert(x < size.x);
		LogAssert(y >= 0);
		LogAssert(y < size.y);
		LogAssert(z >= 0);
		LogAssert(z < size.z);
		return &this->tiles[z * size.x * size.y + y * size.x + x];
	}
	BattleTile *getTile(Vec3<int> pos) { return this->getTile(pos.x, pos.y, pos.z); }
	const BattleTile *getTile(Vec3<int> pos) const { return this->getTile(pos.x, pos.y, pos.z); }
	// Returns the tile this point is 'within'
	BattleTile *getTile(Vec3<float> pos)
	{
		return this->getTile(static_cast<int>(pos.x), static_cast<int>(pos.y),
		                     static_cast<int>(pos.z));
	}
	const BattleTile *getTile(Vec3<float> pos) const
	{
		return this->getTile(static_cast<int>(pos.x), static_cast<int>(pos.y),
		                     static_cast<int>(pos.z));
	}
	Vec3<int> size;

	BattleTileMap(Vec3<int> size, std::vector<std::set<BattleTileObject::Type>> layerMap);
	~BattleTileMap();

	CollisionB findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd) const;

	void addObjectToMap(sp<BattleMapPart>);

	int getLayer(BattleTileObject::Type type) const;
	int getLayerCount() const;
	bool tileIsValid(Vec3<int> tile) const;

	sp<Image> dumpVoxelView(const Rect<int> viewRect, const TileTransform &transform) const;
};
}