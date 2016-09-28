#pragma once
#include "framework/includes.h"
#include "framework/logger.h"
#include "game/state/battle/battle.h"
#include "game/state/tileview/tileobject.h"
#include "library/sp.h"
#include <functional>
#include <set>
#include <vector>

#define TICK_SCALE (36)
#define VELOCITY_SCALE_CITY (Vec3<float>{32, 32, 16})
#define VELOCITY_SCALE_BATTLE (Vec3<float>{24, 24, 20})
#define FALLING_SPEED_CAP float(20)

// This enables showing tiles that were tried by the last pathfinding attempt
// Is only displayed in battlescape right now
//#define PATHFINDING_DEBUG

namespace OpenApoc
{

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
class BattleMapPart;
class TileObjectBattleMapPart;
class BattleUnit;
class TileObjectBattleUnit;
class BattleItem;
class TileObjectBattleItem;
class Sample;

class TileTransform
{
  public:
	virtual Vec2<float> tileToScreenCoords(Vec3<float> coords) const = 0;
	virtual Vec3<float> screenToTileCoords(Vec2<float> screenPos, float z) const = 0;
};

enum class TileViewMode
{
	Isometric,
	Strategy,
};

class Tile
{
  public:
	TileMap &map;
	Vec3<int> position;

	std::set<sp<TileObject>> ownedObjects;
	std::set<sp<TileObject>> intersectingObjects;

	// FIXME: This is effectively a z-sorted list of ownedObjects - can this be merged somehow?
	// Alexey Andronov (Istrebitel): This is no longer so, because
	// units are drawn on a tile different to their owner tile.
	std::vector<std::vector<sp<TileObject>>> drawnObjects;

	Tile(TileMap &map, Vec3<int> position, int layerCount);

	// Only used by Battle

	// Returns unit present in the tile, if any (quick call for just any unit ever)
	sp<TileObjectBattleUnit> getUnitIfPresent();
	// Returns unit present in the tile, if any
	sp<TileObjectBattleUnit> getUnitIfPresent(bool onlyConscious, bool mustOccupy = false,
	                                          bool mustBeStatic = false,
	                                          sp<TileObjectBattleUnit> exceptThis = nullptr,
	                                          bool onlyLarge = false);
	// Returns resting position for items and units in the tile
	Vec3<float> getRestingPosition(bool large = false);
	// Returns the object that provides support (resting position) for items
	sp<BattleMapPart> getItemSupportingObject();
	// Returns if the tile is passable (including side tiles for large)
	bool getPassable(bool large = false, int height = 0);
	// Returns if head fits in the tile
	bool getHeadFits(bool large, int height);
	// Returns if the tile provides ground for standing (including side tiles for large)
	bool getCanStand(bool large = false);
	// Returns if the tile is solid (cannot pop head into it)
	bool getSolidGround(bool large = false);
	// Returns wether tile is an exit
	bool getHasExit(bool large = false);
	// Updates most battlescape tile parameters
	void updateBattlescapeParameters();
	// Updates "unit present" parameter
	void updateBattlescapeUnitPresent();
	// Updates battlescape ui draw order variables
	void updateBattlescapeUIDrawOrder();

	// Height, 0-0.975, of the tile's ground and feature's height
	// Height cannot be 0 as that is equal to 1.000 on the tile below
	float height = 0.0f;
	// Movement cost through the tile's ground (or feature)
	int movementCostIn = 4;
	// Movement cost to walk on the level above this, if next level is empty and this height is 0.975
	int movementCostOver = 255;
	// Movement cost through the tile's left wall
	int movementCostLeft = 0;
	// Movement cost through the tile's right wall
	int movementCostRight = 0;
	// True = there is currently a closed door to the left of this tile
	bool closedDoorLeft = false;
	// True = there is currently a closed door to the right of this tile
	bool closedDoorRight = false;
	// Tile provides solid ground for standing.
	// True = cannot pop head into this tile when ascending
	bool solidGround = false;
	// False = only flyers can stand here, True = anyone can
	bool canStand = false;
	// True = anyone can go upwards from this tile to another lift tile
	bool hasLift = false;
	// True = clicking move to this tile will make soldier retreat on arrival
	// and will override squad movement pattern (everybody will move only to exists in the vicinity)
	bool hasExit = false;
	// True = unit is present in this tile
	sp<TileObjectBattleUnit> firstUnitPresent;
	// True = unit that qualifies as a door opener present in this tile
	bool doorOpeningUnitPresent = false;
	// position in drawnObjects vector to draw back selection bracket at
	int drawBattlescapeSelectionBackAt = 0;
	// position in drawnObjects vector to draw target location at
	int drawTargetLocationIconAt = 0;
	// sfx to use when passing through tile
	sp<std::vector<sp<Sample>>> walkSfx;
	// sfx to use when object falls on tile
	sp<Sample> objectDropSfx;
	// Solid tileobject in the tile with the highest height that supports items
	sp<BattleMapPart> supportProviderForItems;

#ifdef PATHFINDING_DEBUG
	bool pathfindingDebugFlag = false;
#endif
};

class CanEnterTileHelper
{
  public:
	// Returns true if this object can move from 'from' to 'to'. The two tiles must be adjacent!
	virtual bool canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay,
	                          bool demandGiveWay = false) const = 0;
	// Returns true if this object can move from 'from' to 'to'. The two tiles must be adjacent!
	virtual bool canEnterTile(Tile *from, Tile *to, bool demandGiveWay = false) const = 0;
	virtual float adjustCost(Vec3<int> /*  nextPosition */, int /* z */) const { return 0; }
	virtual float getDistance(Vec3<float> from, Vec3<float> to) const = 0;
	virtual ~CanEnterTileHelper() = default;
	// This allows pathfinding to be biased towards goal. Can generate paths that are not perfect, 
	// but helps with pathfinding in battlescape where optimal path can be complicated.
	// Value here defines how much, in percent, can resulting path afford to be unoptimal
	// For example 1.05 means resulting path can be 5% longer than an optimal one
	virtual float applyPathOverheadAllowance(float cost) const { return cost / 1.0f; }
};

class TileMap
{
  private:
	std::vector<Tile> tiles;
	std::vector<std::set<TileObject::Type>> layerMap;

  public:
	const Tile *getTile(int x, int y, int z) const
	{
		
		if (!((x >= 0)
			&& (x < size.x)
			&& (y >= 0)
			&& (y < size.y)
			&& (z >= 0)
			&& (z < size.z)))
		{
			LogError("Incorrect tile coordinates %d,%d,%d", x, y, z);
			return nullptr;
		}
		return &this->tiles[z * size.x * size.y + y * size.x + x];
	}
	Tile *getTile(int x, int y, int z)
	{
		if (!((x >= 0)
			&& (x < size.x)
			&& (y >= 0)
			&& (y < size.y)
			&& (z >= 0)
			&& (z < size.z)))
		{
			LogError("Incorrect tile coordinates %d,%d,%d", x, y, z);
			return nullptr;
		}
		return &this->tiles[z * size.x * size.y + y * size.x + x];
	}
	Tile *getTile(Vec3<int> pos) { return this->getTile(pos.x, pos.y, pos.z); }
	const Tile *getTile(Vec3<int> pos) const { return this->getTile(pos.x, pos.y, pos.z); }
	// Returns the tile this point is 'within'
	Tile *getTile(Vec3<float> pos)
	{
		return this->getTile(static_cast<int>(pos.x), static_cast<int>(pos.y),
		                     static_cast<int>(pos.z));
	}
	const Tile *getTile(Vec3<float> pos) const
	{
		return this->getTile(static_cast<int>(pos.x), static_cast<int>(pos.y),
		                     static_cast<int>(pos.z));
	}
	Vec3<int> size;
	Vec3<int> voxelMapSize;
	Vec3<float> velocityScale;

	TileMap(Vec3<int> size, Vec3<float> velocityScale, Vec3<int> voxelMapSize,
	        std::vector<std::set<TileObject::Type>> layerMap);
	~TileMap();

	std::list<Vec3<int>> findShortestPath(Vec3<int> origin, Vec3<int> destination,
	                                   unsigned int iterationLimit,
	                                   const CanEnterTileHelper &canEnterTile,
	                                   float altitude = 5.0f, bool demandGiveWay = false);

	Collision findCollision(Vec3<float> lineSegmentStart, Vec3<float> lineSegmentEnd,
	                        std::set<TileObject::Type> validTypes = {},
	                        bool check_full_path = false) const;

	void addObjectToMap(sp<Projectile>);
	void addObjectToMap(sp<Vehicle>);
	void addObjectToMap(sp<Scenery>);
	void addObjectToMap(sp<Doodad>);
	void addObjectToMap(sp<BattleMapPart>);
	void addObjectToMap(sp<BattleItem>);
	void addObjectToMap(sp<BattleUnit>);

	int getLayer(TileObject::Type type) const;
	int getLayerCount() const;
	bool tileIsValid(Vec3<int> tile) const;

	sp<Image> dumpVoxelView(const Rect<int> viewRect, const TileTransform &transform, float maxZ,
	                        bool fast = false) const;
};
}; // namespace OpenApoc
