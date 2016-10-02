#include "game/state/battle/battleunitmission.h"
#include "framework/framework.h"
#include "game/state/battle/battlecommonsamplelist.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"

namespace OpenApoc
{

bool BattleUnitTileHelper::canEnterTile(Tile *from, Tile *to, bool demandGiveWay) const
{
	float nothing;
	bool none;
	return canEnterTile(from, to, nothing, none, false, demandGiveWay);
}

bool BattleUnitTileHelper::canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay,
                                        bool demandGiveWay) const
{
	return canEnterTile(from, to, cost, doorInTheWay, false, demandGiveWay);
}

bool BattleUnitTileHelper::canEnterTile(Tile *from, Tile *to, bool ignoreUnits,
                                        bool demandGiveWay) const
{
	float nothing;
	bool none;
	return canEnterTile(from, to, nothing, none, ignoreUnits, demandGiveWay);
}

bool BattleUnitTileHelper::canEnterTile(Tile *from, Tile *to, float &cost, bool &doorInTheWay,
                                        bool ignoreUnits, bool demandGiveWay) const
{
	int costInt = 0;
	doorInTheWay = false;

	// Error checks
	if (!from)
	{
		LogError("No 'from' position supplied");
		return false;
	}
	Vec3<int> fromPos = from->position;
	if (!to)
	{
		LogError("No 'to' position supplied");
		return false;
	}
	Vec3<int> toPos = to->position;
	if (fromPos == toPos)
	{
		LogError("FromPos == ToPos {%d,%d,%d}", toPos.x, toPos.y, toPos.z);
		return false;
	}
	if (!map.tileIsValid(fromPos))
	{
		LogError("FromPos {%d,%d,%d} is not on the map", fromPos.x, fromPos.y, fromPos.z);
		return false;
	}
	if (!map.tileIsValid(toPos))
	{
		LogError("ToPos {%d,%d,%d} is not on the map", toPos.x, toPos.y, toPos.z);
		return false;
	}

	// Unit parameters
	bool large = u.isLarge();
	bool flying = u.canFly();
	// Tiles used by big units
	Tile *fromX1 = nullptr;  // from (x-1, y, z)
	Vec3<int> fromX1Pos;     // fromPos (x-1, y, z)
	Tile *fromY1 = nullptr;  // from (x, y-1, z)
	Vec3<int> fromY1Pos;     // fromPos (x, y-1, z)
	Tile *fromXY1 = nullptr; // from (x-1, y-1, z)
	Vec3<int> fromXY1Pos;    // fromPos (x-1, y-1, z)
	Tile *toX1 = nullptr;    // to (x-1, y, z)
	Vec3<int> toX1Pos;       // toPos (x-1, y, z)
	Tile *toY1 = nullptr;    // to (x, y-1, z)
	Vec3<int> toY1Pos;       // toPos (x, y-1, z)
	Tile *toXY1 = nullptr;   // to (x-1, y-1, z)
	Vec3<int> toXY1Pos;      // toPos (x-1, y-1, z)
	Tile *toZ1 = nullptr;    // to (x, y, z-1)
	Vec3<int> toZ1Pos;       // toPos (x, y, z-1)
	Tile *toXZ1 = nullptr;   // to (x-1, y, z-1)
	Vec3<int> toXZ1Pos;      // toPos (x-1, y, z-1)
	Tile *toYZ1 = nullptr;   // to (x, y-1, z-1)
	Vec3<int> toYZ1Pos;      // toPos (x, y-1, z-1)
	Tile *toXYZ1 = nullptr;  // to (x-1, y-1, z-1)
	Vec3<int> toXYZ1Pos;     // toPos (x-1, y-1, z-1)

	// STEP 01: Check if "to" is passable
	// We could just use Tile::getPassable, however, we need to make some extra calculations
	// Like store the movement cost, look for units and so on
	// Therefore, I think it's better to do it all here
	// Plus, we will re-use some of the tiles we got from the map later down the line

	// STEP 01: Check if "to" is passable (large)
	if (large)
	{
		// Can we fit?
		if (toPos.x < 1 || toPos.y < 1 || toPos.z + 1 >= map.size.z)
		{
			return false;
		}
		// Get tiles
		fromX1 = map.getTile(Vec3<int>{fromPos.x - 1, fromPos.y, fromPos.z});
		fromX1Pos = fromX1->position;
		fromY1 = map.getTile(Vec3<int>{fromPos.x, fromPos.y - 1, fromPos.z});
		fromY1Pos = fromY1->position;
		fromXY1 = map.getTile(Vec3<int>{fromPos.x - 1, fromPos.y - 1, fromPos.z});
		fromXY1Pos = fromXY1->position;

		toX1 = map.getTile(Vec3<int>{toPos.x - 1, toPos.y, toPos.z});
		toX1Pos = toX1->position;
		toY1 = map.getTile(Vec3<int>{toPos.x, toPos.y - 1, toPos.z});
		toY1Pos = toY1->position;
		toXY1 = map.getTile(Vec3<int>{toPos.x - 1, toPos.y - 1, toPos.z});
		toXY1Pos = toXY1->position;
		toZ1 = map.getTile(Vec3<int>{toPos.x, toPos.y, toPos.z + 1});
		toZ1Pos = toZ1->position;
		toXZ1 = map.getTile(Vec3<int>{toPos.x - 1, toPos.y, toPos.z + 1});
		toXZ1Pos = toXZ1->position;
		toYZ1 = map.getTile(Vec3<int>{toPos.x, toPos.y - 1, toPos.z + 1});
		toYZ1Pos = toYZ1->position;
		toXYZ1 = map.getTile(Vec3<int>{toPos.x - 1, toPos.y - 1, toPos.z + 1});
		toXYZ1Pos = toXYZ1->position;

		// Check if we can place our head there
		if (toZ1->solidGround || toXZ1->solidGround || toYZ1->solidGround || toXYZ1->solidGround)
		{
			return false;
		}
		// Check if no static unit occupies it
		if (!ignoreUnits)
		{
			if (to->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
			if (toX1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
			if (toY1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
			if (toXY1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
			if (toZ1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
			if (toXZ1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
			if (toYZ1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
			if (toXYZ1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
				return false;
		}
		// Movement cost into the tiles
		costInt = to->movementCostIn;
		costInt = std::max(costInt, toX1->movementCostIn);
		costInt = std::max(costInt, toY1->movementCostIn);
		costInt = std::max(costInt, toXY1->movementCostIn);
		costInt = std::max(costInt, toZ1->movementCostIn);
		costInt = std::max(costInt, toXZ1->movementCostIn);
		costInt = std::max(costInt, toYZ1->movementCostIn);
		costInt = std::max(costInt, toXYZ1->movementCostIn);
		// Movement cost into the walls of the tiles
		costInt = std::max(costInt, to->movementCostLeft);
		costInt = std::max(costInt, toX1->movementCostRight);
		costInt = std::max(costInt, toY1->movementCostLeft);
		costInt = std::max(costInt, toZ1->movementCostLeft);
		costInt = std::max(costInt, toZ1->movementCostRight);
		costInt = std::max(costInt, toXZ1->movementCostRight);
		costInt = std::max(costInt, toYZ1->movementCostLeft);
		// Check for doors
		doorInTheWay = doorInTheWay || to->closedDoorLeft;
		doorInTheWay = doorInTheWay || to->closedDoorRight;
		doorInTheWay = doorInTheWay || toX1->closedDoorRight;
		doorInTheWay = doorInTheWay || toY1->closedDoorLeft;
		doorInTheWay = doorInTheWay || toZ1->closedDoorLeft;
		doorInTheWay = doorInTheWay || toZ1->closedDoorRight;
		doorInTheWay = doorInTheWay || toXZ1->closedDoorRight;
		doorInTheWay = doorInTheWay || toYZ1->closedDoorLeft;
	}
	// STEP 01: Check if "to" is passable (small)
	else
	{
		// Check that no static unit occupies this tile
		if (!ignoreUnits && to->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay))
			return false;
		// Movement cost into the tiles
		costInt = to->movementCostIn;
	}
	// STEP 01: Failure condition
	if (costInt == 255)
	{
		return false;
	}

	// STEP 02: Disallow picking a path that will make unit fall
	// This line prevents soldiers from picking a route that will make them fall
	// Disabling it will allow paths with falling
	if (!flying)
	{
		bool canStand = to->canStand;
		if (large)
		{
			canStand = canStand || toX1->canStand;
			canStand = canStand || toY1->canStand;
			canStand = canStand || toXY1->canStand;
		}
		if (!canStand)
			return false;
	}

	// STEP 03: Falling and going down the lift
	// STEP 03.01: Check if falling, and exit immediately
	// If falling, then we can only go down, and for free!
	// However, vanilla disallowed that, and instead never let soldiers pick this option
	// So, unless we allow going into non-solid ground for non-flyers,
	// this will never happen (except when giving orders to a falling unit)
	if (!flying)
	{
		bool canStand = from->canStand;
		if (large)
		{
			canStand = canStand || fromX1->canStand;
			canStand = canStand || fromY1->canStand;
			canStand = canStand || fromXY1->canStand;
		}
		if (!canStand)
		{
			if (fromPos.x != toPos.x || fromPos.y != toPos.y || fromPos.z >= toPos.z)
			{
				return false;
			}
			cost = 0.0f;
			return true;
		}
	}
	// STEP 03.02: Check if using lift to go down properly
	if (toPos.z < fromPos.z)
	{
		// If going down and in a lift, we can only go strictly down
		if (fromPos.x != toPos.x || fromPos.y != toPos.y)
		{
			bool fromHasLift = false;
			if (large)
			{
				fromHasLift = from->hasLift || fromX1->hasLift || fromY1->hasLift || fromXY1->hasLift;
			}
			else
			{
				fromHasLift = from->hasLift;
			}
			if (fromHasLift)
			{
				return false;
			}
		}
	}

	// STEP 04: Check if we can ascend (if ascending)
	if (toPos.z > fromPos.z)
	{
		// STEP 04.01: Check if we either stand high enough or are using a lift properly

		// Alexey Andronov (Istrebitel):
		// As per my experiments, having a height value of 26 or more is sufficient
		// to ascend to the next level, no matter how high is the ground level there
		// Since we're storing values from 1 to 40, not from 0 to 39,
		// as in the mappart_type, and store them in float, we compare to 27/40 here
		// This doesn't work on lifts. To ascend into a lift, we must be under it
		// We can always ascend if we're on a lift and going above into a lift
		// We can only ascend into a lift if we're flying or standing beneath it
		bool fromHeightSatisfactory = false;
		bool fromHasLift = false;
		bool toHasLift = false;
		if (large)
		{
			fromHeightSatisfactory = from->height >= 0.675f || fromX1->height >= 0.675f ||
			                         fromY1->height >= 0.675f || fromXY1->height >= 0.675f;
			fromHasLift = from->hasLift || fromX1->hasLift || fromY1->hasLift || fromXY1->hasLift;
			toHasLift = to->hasLift || toX1->hasLift || toY1->hasLift || toXY1->hasLift;
		}
		else
		{
			fromHeightSatisfactory = from->height >= 0.675f;
			fromHasLift = from->hasLift;
			toHasLift = to->hasLift;
		}
		// Success condition: Either of:
		// - We stand high enough and target location is not a lift
		// - We stand on lift, target has lift, and we're moving strictly up
		if (!(fromHeightSatisfactory && !toHasLift) &&
		    !(fromHasLift && toHasLift && toPos.x == fromPos.x && toPos.y == fromPos.y))
		{
			// Non-flyers cannot ascend this way
			if (!flying)
			{
				return false;
			}
			// If flying we can only ascend if target tile is not solid ground
			bool canStand = to->canStand;
			if (large)
			{
				canStand = canStand || toX1->canStand;
				canStand = canStand || toY1->canStand;
				canStand = canStand || toXY1->canStand;
			}
			if (canStand)
			{
				return false;
			}
		}

		// STEP 04.02: Check if we will not bump our head upon departure
		if (large)
		{
			// Will we bump our head when leaving current spot?
			// Check four tiles above our "from"'s head
			if (map.getTile(Vec3<int>{fromPos.x, fromPos.y, fromPos.z + 2})->solidGround ||
			    map.getTile(Vec3<int>{fromX1Pos.x, fromX1Pos.y, fromX1Pos.z + 2})->solidGround ||
			    map.getTile(Vec3<int>{fromY1Pos.x, fromY1Pos.y, fromY1Pos.z + 2})->solidGround ||
			    map.getTile(Vec3<int>{fromXY1Pos.x, fromXY1Pos.y, fromXY1Pos.z + 2})->solidGround)
			{
				return false;
			}
		}
		else
		{
			// Will we bump our head when leaving current spot?
			// Check tile above our "from"'s head
			if (map.getTile(Vec3<int>{fromPos.x, fromPos.y, fromPos.z + 1})->solidGround)
			{
				return false;
			}
		}
	}
	
	// STEP 05: Check if we have enough space for our head upon arrival
	if (!to->getHeadFits(large, u.agent->type->bodyType->maxHeight))
		return false;

	// STEP 06: Check how much it costs to pass through walls we intersect with
	// Check if these walls are passable
	// Also check if we bump into scenery or units
	// Also check if we bump into floor upon descending
	// Also check if we bump into lifts upon descending
	// (unless going strictly down, we cannot intersect lifts)
	// If going up, check on upper level, otherwise check on current level
	int z = std::max(fromPos.z, toPos.z);
	// If going down, additionally check that ground tiles we intersect are empty
	bool goingDown = fromPos.z > toPos.z;
	// STEP 06: For large units
	if (large)
	{
		// STEP 06: [For large units if moving diagonally]
		if (fromPos.x != toPos.x && fromPos.y != toPos.y)
		{
			// STEP 06: [For large units if moving: down-right or up-left / SE or NW]
			if (fromPos.x - toPos.x == fromPos.y - toPos.y)
			{
				//  Legend:
				//	  * = initial position
				//	  - = destination
				//	  0 = tile whose walls are involved
				//	  x = walls that are checked
				//
				//	****   ****  x            ----   ----  x
				//	*  *   *  *  x 0          -  -   -  -  x 0
				//	****   ****  x            ----   ----  x
				//	                               \\             
				//	****   ****  xxxx         ----   ****  xxxx
				//	*  *   *  *  - 0-         -  -   *  *  * 0*
				//	****   ****  ----         ----   ****  ****
				//	           \\                                 
				//	xxxx   x---  ----         xxxx   x***  ****
				//	  0    x 0-  -  -           0    x 0*  *  *
				//	       x---  ----                x***  ****

				Tile *rightTopZ0 =
				    map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y) - 2, z);
				Tile *rightBottomZ0 =
				    map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y) - 1, z);
				Tile *bottomLeftZ0 =
				    map.getTile(std::max(fromPos.x, toPos.x) - 2, std::max(fromPos.y, toPos.y), z);
				Tile *bottomRightZ0 =
				    map.getTile(std::max(fromPos.x, toPos.x) - 1, std::max(fromPos.y, toPos.y), z);
				Tile *rightTopZ1 = map.getTile(std::max(fromPos.x, toPos.x),
				                               std::max(fromPos.y, toPos.y) - 2, z + 1);
				Tile *rightBottomZ1 = map.getTile(std::max(fromPos.x, toPos.x),
				                                  std::max(fromPos.y, toPos.y) - 1, z + 1);
				Tile *bottomLeftZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 2,
				                                 std::max(fromPos.y, toPos.y), z + 1);
				Tile *bottomRightZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 1,
				                                  std::max(fromPos.y, toPos.y), z + 1);

				// STEP 06: [For large units if moving: down-right or up-left / SE or NW]
				// Find highest movement cost amongst all walls we intersect
				costInt = std::max(costInt, rightTopZ0->movementCostLeft);
				costInt = std::max(costInt, rightBottomZ0->movementCostRight);
				costInt = std::max(costInt, bottomLeftZ0->movementCostRight);
				costInt = std::max(costInt, bottomRightZ0->movementCostLeft);
				costInt = std::max(costInt, rightTopZ1->movementCostLeft);
				costInt = std::max(costInt, rightBottomZ1->movementCostRight);
				costInt = std::max(costInt, bottomLeftZ1->movementCostRight);
				costInt = std::max(costInt, bottomRightZ1->movementCostLeft);
				// Check door state
				doorInTheWay = doorInTheWay || rightTopZ0->closedDoorLeft;
				doorInTheWay = doorInTheWay || rightBottomZ0->closedDoorRight;
				doorInTheWay = doorInTheWay || bottomLeftZ0->closedDoorRight;
				doorInTheWay = doorInTheWay || bottomRightZ0->closedDoorLeft;
				doorInTheWay = doorInTheWay || rightTopZ1->closedDoorLeft;
				doorInTheWay = doorInTheWay || rightBottomZ1->closedDoorRight;
				doorInTheWay = doorInTheWay || bottomLeftZ1->closedDoorRight;
				doorInTheWay = doorInTheWay || bottomRightZ1->closedDoorLeft;

				// STEP 06: [For large units if moving: down-right or up-left / SE or NW]
				// Diagonally located tiles cannot have impassable scenery or static units
				if (bottomLeftZ0->movementCostIn == 255 || rightTopZ0->movementCostIn == 255 ||
				    bottomLeftZ1->movementCostIn == 255 || rightTopZ1->movementCostIn == 255)
				{
					return false;
				}
				if (!ignoreUnits &&
				    (bottomLeftZ0->getUnitIfPresent(true, true, true, u.tileObject,
				                                    demandGiveWay) ||
				     rightTopZ0->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay) ||
				     bottomLeftZ1->getUnitIfPresent(true, true, true, u.tileObject,
				                                    demandGiveWay) ||
				     rightTopZ1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay)))
				{
					return false;
				}

				// STEP 06: [For large units if moving: down-right or up-left / SE or NW]
				// If going down, check that we're not bumping into ground or gravlift on "from"
				// level 
				if (goingDown)
				{
					// Going down-right
					if (toPos.x > fromPos.x)
					{
						auto edge = map.getTile(toPos.x, toPos.y, toPos.z + 2);
						// Legend: * = from, + = "to" tile, X = tiles we already have
						//  **X
						//  **X
						//  XX+
						// Must check 5 tiles above our head, already have 4 of them
						if (edge->solidGround || edge->hasLift || rightBottomZ1->solidGround ||
						    rightBottomZ1->hasLift || bottomRightZ1->solidGround ||
						    bottomRightZ1->hasLift || rightTopZ1->solidGround ||
						    rightTopZ1->hasLift || bottomLeftZ1->solidGround ||
						    bottomLeftZ1->hasLift)
						{
							return false;
						}
					}
					// Going up-left
					else
					{
						auto leftTop = map.getTile(toPos.x - 1, toPos.y - 1, toPos.z + 2);
						auto leftMiddle = map.getTile(toPos.x - 1, toPos.y, toPos.z + 2);
						auto topMiddle = map.getTile(toPos.x, toPos.y - 1, toPos.z + 2);
						// Legend: * = from, + = "to" tile, X = tiles we already have
						//  xxX
						//  x+*
						//  X**
						// Must check 5 tiles above our head, already have 2 of them
						if (leftMiddle->solidGround || leftMiddle->hasLift ||
						    leftTop->solidGround || leftTop->hasLift || topMiddle->solidGround ||
						    topMiddle->hasLift || rightTopZ1->solidGround || rightTopZ1->hasLift ||
						    bottomLeftZ1->solidGround || bottomLeftZ1->hasLift || toZ1->hasLift ||
						    toXZ1->hasLift || toYZ1->hasLift || toXYZ1->hasLift)
						{
							return false;
						}
					}
				}
			}
			// STEP 06: [For large units if moving: down-left or up-right / NE or SW]
			else
			{
				//	Legend:
				//    * = initial position
				//    - = destination
				//    0 = tile whose walls are involved
				//    x = walls that are checked
				//
				//	      x***  ****               x---  ----
				//	      x 0*  *  *               x 0-  -  -
				//	      x***  ****               x---  ----
				//	                                   //
				//	xxxx  ****  ****         xxxx  ****  ----
				//	- 0-  *  *  *  *         * 0*  *  *  -  -
				//	----  ****  ****         ****  ****  ----
				//	    //
				//	----  ----  xxxx         ****  ****  xxxx
				//	-  -  -  -  x 0          *  *  *  *  x 0
				//	----  ----  x            ****  ****  x

				Tile *topLeftZ0 = map.getTile(std::max(fromPos.x, toPos.x) - 2,
				                              std::max(fromPos.y, toPos.y) - 2, z);
				Tile *topZ0 = map.getTile(std::max(fromPos.x, toPos.x) - 1,
				                          std::max(fromPos.y, toPos.y) - 2, z);
				Tile *leftZ0 = map.getTile(std::max(fromPos.x, toPos.x) - 2,
				                           std::max(fromPos.y, toPos.y) - 1, z);
				Tile *bottomRightZ0 =
				    map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);
				Tile *topLeftZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 2,
				                              std::max(fromPos.y, toPos.y) - 2, z + 1);
				Tile *topZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 1,
				                          std::max(fromPos.y, toPos.y) - 2, z + 1);
				Tile *leftZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 2,
				                           std::max(fromPos.y, toPos.y) - 1, z + 1);
				Tile *bottomRightZ1 =
				    map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z + 1);

				// STEP 06: [For large units if moving: down-left or up-right / NE or SW]
				// Find highest movement cost amongst all walls we intersect
				costInt = std::max(costInt, topZ0->movementCostLeft);
				costInt = std::max(costInt, leftZ0->movementCostRight);
				costInt = std::max(costInt, bottomRightZ0->movementCostLeft);
				costInt = std::max(costInt, bottomRightZ0->movementCostRight);
				costInt = std::max(costInt, topZ1->movementCostLeft);
				costInt = std::max(costInt, leftZ1->movementCostRight);
				costInt = std::max(costInt, bottomRightZ1->movementCostLeft);
				costInt = std::max(costInt, bottomRightZ1->movementCostRight);
				// Check door state
				doorInTheWay = doorInTheWay || topZ0->closedDoorLeft;
				doorInTheWay = doorInTheWay || leftZ0->closedDoorRight;
				doorInTheWay = doorInTheWay || bottomRightZ0->closedDoorLeft;
				doorInTheWay = doorInTheWay || bottomRightZ0->closedDoorRight;
				doorInTheWay = doorInTheWay || topZ1->closedDoorLeft;
				doorInTheWay = doorInTheWay || leftZ1->closedDoorRight;
				doorInTheWay = doorInTheWay || bottomRightZ1->closedDoorLeft;
				doorInTheWay = doorInTheWay || bottomRightZ1->closedDoorRight;

				// STEP 06: [For large units if moving: down-left or up-right / NE or SW]
				// Diagonally located tiles cannot have impassable scenery or static units
				if (topLeftZ0->movementCostIn == 255 || bottomRightZ0->movementCostIn == 255 ||
				    topLeftZ1->movementCostIn == 255 || bottomRightZ1->movementCostIn == 255)
				{
					return false;
				}
				if (!ignoreUnits &&
				    (topLeftZ0->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay) ||
				     bottomRightZ0->getUnitIfPresent(true, true, true, u.tileObject,
				                                     demandGiveWay) ||
				     topLeftZ1->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay) ||
				     bottomRightZ1->getUnitIfPresent(true, true, true, u.tileObject,
				                                     demandGiveWay)))
				{
					return false;
				}

				// STEP 06: [For large units if moving: down-left or up-right / NE or SW]
				// If going down, check that we're not bumping into ground or gravlift on "from"
				// level
				if (goingDown)
				{
					// Going up-right
					if (toPos.x > fromPos.x)
					{
						auto rightMiddle = map.getTile(toPos.x, toPos.y, toPos.z + 2);
						auto rightTop = map.getTile(toPos.x, toPos.y - 1, toPos.z + 2);
						// Legend: * = from, + = "to" tile, X = tiles we already have
						//  XXx
						//  **+
						//  **X
						// Must check 5 tiles above our head, already have 3 of them
						if (rightMiddle->solidGround || rightMiddle->hasLift ||
						    rightTop->solidGround || rightTop->hasLift || topLeftZ1->solidGround ||
						    topLeftZ1->hasLift || topZ1->solidGround || topZ1->hasLift ||
						    bottomRightZ1->solidGround || bottomRightZ1->hasLift || toZ1->hasLift ||
						    toXZ1->hasLift || toYZ1->hasLift || toXYZ1->hasLift)
						{
							return false;
						}
					}
					// Going bottom-left
					else
					{
						auto bottomLeft = map.getTile(toPos.x - 1, toPos.y, toPos.z + 2);
						auto bottomMiddle = map.getTile(toPos.x, toPos.y, toPos.z + 2);
						// Legend: * = from, + = "to" tile, X = tiles we already have
						//  X**
						//  X**
						//  x+X
						// Must check 5 tiles above our head, already have 2 of them
						if (bottomLeft->solidGround || bottomLeft->hasLift ||
						    bottomMiddle->solidGround || bottomMiddle->hasLift ||
						    topLeftZ1->solidGround || topLeftZ1->hasLift || leftZ1->solidGround ||
						    leftZ1->hasLift || bottomRightZ1->solidGround ||
						    bottomRightZ1->hasLift || toZ1->hasLift || toXZ1->hasLift ||
						    toYZ1->hasLift || toXYZ1->hasLift)
						{
							return false;
						}
					}
				}
			}
		}
		// STEP 06: [For large units if moving linearly]
		else
		{
			// STEP 06: [For large units if moving along X]
			if (fromPos.x != toPos.x)
			{
				Tile *topZ0 = map.getTile(toPos.x, toPos.y - 1, z);
				Tile *bottomz0 = map.getTile(toPos.x, toPos.y, z);
				Tile *topZ1 = map.getTile(toPos.x, toPos.y - 1, z + 1);
				Tile *bottomZ1 = map.getTile(toPos.x, toPos.y, z + 1);

				// STEP 06: [For large units if moving along X]
				// Find highest movement cost amongst all walls we intersect
				costInt = std::max(costInt, topZ0->movementCostLeft);
				costInt = std::max(costInt, bottomz0->movementCostLeft);
				costInt = std::max(costInt, topZ1->movementCostLeft);
				costInt = std::max(costInt, bottomZ1->movementCostLeft);
				// Check door state
				doorInTheWay = doorInTheWay || topZ0->closedDoorLeft;
				doorInTheWay = doorInTheWay || bottomz0->closedDoorLeft;
				doorInTheWay = doorInTheWay || topZ1->closedDoorLeft;
				doorInTheWay = doorInTheWay || bottomZ1->closedDoorLeft;

				// Do not have to check for units because we already checked in STEP 01
				// Do not have to check for scenery because that's included in movement cost

				// STEP 06: [For large units if moving along X]
				// If going down must check each tile above our head for presence of solid ground
				// We already checked for solid ground presence on our head level in STEP 01
				// We still have to check it for gravlift though
				if (goingDown)
				{
					auto topOther = map.getTile(toPos.x - 1, toPos.y - 1, toPos.z + 2);
					auto bottomOther = map.getTile(toPos.x - 1, toPos.y, toPos.z + 2);
					if (topZ1->solidGround || topZ1->hasLift || bottomZ1->solidGround ||
					    bottomZ1->hasLift || topOther->solidGround || topOther->hasLift ||
					    bottomOther->solidGround || bottomOther->hasLift || toZ1->hasLift ||
					    toXZ1->hasLift || toYZ1->hasLift || toXYZ1->hasLift)
					{
						return false;
					}
				}
			}
			// STEP 06: [For large units if moving along Y]
			else if (fromPos.y != toPos.y)
			{
				Tile *leftZ0 = map.getTile(toPos.x - 1, toPos.y, z);
				Tile *rightZ0 = map.getTile(toPos.x, toPos.y, z);
				Tile *leftZ1 = map.getTile(toPos.x - 1, toPos.y, z + 1);
				Tile *rightZ1 = map.getTile(toPos.x, toPos.y, z + 1);

				// STEP 06: [For large units if moving along Y]
				// Find highest movement cost amongst all walls we intersect
				costInt = std::max(costInt, leftZ0->movementCostRight);
				costInt = std::max(costInt, rightZ0->movementCostRight);
				costInt = std::max(costInt, leftZ1->movementCostRight);
				costInt = std::max(costInt, rightZ1->movementCostRight);
				// Check door state
				doorInTheWay = doorInTheWay || leftZ0->closedDoorRight;
				doorInTheWay = doorInTheWay || rightZ0->closedDoorRight;
				doorInTheWay = doorInTheWay || leftZ1->closedDoorRight;
				doorInTheWay = doorInTheWay || rightZ1->closedDoorRight;

				// Do not have to check for units because we already did in STEP 01
				// Do not have to check for scenery because that's included in movement cost

				// STEP 06: [For large units if moving along Y]
				// If going down must check each tile above our head for presence of solid ground
				// We already checked for solid ground presence on our head level in STEP 01
				// We still have to check it for gravlift though
				if (goingDown)
				{
					auto leftOther = map.getTile(toPos.x - 1, toPos.y - 1, toPos.z + 2);
					auto rightOther = map.getTile(toPos.x, toPos.y - 1, toPos.z + 2);
					if (leftZ1->solidGround || leftZ1->hasLift || rightZ1->solidGround ||
					    rightZ1->hasLift || leftOther->solidGround || leftOther->hasLift ||
					    rightOther->solidGround || rightOther->hasLift || toZ1->hasLift ||
					    toXZ1->hasLift || toYZ1->hasLift || toXYZ1->hasLift)
					{
						return false;
					}
				}
			}
			// STEP 06: [For large units if moving up or down]
			else if (goingDown)
			{
				// Do not have to check for units because we already did in STEP 01

				// Cannot descend if on solid ground
				if (from->solidGround || fromX1->solidGround || fromY1->solidGround ||
				    fromXY1->solidGround)
				{
					return false;
				}
			}
		}
	}
	// STEP 06: [For small units ]
	else
	{
		// STEP 06: [For small units if moving diagonally]
		if (fromPos.x != toPos.x && fromPos.y != toPos.y)
		{
			//  Legend:
			//	  * = initial position
			//	  - = destination
			//	  0 = tile whose walls are involved
			//	  x = walls that are checked
			//
			//		    x---           ****  x
			//		    x 0-           *  *  x 0
			//		    x---           ****  x
			//		//                   \\      
			//	xxxx  xxxx           xxxx  xxxx
			//	* 0*  x 0              0   x 0-
			//	****  x                    x---
			//
			//
			//		    x***           ----  x
			//		    x 0*           -  -  x 0
			//		    x***           ----  x
			//		//                   \\      
			//	xxxx  xxxx           xxxx  xxxx
			//	- 0-  x 0              0   x 0*
			//	----  x                    x***

			Tile *topLeft =
			    map.getTile(std::min(fromPos.x, toPos.x), std::min(fromPos.y, toPos.y), z);
			Tile *topRight =
			    map.getTile(std::max(fromPos.x, toPos.x), std::min(fromPos.y, toPos.y), z);
			Tile *bottomLeft =
			    map.getTile(std::min(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);
			Tile *bottomRight =
			    map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);

			// STEP 06: [For small units if moving diagonally]
			// Find highest movement cost amongst all walls we intersect
			costInt = std::max(costInt, topRight->movementCostLeft);
			costInt = std::max(costInt, bottomLeft->movementCostRight);
			costInt = std::max(costInt, bottomRight->movementCostLeft);
			costInt = std::max(costInt, bottomRight->movementCostRight);
			// Check door state
			doorInTheWay = doorInTheWay || topRight->closedDoorLeft;
			doorInTheWay = doorInTheWay || bottomLeft->closedDoorRight;
			doorInTheWay = doorInTheWay || bottomRight->closedDoorLeft;
			doorInTheWay = doorInTheWay || bottomRight->closedDoorRight;

			// STEP 06: [For small units if moving diagonally down-right or up-left]
			// Diagonally located tiles cannot have impassable scenery or static units
			if (fromPos.x - toPos.x == fromPos.y - toPos.y)
			{
				if (bottomLeft->movementCostIn == 255 || topRight->movementCostIn == 255)
				{
					return false;
				}
				if (!ignoreUnits &&
				    (bottomLeft->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay) ||
				     topRight->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay)))
				{
					return false;
				}
			}
			// STEP 06: [For small units if moving diagonally down-left or up-right]
			// Diagonally located tiles cannot have impassable scenery or static units
			else
			{
				if (topLeft->movementCostIn == 255 || bottomRight->movementCostIn == 255)
				{
					return false;
				}
				if (!ignoreUnits &&
				    (topLeft->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay) ||
				     bottomRight->getUnitIfPresent(true, true, true, u.tileObject, demandGiveWay)))
				{
					return false;
				}
			}

			// STEP 06: [For small units if moving diagonally]
			if (goingDown)
			{
				// We cannot have solid ground or lift in any of the three tiles besides ours
				if (!(toPos.x > fromPos.x && toPos.y > fromPos.y) &&
				    (topLeft->solidGround || topLeft->hasLift))
				{
					return false;
				}
				if (!(toPos.x < fromPos.x && toPos.y > fromPos.y) &&
				    (topRight->solidGround || topRight->hasLift))
				{
					return false;
				}
				if (!(toPos.x > fromPos.x && toPos.y < fromPos.y) &&
				    (bottomLeft->solidGround || bottomLeft->hasLift))
				{
					return false;
				}
				if (!(toPos.x < fromPos.x && toPos.y < fromPos.y) &&
				    (bottomRight->solidGround || bottomRight->hasLift))
				{
					return false;
				}
			}
		}
		// STEP 06: [For small units if moving linearly]
		else
		{
			Tile *bottomRight =
			    map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);

			// STEP 06: [For small units if moving along X]
			if (fromPos.x != toPos.x)
			{
				costInt = std::max(costInt, bottomRight->movementCostLeft);
				doorInTheWay = doorInTheWay || bottomRight->closedDoorLeft;

				// Do not have to check for units because we already did in STEP 01
				// Do not have to check for scenery because that's included in movement cost

				// Cannot go down if above target tile is solid ground or gravlift
				if (goingDown)
				{
					auto t = map.getTile(toPos.x, toPos.y, toPos.z + 1);
					if (t->solidGround || t->hasLift)
					{
						return false;
					}
				}
			}
			// STEP 06: [For small units if moving along Y]
			else if (fromPos.y != toPos.y)
			{
				costInt = std::max(costInt, bottomRight->movementCostRight);
				doorInTheWay = doorInTheWay || bottomRight->closedDoorRight;

				// Do not have to check for units because we already did in STEP 01
				// Do not have to check for scenery because that's included in movement cost

				// Cannot go down if above target tile is solid ground or gravlift
				if (goingDown)
				{
					auto t = map.getTile(toPos.x, toPos.y, toPos.z + 1);
					if (t->solidGround || t->hasLift)
					{
						return false;
					}
				}
			}
			// STEP 06: [For small units if moving up/down]
			else if (goingDown)
			{
				// Do not have to check for units because we already did in STEP 01

				// Cannot descend if on solid ground
				if (from->solidGround)
				{
					return false;
				}
			}
		}
	}
	// STEP 06: Failure condition
	if (costInt == 255)
	{
		return false;
	}

	// STEP 07: Calculate movement cost modifier
	// It costs 1x to move to adjacent tile, 1.5x to move diagonally,
	// 2x to move diagonally to another layer.
	// Also, it costs 0x to fall, but we have checked for that above
	cost = (float)costInt;
	float costModifier = 0.5f;
	if (fromPos.x != toPos.x)
	{
		costModifier += 0.5f;
	}
	if (fromPos.y != toPos.y)
	{
		costModifier += 0.5f;
	}
	if (fromPos.z != toPos.z)
	{
		costModifier += 0.5f;
	}
	cost *= costModifier;

	return true;
}

float BattleUnitTileHelper::getDistance(Vec3<float> from, Vec3<float> to) const
{
	auto diff = to - from;
	auto xDiff = std::abs(diff.x);
	auto yDiff = std::abs(diff.y);
	auto zDiff = std::abs(diff.z);
	return (std::max(std::max(xDiff, yDiff), zDiff) + xDiff + yDiff + zDiff) * 2.0f;
}

BattleUnitMission *BattleUnitMission::gotoLocation(BattleUnit &u, Vec3<int> target, int facingDelta,
                                                   bool allowSkipNodes, int giveWayAttempts,
                                                   bool demandGiveWay, bool allowRunningAway)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::GotoLocation;
	mission->targetLocation = target;
	mission->giveWayAttemptsRemaining = giveWayAttempts;
	mission->allowSkipNodes = allowSkipNodes;
	mission->demandGiveWay = demandGiveWay;
	mission->allowRunningAway = allowRunningAway;
	return mission;
}

BattleUnitMission *BattleUnitMission::snooze(BattleUnit &, unsigned int snoozeTicks)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::Snooze;
	mission->timeToSnooze = snoozeTicks;
	return mission;
}

BattleUnitMission *BattleUnitMission::restartNextMission(BattleUnit &)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::RestartNextMission;
	return mission;
}

BattleUnitMission *BattleUnitMission::acquireTU(BattleUnit &, unsigned int acquireTU)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::AcquireTU;
	mission->timeUnits = acquireTU;
	return mission;
}

BattleUnitMission *BattleUnitMission::changeStance(BattleUnit &, AgentType::BodyState state)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::ChangeBodyState;
	mission->bodyState = state;
	return mission;
}

BattleUnitMission *BattleUnitMission::throwItem(BattleUnit &u, sp<AEquipment> item,
                                                Vec3<int> target, float velocityXY, 
												float velocityZ)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::ThrowItem;
	mission->item = item;
	mission->targetLocation = target;
	mission->velocityXY = velocityXY;
	mission->velocityZ = velocityZ;
	return mission;
}

BattleUnitMission *BattleUnitMission::dropItem(BattleUnit &u, sp<AEquipment> item)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::DropItem;
	mission->item = item;
	return mission;
}

BattleUnitMission *BattleUnitMission::turn(BattleUnit &u, Vec2<int> target, bool free, 
	bool requireGoal)
{
	auto pos = u.tileObject->getOwningTile()->position;
	return turn(u, Vec3<int>{pos.x + target.x, pos.y + target.y, pos.z}, free, requireGoal);
}

BattleUnitMission *BattleUnitMission::turn(BattleUnit &u, Vec3<int> target, bool free, 
	bool requireGoal)
{
	return turn(u, u.tileObject->getOwningTile()->position, target, free, requireGoal);
}

BattleUnitMission *BattleUnitMission::turn(BattleUnit &u, Vec3<float> target, bool free, 
	bool requireGoal)
{
	return turn(u, u.position, target, free, requireGoal);
}

BattleUnitMission *BattleUnitMission::turn(BattleUnit &u, Vec3<float> from, Vec3<float> to, 
	bool free, bool requireGoal)
{
	static const std::list<Vec3<float>> angles = {
	    {0, -1, 0}, {1, -1, 0}, {1, 0, 0},  {1, 1, 0},
	    {0, 1, 0},  {-1, 1, 0}, {-1, 0, 0}, {-1, -1, 0},
	};

	auto *mission = new BattleUnitMission();
	mission->type = MissionType::Turn;
	mission->requireGoal = requireGoal;
	mission->free = free;
	float closestAngle = FLT_MAX;
	Vec3<int> closestVector = {0, 0, 0};
	Vec3<float> targetFacing = (Vec3<float>)(to - from);
	if (targetFacing.x == 0.0f && targetFacing.y == 0.0f)
	{
		closestVector = {u.facing.x, u.facing.y, 0};
	}
	else
	{
		targetFacing.z = 0;
		for (auto &a : angles)
		{
			float angle = glm::angle(glm::normalize(targetFacing), glm::normalize(a));
			if (angle < closestAngle && u.agent->isFacingAllowed(Vec2<int>{a.x, a.y}))
			{
				closestAngle = angle;
				closestVector = a;
			}
		}
	}
	mission->targetFacing = {closestVector.x, closestVector.y};
	return mission;
}

BattleUnitMission *BattleUnitMission::fall(BattleUnit &u)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::Fall;
	return mission;
}

BattleUnitMission *BattleUnitMission::reachGoal(BattleUnit &u)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::ReachGoal;
	mission->targetLocation = u.goalPosition;
	return mission;
}

BattleUnitMission *BattleUnitMission::teleport(BattleUnit &u, sp<AEquipment> item, Vec3<int> target)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::Teleport;
	mission->item = item;
	mission->targetLocation = target;
	return mission;
}

bool BattleUnitMission::spendAgentTUs(GameState &state, BattleUnit &u, int cost)
{
	if (u.spendTU(cost))
	{
		return true;
	}
	u.addMission(state, acquireTU(u, cost));
	return false;
}

bool BattleUnitMission::getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest)
{
	if (this->type != MissionType::GotoLocation)
		return false;
	return advanceAlongPath(state, u, dest);
}

bool BattleUnitMission::getNextFacing(GameState &state, BattleUnit &u, Vec2<int> &dest, int &ticks)
{
	if (this->type != MissionType::Turn)
		return false;
	return advanceFacing(state, u, dest, ticks);
}

bool BattleUnitMission::getNextBodyState(GameState &state, BattleUnit &u, AgentType::BodyState &dest, int &ticks)
{
	if (this->type != MissionType::ChangeBodyState)
		return false;
	return advanceBodyState(state, u, bodyState, dest, ticks);
}

void BattleUnitMission::update(GameState &state, BattleUnit &u, unsigned int ticks, bool finished)
{
	switch (this->type)
	{
		case MissionType::GotoLocation:
		{
			if (finished)
			{
				if (u.current_movement_state != AgentType::MovementState::None)
				{
					u.setMovementState(AgentType::MovementState::None);
				}
				if (allowRunningAway)
				{
					if (u.tileObject->getOwningTile()->getHasExit(u.isLarge()))
					{
						u.retreat(state);
					}
				}
			}
			else // = not finished
			{
				// Update movement speed if we're already moving
				if (u.current_movement_state != AgentType::MovementState::None)
				{
					makeAgentMove(u);
				}
			}
			return;
		}
		case MissionType::Snooze:
		{
			if (ticks >= this->timeToSnooze)
			{
				this->timeToSnooze = 0;
			}
			else
			{
				this->timeToSnooze -= ticks;
			}
			return;
		}
		case MissionType::ThrowItem:
			if (finished)
			{
				if (item)
				{
					auto battle = u.battle.lock();
					if (!battle)
						return;
					auto bi = mksp<BattleItem>();
					bi->battle = u.battle;
					bi->item = item;
					item = nullptr;
					bi->position = u.position + Vec3<float>{0.0, 0.0, 0.5f};
					bi->supported = false;
					battle->items.push_back(bi);
					u.tileObject->map.addObjectToMap(bi);
				}
			}
			return;
		case MissionType::ReachGoal:
			if (finished)
			{
				if (u.current_movement_state != AgentType::MovementState::None)
				{
					u.setMovementState(AgentType::MovementState::None);
				}
			}
			return;
		case MissionType::AcquireTU:
		case MissionType::RestartNextMission:
		case MissionType::ChangeBodyState:
		case MissionType::DropItem:
		case MissionType::Fall:
		case MissionType::Turn:
		case MissionType::Teleport:
			return;
		default:
			LogWarning("TODO: Implement");
			return;
	}
}

bool BattleUnitMission::isFinished(GameState &state, BattleUnit &u, bool callUpdateIfFinished)
{
	if (isFinishedInternal(state, u))
	{
		if (callUpdateIfFinished)
		{
			update(state, u, 0, true);
		}
		return true;
	}
	return false;
}

bool BattleUnitMission::isFinishedInternal(GameState &state, BattleUnit &u)
{
	switch (this->type)
	{
		case MissionType::AcquireTU:
			return u.agent->modified_stats.time_units >= (int)timeUnits;
		case MissionType::ReachGoal:
			return u.atGoal || u.falling;
		case MissionType::GotoLocation:
			return this->currentPlannedPath.empty() || u.isDead();
		case MissionType::Snooze:
			return this->timeToSnooze == 0;
		case MissionType::ChangeBodyState:
			return u.current_body_state == this->bodyState;
		case MissionType::ThrowItem:
			return !item;
		case MissionType::Turn:
			if (u.facing == targetFacing || u.isDead())
			{
				return true;
			}
			return false;
		case MissionType::Fall:
			return !u.falling;
		case MissionType::DropItem:
			if (item)
			{
				LogError("DropItem's item still present, was isFinished called before its start?");
				start(state, u);
			}
			return true;
		case MissionType::Teleport:
		case MissionType::RestartNextMission:
			return true;
		default:
			LogWarning("TODO: Implement");
			return false;
	}
}

void BattleUnitMission::start(GameState &state, BattleUnit &u)
{
	LogWarning("Unit mission \"%s\" starting", getName().cStr());

	switch (this->type)
	{
		
		case MissionType::Turn:
		case MissionType::ChangeBodyState:
		{
			return;
		}
		case MissionType::Teleport:
		{
			// The only mission that activates immediately upon start, and does not wait for TU
			if (item)
			{
				// Check if we can be there
				auto t = u.tileObject->map.getTile(targetLocation);
				bool canStand = t->getCanStand(u.isLarge());
				if (!t->getPassable(u.isLarge(), u.agent->type->bodyType->maxHeight) || t->getUnitIfPresent(true, true, false, nullptr, false, u.isLarge()) || (!u.canFly() && !canStand))
				{
					return;
				}
				// Teleportation requires full teleporter ammo
				if (item->ammo != item->type->max_ammo)
				{
					return;
				}
				if (item->type->type != AEquipmentType::Type::Teleporter)
				{
					LogError("Unit trying to teleport using non-teleporter item!?");
					return;
				}
				// Teleportation cost is 55% TUs
				int cost = u.agent->current_stats.time_units * 55 / 100;
				if (!u.spendTU(cost))
				{
					return;
				}
				// Process item
				item->ammo -= (int)glm::length(Vec3<float>(u.tileObject->getOwningTile()->position - targetLocation));
				item = nullptr;
				// Teleport unit
				u.missions.clear();
				u.setPosition(t->getRestingPosition(u.isLarge()));
				u.resetGoal();
				AgentType::BodyState targetBodyState = canStand ? AgentType::BodyState::Standing : targetBodyState = AgentType::BodyState::Flying;
				if (!u.agent->isBodyStateAllowed(targetBodyState))
					targetBodyState = AgentType::BodyState::Flying;
				if (!u.agent->isBodyStateAllowed(targetBodyState))
					targetBodyState = AgentType::BodyState::Kneeling;
				if (!u.agent->isBodyStateAllowed(targetBodyState))
					targetBodyState = AgentType::BodyState::Prone;
				if (!u.agent->isBodyStateAllowed(targetBodyState))
					LogError("Unit has no valid body state? WTF?");
				u.setBodyState(targetBodyState);
				u.setMovementState(AgentType::MovementState::None);
				u.falling = false;

				if (state.battle_common_sample_list->teleport)
				{
					fw().soundBackend->playSample(state.battle_common_sample_list->teleport,
						u.getPosition(), 0.25f);
				}
			}
			return;
		}
		case MissionType::ThrowItem:
			// Item disappeared from inventory
			if (item && item->ownerAgent != u.agent)
			{
				item = nullptr;
				return;
			}
			// Half way there - throw the item!
			if (u.current_body_state == AgentType::BodyState::Throwing)
			{
				if (state.battle_common_sample_list->throwSounds.size() > 0)
				{
					fw().soundBackend->playSample(
					    listRandomiser(state.rng, state.battle_common_sample_list->throwSounds),
					    u.getPosition(), 0.25f);
				}

				auto battle = u.battle.lock();
				if (!battle)
					return;
				auto bi = mksp<BattleItem>();
				bi->battle = u.battle;
				bi->item = item;
				item = nullptr;

				// Depart from middle of our body
				bi->position =
				    u.position + Vec3<float>{0.0, 0.0, (float)u.getCurrentHeight() / 2.0f / 40.0f};
				Vec3<float> targetVector = targetLocation - u.tileObject->getOwningTile()->position;
				
				// FIXME: Apply accuracy!
				bi->velocity = (glm::normalize(targetVector) * velocityXY +
				               Vec3<float>{0.0, 0.0, velocityZ}) * VELOCITY_SCALE_BATTLE;
				bi->supported = false;
				battle->items.push_back(bi);
				u.tileObject->map.addObjectToMap(bi);

				u.addMission(state, changeStance(u, AgentType::BodyState::Standing));
				return;
			}
			// Just starting the throw
			else if (item)
			{
				// Do we need to turn?
				auto m = turn(u, (Vec3<float>)targetLocation + Vec3<float>{0.5f, 0.5f, 0.0f});
				if (u.current_body_state != AgentType::BodyState::Standing ||
				    u.current_body_state != u.target_body_state || !m->isFinished(state, u))
				{
					// FIXME: actually read the option
					bool USER_OPTION_ALLOW_INSTANT_THROWS = false;
					if (!USER_OPTION_ALLOW_INSTANT_THROWS)
					{
						if (!m->isFinished(state, u))
						{
							u.addMission(state, m);
							return;
						}
						if (u.current_body_state != u.target_body_state)
						{
							// body state is changing - wait for it to change and then retry
							u.missions.emplace(++u.missions.begin(), BattleUnitMission::throwItem(
							                                             u, item, targetLocation, 
																		 velocityXY, velocityZ));
							u.missions.pop_front(); // pop this throw
							u.missions.front()->start(state, u); // re-start body change
							return;
						}
						else // body state is static but not standing
						{
							u.addMission(state, BattleUnitMission::changeStance(u, AgentType::BodyState::Standing));
							return;
						}
					}
					u.setBodyState(AgentType::BodyState::Standing);
					u.facing = m->targetFacing;
					u.goalFacing = m->targetFacing;
				}
				// Calculate cost
				// I *think* this is correct? 18 TUs at 100 time units
				int cost = u.agent->current_stats.time_units * 18 / 100;
				if (!spendAgentTUs(state, u, cost))
				{
					LogWarning("Unit mission \"%s\" could not start: unsufficient TUs",
					           getName().cStr());
					return;
				}
				// Remove item
				item->ownerAgent->removeEquipment(item);
				// Start throw animation
				u.addMission(state, BattleUnitMission::changeStance(u, AgentType::BodyState::Throwing));
				return;
			}
			// Throw finished - nothing else to do
			else
			{
				// Nothing else to do
			}
			return;
		case MissionType::DropItem:
		{
			if (item)
			{// Remove item
				item->ownerAgent->removeEquipment(item);
				// Drop item
				auto battle = u.battle.lock();
				if (!battle)
					return;
				auto bi = mksp<BattleItem>();
				bi->battle = u.battle;
				bi->item = item;
				item = nullptr;
				bi->position = u.position + Vec3<float>{0.0, 0.0, 0.5f};
				bi->supported = false;
				battle->items.push_back(bi);
				u.tileObject->map.addObjectToMap(bi);
			}
			return;
		}
		case MissionType::GotoLocation:
			// If we have already tried to use this mission, see if path is still valid
			if (currentPlannedPath.size() > 0)
			{
				auto t = u.tileObject->getOwningTile();
				auto pos = *currentPlannedPath.begin();
				// If we're not far enough and can enter first tile in path
				if (t->position == pos)
				{
					// No need to add our position in
				}
				else if (std::abs(t->position.x - pos.x) <= 1 &&
				         std::abs(t->position.y - pos.y) <= 1 &&
				         std::abs(t->position.z - pos.z) <= 1 &&
				         BattleUnitTileHelper{t->map, u}.canEnterTile(t, t->map.getTile(pos), true))
				{
					// Add our position in
					currentPlannedPath.push_front(t->position);
				}
				else
				{
					// Path is invalid
					currentPlannedPath.clear();
				}
			}
			if (currentPlannedPath.size() == 0)
			{
				this->setPathTo(u, this->targetLocation);
			}
			// Starting GotoLocation mission never costs TU's, instead, getting next path node does
			return;
		case MissionType::ReachGoal:
			// Reaching goal means we already requested this node and paid the price, so no TU cost
			makeAgentMove(u);
			return;
		case MissionType::Fall:
			u.setMovementState(AgentType::MovementState::None);
			u.falling = true;
			return;
		case MissionType::AcquireTU:
		case MissionType::RestartNextMission:
		case MissionType::Snooze:
			return;
		default:
			LogWarning("TODO: Implement");
			return;
	}
}

void BattleUnitMission::setPathTo(BattleUnit &u, Vec3<int> target, int maxIterations)
{
	auto unitTile = u.tileObject;
	if (unitTile)
	{
		auto &map = u.tileObject->map;
		auto to = map.getTile(target);
		// Check if target tile is valid
		while (true)
		{
			if (!u.canMove() || !to->getPassable(u.isLarge(), u.agent->type->bodyType->maxHeight))
			{
				LogInfo("Cannot move to %d %d %d, impassable", target.x, target.y, target.z);
				return;
			}
			if (u.canFly() || to->getCanStand(u.isLarge()))
			{
				break;
			}
			target.z--;
			if (target.z == -1)
			{
				LogError("Solid ground missing on level 0? Reached %d %d %d", target.x, target.y,
				         target.z);
				return;
			}
			to = map.getTile(target);
		}

		auto path = map.findShortestPath(u.goalPosition, target, maxIterations,
		                                 BattleUnitTileHelper{map, u}, demandGiveWay);

		// Always start with the current position
		this->currentPlannedPath.push_back(u.goalPosition);
		for (auto &p : path)
		{
			this->currentPlannedPath.push_back(p);
		}
		targetLocation = currentPlannedPath.back();
	}
	else
	{
		LogError("Mission %s: Unit without tileobject attempted pathfinding!",
		         this->getName().cStr());
	}
}

bool BattleUnitMission::advanceAlongPath(GameState &state, BattleUnit &u, Vec3<float> &dest)
{
	// FIXME: do not go into other units

	if (u.isUnconscious() || u.isDead())
		return false;
	if (currentPlannedPath.empty())
		return false;
	auto poppedPos = currentPlannedPath.front();
	currentPlannedPath.pop_front();
	if (currentPlannedPath.empty())
		return false;
	auto pos = currentPlannedPath.front();

	// See if we can actually go there
	auto tFrom = u.tileObject->getOwningTile();
	auto tTo = tFrom->map.getTile(pos);
	float cost = 0;
	bool closedDoorInTheWay = false;
	if (tFrom->position != pos &&
	    (std::abs(tFrom->position.x - pos.x) > 1 || std::abs(tFrom->position.y - pos.y) > 1 ||
	     std::abs(tFrom->position.z - pos.z) > 1 ||
	     !BattleUnitTileHelper{tFrom->map, u}.canEnterTile(tFrom, tTo, cost, closedDoorInTheWay,
	                                                       true)))
	{
		// Next tile became impassable, pick a new path
		currentPlannedPath.clear();
		u.addMission(state, MissionType::RestartNextMission);
		return false;
	}

	// See if we can make a shortcut
	// When ordering move to a unit already on the move, we can have a situation
	// where going directly to 2nd step in the path is faster than going to the first
	// In this case, we should skip unnesecary steps
	auto it = ++currentPlannedPath.begin();
	// Start with position after next
	// If next position has a node and we can go directly to that node
	// Then update current position and iterator
	float newCost = 0;
	bool newDoorInWay = false;
	while (it != currentPlannedPath.end() &&
	       (tFrom->position == *it ||
	        (allowSkipNodes && std::abs(tFrom->position.x - it->x) <= 1 &&
	         std::abs(tFrom->position.y - it->y) <= 1 && std::abs(tFrom->position.z - it->z) <= 1 &&
	         BattleUnitTileHelper{tFrom->map, u}.canEnterTile(tFrom, tFrom->map.getTile(*it),
	                                                          newCost, newDoorInWay))))
	{
		currentPlannedPath.pop_front();
		poppedPos = pos;
		pos = currentPlannedPath.front();
		tTo = tFrom->map.getTile(pos);
		cost = newCost;
		closedDoorInTheWay = newDoorInWay;
		it = ++currentPlannedPath.begin();
	}

	// Do we need to change stance?
	switch (u.movement_mode)
	{
		// If want to move prone but not prone - go prone
		case BattleUnit::MovementMode::Prone:
			if (u.current_body_state == AgentType::BodyState::Prone)
			{
				// Ensure we can go prone at target tile
				if (u.canProne(pos, u.facing))
				{
					break;
				}
			}
			else if (u.canProne(u.position, u.facing) && u.canProne(pos, u.facing))
			{
				u.addMission(state, changeStance(u, AgentType::BodyState::Prone));
				return false;
			}
		// Intentional fall-though, if we want to go prone but cannot go prone,
		// we should act as if we're told to walk/run
		// If want to move standing up but not standing/flying - go standing/flying where
		// appropriate
		case BattleUnit::MovementMode::Walking:
		case BattleUnit::MovementMode::Running:
			auto t = u.tileObject->getOwningTile();
			if (u.current_body_state != AgentType::BodyState::Standing &&
			    u.current_body_state != AgentType::BodyState::Flying)
			{
				if (t->getCanStand(u.isLarge()) && t->map.getTile(pos)->getCanStand(u.isLarge()))
				{
					u.addMission(state, changeStance(u, AgentType::BodyState::Standing));
					return false;
				}
				else
				{
					u.addMission(state, changeStance(u, AgentType::BodyState::Flying));
					return false;
				}
			}
			// Stop flying if we no longer require it
			if (u.current_body_state == AgentType::BodyState::Flying &&
			    t->getCanStand(u.isLarge()) && t->map.getTile(pos)->getCanStand(u.isLarge()))
			{
				u.addMission(state, changeStance(u, AgentType::BodyState::Standing));
				return false;
			}
			// Start flying if we require it
			if (u.current_body_state != AgentType::BodyState::Flying && u.canFly() &&
			    !(t->getCanStand(u.isLarge()) && t->map.getTile(pos)->getCanStand(u.isLarge())))
			{
				u.addMission(state, changeStance(u, AgentType::BodyState::Flying));
				return false;
			}
			break;
	}

	// Is there a unit in the target tile other than us?
	auto blockingUnitTileObject = tTo->getUnitIfPresent(true, true, false, u.tileObject);
	if (blockingUnitTileObject)
	{
		auto blockingUnit = blockingUnitTileObject->getUnit();
		u.current_movement_state = AgentType::MovementState::None;
		// If unit is still patient enough, and we can ask to give way
		if (giveWayAttemptsRemaining-- > 0 && blockingUnit->owner == u.owner
		    // and we're not trying to stay there
		    && currentPlannedPath.size() > 1
		    // and unit we're asking is not big
		    // (coding giving way for large units is too much of a fuss,
		    // and ain't going to be used a lot anyway, just path around them)
		    && !blockingUnit->isLarge())
		{
			// Ask unit to give way to us if not asked already
			if (blockingUnit->giveWayRequest.size() == 0
			    // and it's not busy
			    && !blockingUnit->isBusy())
			{
				// If unit is prone and we're trying to go into it's legs
				if (blockingUnit->current_body_state == AgentType::BodyState::Prone &&
				    blockingUnit->tileObject->getOwningTile()->position != pos)
				{
					blockingUnit->giveWayRequest.emplace_back(0, 0);
				}
				// If unit is not prone or we're trying to go into it's body
				else
				{
					static const std::map<Vec2<int>, int> facing_dir_map = {
					    {{0, -1}, 0}, {{1, -1}, 1}, {{1, 0}, 2},  {{1, 1}, 3},
					    {{0, 1}, 4},  {{-1, 1}, 5}, {{-1, 0}, 6}, {{-1, -1}, 7}};
					static const std::map<int, Vec2<int>> dir_facing_map = {
					    {0, {0, -1}}, {1, {1, -1}}, {2, {1, 0}},  {3, {1, 1}},
					    {4, {0, 1}},  {5, {-1, 1}}, {6, {-1, 0}}, {7, {-1, -1}}};

					// Start with unit's facing, and go to the sides, adding facings
					// if they're not in our path and not our current position.
					// Next facings: [0] is clockwise, [1] is counter-clockwise from current
					std::vector<int> nextFacings = {facing_dir_map.at(blockingUnit->facing),
					                                facing_dir_map.at(blockingUnit->facing)};
					for (int i = 0; i <= 4; i++)
					{
						int limit = i == 0 || i == 4 ? 0 : 1;
						for (int j = 0; j <= limit; j++)
						{
							auto nextFacing = dir_facing_map.at(nextFacings[j]);
							Vec3<int> nextPos = {blockingUnit->position.x + nextFacing.x,
							                     blockingUnit->position.y + nextFacing.y,
							                     blockingUnit->position.z};
							if (nextPos == (Vec3<int>)u.position ||
							    std::find(currentPlannedPath.begin(), currentPlannedPath.end(),
							              nextPos) != currentPlannedPath.end())
							{
								continue;
							}
							blockingUnit->giveWayRequest.push_back(nextFacing);
						}
						nextFacings[0] = nextFacings[0] == 7 ? 0 : nextFacings[0] + 1;
						nextFacings[1] = nextFacings[1] == 0 ? 7 : nextFacings[1] - 1;
					}
				}
			}

			// Snooze for a moment and try again
			u.addMission(state, snooze(u, 15));
			return false;
		}
		else
		{
			// Unit's patience has ran out
			currentPlannedPath.clear();
			u.addMission(state, MissionType::RestartNextMission);
			return false;
		}
	}

	// Is there a door?
	if (closedDoorInTheWay)
	{
		// Snooze for a moment and try again
		u.addMission(state, snooze(u, 5));
		return false;
	}

	// Running decreases cost
	// FIXME: Handle strafing and going backwards influencing TU spent
	if (u.agent->canRun() && u.movement_mode == BattleUnit::MovementMode::Running)
	{
		cost /= 2;
	}
	if (u.current_body_state == AgentType::BodyState::Prone)
	{
		cost *= 3;
		cost /= 2;
	}
	int intCost = (int)cost;
	if (costPaidUpFront > 0)
	{
		if (costPaidUpFront >= cost)
		{
			costPaidUpFront -= intCost;
			intCost = 0;
		}
		else
		{
			intCost -= costPaidUpFront;
			costPaidUpFront = 0;
		}
	}
	// See if we can afford doing this move
	if (!spendAgentTUs(state, u, intCost))
	{
		currentPlannedPath.push_front(poppedPos);
		return false;
	}

	// Do we need to turn?
	auto m = turn(u, pos, true);
	if (!m->isFinished(state, u, false))
	{
		u.addMission(state, m);
		currentPlannedPath.push_front(poppedPos);
		costPaidUpFront += intCost;
		return false;
	}

	// FIXME: Deplete stamina according to encumbrance if running
	makeAgentMove(u);

	dest = u.tileObject->map.getTile(pos)->getRestingPosition(u.isLarge());
	return true;
}

bool BattleUnitMission::advanceFacing(GameState &state, BattleUnit &u, Vec2<int> &dest, int &ticks)
{
	static const std::map<Vec2<int>, int> facing_dir_map = {
		{ { 0, -1 }, 0 },{ { 1, -1 }, 1 },{ { 1, 0 }, 2 },{ { 1, 1 }, 3 },
		{ { 0, 1 }, 4 },{ { -1, 1 }, 5 },{ { -1, 0 }, 6 },{ { -1, -1 }, 7 } };
	static const std::map<int, Vec2<int>> dir_facing_map = {
		{ 0,{ 0, -1 } },{ 1,{ 1, -1 } },{ 2,{ 1, 0 } },{ 3,{ 1, 1 } },
		{ 4,{ 0, 1 } },{ 5,{ -1, 1 } },{ 6,{ -1, 0 } },{ 7,{ -1, -1 } } };

	if (targetFacing == u.facing)
		return false;

	int curFacing = facing_dir_map.at(u.facing);
	int tarFacing = facing_dir_map.at(targetFacing);
	if (curFacing == tarFacing)
		return false;

	// Already facing properly?
	if (isFinished(state, u))
	{
		return false;
	}
	// Go to goal first
	if (!u.atGoal && requireGoal)
	{
		u.addMission(state, MissionType::ReachGoal);
		return false;
	}
	// If we need to turn, do we need to change stance first?
	if (u.current_body_state == AgentType::BodyState::Prone)
	{
		u.addMission(state, changeStance(u, AgentType::BodyState::Kneeling));
		return false;
	}

	int clockwiseDistance = tarFacing - curFacing;
	if (clockwiseDistance < 0)
	{
		clockwiseDistance += 8;
	}
	int counterClockwiseDistance = curFacing - tarFacing;
	if (counterClockwiseDistance < 0)
	{
		counterClockwiseDistance += 8;
	}
	do
	{
		if (clockwiseDistance < counterClockwiseDistance)
		{
			curFacing = curFacing == 7 ? 0 : (curFacing + 1);
		}
		else
		{
			curFacing = curFacing == 0 ? 7 : (curFacing - 1);
		}
		dest = dir_facing_map.at(curFacing);

	} while (!u.agent->isFacingAllowed(dest));

	// Calculate and spend cost
	int cost = free ? 0 : 1;
	if (!spendAgentTUs(state, u, cost))
	{
		LogWarning("Turning could not start: unsufficient TUs",
			getName().cStr());
		return false;
	}
	
	// Cost spent, start turning
	ticks = TICKS_PER_FRAME_UNIT;
	return true;
}

bool BattleUnitMission::advanceBodyState(GameState &state, BattleUnit &u, AgentType::BodyState targetState, AgentType::BodyState &dest, int &ticks)
{
	if (targetState == u.target_body_state)
	{
		return false;
	}
	if (u.current_body_state != u.target_body_state)
	{
		LogError("Requesting to change body state during another body state change?");
		u.setBodyState(u.target_body_state);
	}
	bool instant = false;
	if (u.agent->getAnimationPack()->getFrameCountBody(
		u.getDisplayedItem(), u.target_body_state, targetState, u.current_hand_state,
		u.current_movement_state, u.facing) == 0)
	{
		if (u.agent->getAnimationPack()->getFrameCountBody(
			u.getDisplayedItem(), u.target_body_state, targetState, u.current_hand_state,
			AgentType::MovementState::None, u.facing) != 0)
		{
			u.setMovementState(AgentType::MovementState::None);
		}
		else
		{
			instant = true;
		}
	}
	// Transition for stance changes
	// If trying to fly stand up first
	if (targetState == AgentType::BodyState::Flying &&
		u.current_body_state != AgentType::BodyState::Standing)
	{
		return advanceBodyState(state, u, AgentType::BodyState::Standing, dest, ticks);
	}
	// If trying to stop flying stand up first
	if (targetState != AgentType::BodyState::Standing &&
		u.current_body_state == AgentType::BodyState::Flying &&
		u.agent->isBodyStateAllowed(AgentType::BodyState::Standing))
	{
		return advanceBodyState(state, u, AgentType::BodyState::Standing, dest, ticks);
	}
	// If trying to stand up from prone go kneel first
	if (targetState != AgentType::BodyState::Kneeling &&
		u.current_body_state == AgentType::BodyState::Prone &&
		u.agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
	{
		return advanceBodyState(state, u, AgentType::BodyState::Kneeling, dest, ticks);
	}
	// If trying to go prone from not kneeling then kneel first
	if (targetState == AgentType::BodyState::Prone &&
		u.current_body_state != AgentType::BodyState::Kneeling &&
		u.agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
	{
		return advanceBodyState(state, u, AgentType::BodyState::Kneeling, dest, ticks);
	}
	// Calculate and spend cost
	int cost = getBodyStateChangeCost(u.target_body_state, targetState);
	
	if (!spendAgentTUs(state, u, cost))
	{
		LogWarning("Body state changecould not start: unsufficient TUs",
			getName().cStr());
		return false;
	}
	// Change state actually
	dest = targetState;
	ticks = instant ? 0 : u.agent->getAnimationPack()->getFrameCountBody(
		u.getDisplayedItem(), u.current_body_state,
		targetState, u.current_hand_state,
		u.current_movement_state, u.facing) *
		TICKS_PER_FRAME_UNIT;
	return true;
}

int BattleUnitMission::getBodyStateChangeCost(AgentType::BodyState from, AgentType::BodyState to)
{
	// If not within these conditions, it costs nothing!
	switch (to)
	{
	case AgentType::BodyState::Flying:
	case AgentType::BodyState::Standing:
		switch (from)
		{
			case AgentType::BodyState::Kneeling:
				return 8;
			case AgentType::BodyState::Prone:
				return 16;
		}
		break;
	case AgentType::BodyState::Kneeling:
		switch (from)
		{
			case AgentType::BodyState::Prone:
			case AgentType::BodyState::Standing:
			case AgentType::BodyState::Flying:
				return 8;
		}
		break;
	case AgentType::BodyState::Prone:
		switch (from)
		{
			case AgentType::BodyState::Kneeling:
			case AgentType::BodyState::Standing:
			case AgentType::BodyState::Flying:
				return 8;
		}
		break;
	}
	return 0;
}

void BattleUnitMission::makeAgentMove(BattleUnit &u)
{
	// FIXME: Account for different movement ways (strafing, backwards etc.)
	if (u.movement_mode == BattleUnit::MovementMode::Running &&
	    u.current_body_state != AgentType::BodyState::Prone)
	{
		u.setMovementState(AgentType::MovementState::Running);
	}
	else if (u.current_body_state != AgentType::BodyState::Kneeling &&
	         u.current_body_state != AgentType::BodyState::Throwing)
	{
		u.setMovementState(AgentType::MovementState::Normal);
	}
	else
	{
		LogError("Trying to move while kneeling or throwing, wtf?");
	}
}

UString BattleUnitMission::getName()
{
	UString name = "UNKNOWN";
	switch (this->type)
	{
		case MissionType::AcquireTU:
			name = "AcquireTUs " + UString::format(" %u", this->timeUnits);
			break;
		case MissionType::GotoLocation:
			name =
			    "GotoLocation " + UString::format(" {%d,%d,%d}", this->targetLocation.x,
			                                      this->targetLocation.y, this->targetLocation.z);
			break;
		case MissionType::Teleport:
			name =
				"Teleport to " + UString::format(" {%d,%d,%d}", this->targetLocation.x,
					this->targetLocation.y, this->targetLocation.z);
			break;
		case MissionType::RestartNextMission:
			name = "Restart next mission";
			break;
		case MissionType::Snooze:
			name = "Snooze " + UString::format(" for %u ticks", this->timeToSnooze);
			break;
		case MissionType::ChangeBodyState:
			name = "ChangeBodyState " + UString::format("%d", (int)this->bodyState);
			break;
		case MissionType::ThrowItem:
			name = "ThrowItem " +
			       UString::format(
			           "%s at %d,%d,%d", item ? this->item->type->name.cStr() : "(item is gone)",
			           this->targetLocation.x, this->targetLocation.y, this->targetLocation.z);
			break;
		case MissionType::DropItem:
			name = "DropItem " +
			       UString::format("%s", item ? this->item->type->name.cStr() : "(item is gone)");
			break;
		case MissionType::ReachGoal:
			name = "ReachGoal";
			break;
		case MissionType::Turn:
			name =
			    "Turn " + UString::format(" {%d,%d}", this->targetFacing.x, this->targetFacing.y);
			break;
		case MissionType::Fall:
			name = "Fall!...FALL!!";
			break;
	}
	return name;
}

} // namespace OpenApoc
