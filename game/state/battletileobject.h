#pragma once
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{
class Renderer;
class BattleTileMap;
class BattleTile;
class VoxelMap;
enum class TileViewMode;
class TileTransform;

class BattleTileObject : public std::enable_shared_from_this<BattleTileObject>
{
  public:
	enum class Type
	{
		Ground,
		LeftWall,
		RightWall,
		Scenery,
	};

	/* 'screenPosition' is where the center of the object should be drawn */
	virtual void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	                  TileViewMode mode) = 0;
	const Type &getType() const { return this->type; }
	virtual Vec3<float> getPosition() const = 0;

	virtual float getDistanceTo(sp<BattleTileObject> target);
	virtual float getDistanceTo(Vec3<float> target);
	virtual void setPosition(Vec3<float> newPosition);
	virtual void removeFromMap();

	BattleTile *getOwningTile() const { return this->owningTile; }

	virtual sp<VoxelMap> getVoxelMap() { return nullptr; }
	virtual Vec3<float> getVoxelOffset() const { return bounds / 2.0f; }

	virtual const UString &getName() { return this->name; }

	virtual ~BattleTileObject();

	BattleTileMap &map;

  protected:
	friend class BattleTileMap;

	Type type;

	BattleTile *owningTile;
	std::vector<BattleTile *> intersectingTiles;

	BattleTileObject(BattleTileMap &map, Type type, Vec3<float> bounds);

	// The bounds is a cube centered around the 'position' used for stuff like collision detection
	Vec3<float> bounds;
	UString name;
};
}