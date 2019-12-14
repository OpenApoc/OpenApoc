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
class Image;
enum class TileViewMode;
class TileTransform;

class TileObject : public std::enable_shared_from_this<TileObject>
{
  public:
	enum class Type
	{
		// For purpose of determining draw order, these four must come first
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
		Hazard,
	};

	/* 'screenPosition' is where the center of the object should be drawn */
	virtual void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	                  TileViewMode mode, bool visible = true, int currentLevel = 0,
	                  bool friendly = false, bool hostile = false) = 0;
	const Type &getType() const { return this->type; }
	virtual Vec3<float> getPosition() const = 0;
	// Vector from object position to object center
	virtual Vec3<float> getCenterOffset() const { return {0.0f, 0.0f, 0.0f}; }
	virtual Vec3<float> getCenter() const { return getPosition() + getCenterOffset(); };

	// Used to calculate draw order
	virtual float getZOrder() const { return getCenter().z + (float)getType() / 1000.0f; }

	virtual float getDistanceTo(sp<TileObject> target) const;
	virtual float getDistanceTo(Vec3<float> target) const;
	// For TileObjects, position is usually the location of the object's centre
	// Usually, owner objects will have the same position (i.e. proj.pos == to_proj.pos)
	// Some, however, will not (scenery's position points to top left corner, but
	// getPosition returns centre)
	// Some TileObjects, OTOH, have position pointing to their center on xy and bottom on z
	// This is useful for battlescape where object should be assigned to the tile where
	// it is resting, therefore, where it's bottom is
	virtual void setPosition(Vec3<float> newPosition);
	virtual void removeFromMap();
	virtual void addToDrawnTiles(Tile *tile);

	Tile *getOwningTile() const { return this->owningTile; }
	std::vector<Tile *> getIntersectingTiles() const { return this->intersectingTiles; }

	virtual sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex [[maybe_unused]],
	                                 bool los [[maybe_unused]]) const
	{
		return nullptr;
	}
	virtual bool hasVoxelMap(bool los [[maybe_unused]]) const { return false; }
	// Vector from voxel map top left back corner to object center
	virtual Vec3<float> getVoxelOffset() const { return bounds_div_2; }
	virtual Vec3<float> getVoxelCentrePosition() const { return {0.0, 0.0, 0.0}; }
	Vec3<float> getBounds() const { return bounds; }

	virtual const UString &getName() { return this->name; }

	virtual ~TileObject();

	TileMap &map;

	static void drawTinted(Renderer &r, sp<Image> sprite, Vec2<float> position, bool visible);

	// TileObjects are not copy-able
	TileObject(const TileObject &) = delete;
	// Move is fine
	TileObject(TileObject &&) = default;

  protected:
	friend class TileMap;

	Type type;

	Tile *owningTile;
	Tile *drawOnTile;
	std::vector<Tile *> intersectingTiles;

	TileObject(TileMap &map, Type type, Vec3<float> bounds);

	// The bounds is a cube centered around the 'position' used for stuff like collision detection
	Vec3<float> bounds;
	// Bounds hardly ever change, so this will provide faster calculations
	// Bounds divided by 2, used to find inclusive object boundary's coordinates
	Vec3<float> bounds_div_2;

	virtual void setBounds(Vec3<float> bounds);

	UString name;

  private:
};

} // namespace OpenApoc
