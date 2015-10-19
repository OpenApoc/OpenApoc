#pragma once
#include "library/sp.h"

#include "game/tileview/tile.h"

namespace OpenApoc
{

class VoxelMap;
class Projectile;

class TileObjectCollidable : virtual public TileObject
{
  protected:
	std::set<Tile *> affectedTiles;
	Vec3<int> tileSizeInVoxels;
	Vec3<int> bounds;
	sp<VoxelMap> voxels;

  public:
	virtual const Vec3<int> &getTileSizeInVoxels() const override;
	virtual const Vec3<int> &getBounds() const override;
	virtual bool hasVoxelAt(const Vec3<float> &worldPosition) const override;
	virtual void handleCollision(const Collision &c) override;

	virtual void setPosition(Vec3<float> newPos) override;

	virtual void removeFromAffectedTiles() override;
	virtual void addToAffectedTiles() override;

	TileObjectCollidable(TileMap &map, Vec3<float> position, Vec3<int> tileSizeInVoxels,
	                     sp<VoxelMap> voxels);
};

class Collision
{
  public:
	sp<TileObject> tileObject;
	sp<Projectile> projectile;
	Vec3<float> position;
	bool operator!() { return !!tileObject; }
	explicit operator bool() const { return tileObject != nullptr; }
};

}; // namespace OpenApoc
