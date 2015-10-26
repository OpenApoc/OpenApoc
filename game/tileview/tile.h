#pragma once
#include "library/sp.h"

#include "framework/includes.h"
#include <set>
#include <functional>

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
class TileObject;
class Projectile;
class TileObjectProjectile;
class Vehicle;
class TileObjectVehicle;
class Scenery;
class TileObjectScenery;

class Tile
{
  public:
	TileMap &map;
	Vec3<int> position;

	std::set<sp<TileObject>> ownedObjectsNew;
	std::set<sp<TileObject>> intersectingObjectsNew;

	Tile(TileMap &map, Vec3<int> position);

	Collision findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd);
};

class TileMap
{
  private:
	std::vector<Tile> tiles;

  public:
	Framework &fw;
	Tile *getTile(int x, int y, int z);
	Tile *getTile(Vec3<int> pos);
	// Returns the tile this point is 'within'
	Tile *getTile(Vec3<float> pos);
	Vec3<int> size;

	TileMap(Framework &fw, Vec3<int> size);
	~TileMap();

	std::list<Tile *>
	findShortestPath(Vec3<int> origin, Vec3<int> destination, unsigned int iterationLimit,
	                 const Vehicle &v,
	                 std::function<bool(const Tile &tile, const Vehicle &v)> canEnterTileFn);

	Collision findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd);

	sp<TileObjectProjectile> addObjectToMap(sp<Projectile>);
	sp<TileObjectVehicle> addObjectToMap(sp<Vehicle>);
	sp<TileObjectScenery> addObjectToMap(sp<Scenery>);
};
}; // namespace OpenApoc
