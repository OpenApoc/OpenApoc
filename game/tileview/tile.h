#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include "game/tileview/tileobject.h"
#include <set>
#include <functional>
#include <vector>

// DANGER WILL ROBINSON - MADE UP VALUES AHEAD
// I suspect quantities of distance/velocity are stored in units of {32,32,16} (same as the voxel
// size)?
// And progressing them by 1/15th of that every tick looks about right?
#define TICK_SCALE (15)
#define VELOCITY_SCALE (Vec3<float>{32, 32, 16})

namespace OpenApoc
{

class Framework;
class Image;
class TileMap;
class Tile;
class Collision;
class VoxelMap;
class Renderer;
class TileView;
class Projectile;
class TileObjectProjectile;
class Vehicle;
class TileObjectVehicle;
class Scenery;
class TileObjectScenery;
class Doodad;
class TileObjectDoodad;

class Tile
{
  public:
	TileMap &map;
	Vec3<int> position;

	std::set<sp<TileObject>> ownedObjects;
	std::set<sp<TileObject>> intersectingObjects;

	// FIXME: This is effectively a z-sorted list of ownedObjects - can this be merged somehow?
	std::vector<std::vector<sp<TileObject>>> drawnObjects;

	Tile(TileMap &map, Vec3<int> position, int layerCount);

	Collision findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd);
};

class CanEnterTileHelper
{
  public:
	// Returns true if this object can move from 'from' to 'to'. The two tiles must be adjacent!
	virtual bool canEnterTile(Tile *from, Tile *to) const = 0;
};

class TileMap
{
  private:
	std::vector<Tile> tiles;
	std::vector<std::set<TileObject::Type>> layerMap;

  public:
	Framework &fw;
	Tile *getTile(int x, int y, int z);
	Tile *getTile(Vec3<int> pos);
	// Returns the tile this point is 'within'
	Tile *getTile(Vec3<float> pos);
	Vec3<int> size;

	TileMap(Framework &fw, Vec3<int> size, std::vector<std::set<TileObject::Type>> layerMap);
	~TileMap();

	std::list<Tile *> findShortestPath(Vec3<int> origin, Vec3<int> destination,
	                                   unsigned int iterationLimit,
	                                   const CanEnterTileHelper &canEnterTile);

	Collision findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd);

	void addObjectToMap(sp<Projectile>);
	void addObjectToMap(sp<Vehicle>);
	void addObjectToMap(sp<Scenery>);
	void addObjectToMap(sp<Doodad>);

	int getLayer(TileObject::Type type) const;
	int getLayerCount() const;
};
}; // namespace OpenApoc
