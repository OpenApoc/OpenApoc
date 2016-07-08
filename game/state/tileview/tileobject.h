#pragma once

#include "library/sp.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class Renderer;
class TileMap;
class Tile;
class VoxelMap;
enum class TileViewMode;
class TileTransform;

class TileObject : public std::enable_shared_from_this<TileObject>
{
  public:
	enum class Type
	{
		Projectile,
		Vehicle,
		Scenery,
		Doodad,
		Shadow,
	};

	/* 'screenPosition' is where the center of the object should be drawn */
	virtual void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	                  TileViewMode mode) = 0;
	const Type &getType() const { return this->type; }
	virtual Vec3<float> getPosition() const = 0;

	virtual float getDistanceTo(sp<TileObject> target);
	virtual void setPosition(Vec3<float> newPosition);
	virtual void removeFromMap();

	Tile *getOwningTile() const { return this->owningTile; }

	virtual sp<VoxelMap> getVoxelMap() { return nullptr; }
	virtual Vec3<float> getVoxelOffset() const { return bounds / 2.0f; }

	virtual ~TileObject();

	TileMap &map;

  protected:
	friend class TileMap;

	Type type;

	Tile *owningTile;
	std::vector<Tile *> intersectingTiles;

	TileObject(TileMap &map, Type type, Vec3<float> bounds);

	// The bounds is a cube centered around the 'position' used for stuff like collision detection
	Vec3<float> bounds;
};

} // namespace OpenApoc
