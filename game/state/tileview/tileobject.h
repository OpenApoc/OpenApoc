#pragma once

#include "library/sp.h"
#include "library/strings.h"
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
		// For easier checking of Z-levels in battlescape, first four entries
		// here must be the four battle map parts in their drawing order
		Ground,
		LeftWall,
		RightWall,
		Feature,

		// From here on in, anything else goes. However, object type is a tiebreaker 
		// in case both objects are located on the same exact position
		Shadow, 
		Item,
		Unit,
		Projectile,
		Vehicle,
		Scenery,
		Doodad,
	};

	/* 'screenPosition' is where the center of the object should be drawn */
	virtual void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	                  TileViewMode mode, int currentLevel) = 0;
	const Type &getType() const { return this->type; }
	virtual Vec3<float> getPosition() const = 0;

	virtual float getDistanceTo(sp<TileObject> target);
	virtual float getDistanceTo(Vec3<float> target);
	virtual void setPosition(Vec3<float> newPosition);
	virtual void removeFromMap();

	Tile *getOwningTile() const { return this->owningTile; }

	virtual sp<VoxelMap> getVoxelMap() { return nullptr; }
	virtual Vec3<float> getVoxelOffset() const { return bounds_div_2; }

	virtual const UString &getName() { return this->name; }

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
	// Bounds never change, so this will provide faster calculations
	// Bounds divided by 2, used to find inclusive object boundary's coordinates
	Vec3<float> bounds_div_2;
	// Bounds divided by 2, subtracting 1, used to find exclusive object boundary's coordinates
	Vec3<float> bounds_div_2_sub_1;

	void setBounds(Vec3<float> bounds);

	UString name;
};

} // namespace OpenApoc
