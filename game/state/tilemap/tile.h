#pragma once

#include "framework/logger.h"
#include "game/state/gametime.h"
#include "game/state/stateobject.h"
#include "library/colour.h"
#include "library/rect.h"
#include "library/sp.h"
#include <map>
#include <set>
#include <vector>

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
class BattleHazard;
class TileObjectBattleHazard;
class Sample;
class TileObject;
class Organisation;

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

	// Vars

	// Height, in battle its 0-0.975, of the tile's ground and feature's height (height / 40)
	//         in city its 0-0.999, of the tile's scenery height (height / 16.1)
	// Height cannot be 1 as that is equal to 0.000 on the tile below
	float height = 0.0f;
	// Same but for city overlays
	float overlayHeight = 0.0f;
	// Movement cost through the tile's ground (or feature)
	int movementCostIn = 4;
	// Movement cost to walk on the level above this, if next level is empty and this height is
	// 0.975
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
	unsigned int drawBattlescapeSelectionBackAt = 0;
	// position in drawnObjects vector to draw target location at
	unsigned int drawTargetLocationIconAt = 0;
	// sfx to use when passing through tile
	sp<std::vector<sp<Sample>>> walkSfx;
	// sfx to use when object falls on tile
	sp<Sample> objectDropSfx;
	// Solid tileobject in the tile with the highest height that supports items
	sp<BattleMapPart> supportProviderForItems;
	// How much tiles are added to vision distance after passing this tile
	int visionBlockValue = 0;
	// Non-dead scenery present in this tile
	sp<Scenery> presentScenery;

	// Methods

	// Returns unit present in the tile, if any (quick call for just any unit ever)
	sp<TileObjectBattleUnit> getUnitIfPresent() const;
	// Returns unit present in the tile, if any
	sp<TileObjectBattleUnit> getUnitIfPresent(bool onlyConscious, bool mustOccupy = false,
	                                          bool mustBeStatic = false,
	                                          sp<TileObjectBattleUnit> exceptThis = nullptr,
	                                          bool onlyLarge = false,
	                                          bool checkLargeSpace = false) const;
	std::list<sp<BattleUnit>> getUnits(bool onlyConscious, bool mustOccupy = false,
	                                   bool mustBeStatic = false,
	                                   sp<TileObjectBattleUnit> exceptThis = nullptr,
	                                   bool onlyLarge = false, bool checkLargeSpace = false) const;
	// Returns items that can be collected by standing in this tile)
	std::list<sp<BattleItem>> getItems();
	// Returns resting position for items and units in the tile
	Vec3<float> getRestingPosition(bool large = false, bool overlay = false);
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
	// Returns whether tile is an exit
	bool getHasExit(bool large = false);

	// Updaters

	// Updates most battlescape tile parameters
	void updateCityscapeParameters();

	// Updates most battlescape tile parameters
	void updateBattlescapeParameters();
	// Updates "unit present" parameter
	void updateBattlescapeUnitPresent();
	// Updates battlescape ui draw order variables
	void updateBattlescapeUIDrawOrder();
	// Updates vision blockage of the tile
	// returns true if vision changed
	bool updateVisionBlockage(int value = 0);

	bool pathfindingDebugFlag = false;
};

class CanEnterTileHelper
{
  public:
	// Returns true if this object can move from 'from' to 'to'. The two tiles must be adjacent!
	virtual bool canEnterTile(Tile *from, Tile *to, bool allowJumping, bool &jumped, float &cost,
	                          bool &doorInTheWay, bool ignoreStaticUnits = false,
	                          bool ignoreMovingUnits = false,
	                          bool ignoreAllUnits = false) const = 0;
	// Returns true if this object can move from 'from' to 'to'. The two tiles must be adjacent!
	virtual bool canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits = false,
	                          bool ignoreMovingUnits = false,
	                          bool ignoreAllUnits = false) const = 0;
	virtual float adjustCost(Vec3<int> /*  nextPosition */, int /* z */) const { return 0; }
	virtual float getDistance(Vec3<float> from, Vec3<float> to) const = 0;
	virtual float getDistance(Vec3<float> from, Vec3<float> toStart, Vec3<float> toEnd) const = 0;
	virtual ~CanEnterTileHelper() = default;
	// This allows pathfinding to be biased towards goal. Can generate paths that are not perfect,
	// but helps with pathfinding in battlescape where optimal path can be complicated.
	// Value here defines how much, in percent, can resulting path afford to be unoptimal
	// For example 1.05 means resulting path can be 5% longer than an optimal one
	virtual float pathOverheadAlloawnce() const { return 1.0f; }
};

}; // namespace OpenApoc
