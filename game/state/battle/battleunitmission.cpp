#include "game/state/battle/battleunitmission.h"
#include "framework/framework.h"
#include "framework/sound.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gameevent.h"
#include "game/state/gamestate.h"
#include "game/state/rules/aequipmenttype.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/shared/aequipment.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{
namespace
{
static const std::map<Vec2<int>, int> facing_dir_map = {{{0, -1}, 0}, {{1, -1}, 1}, {{1, 0}, 2},
                                                        {{1, 1}, 3},  {{0, 1}, 4},  {{-1, 1}, 5},
                                                        {{-1, 0}, 6}, {{-1, -1}, 7}};
static const std::map<int, Vec2<int>> dir_facing_map = {{0, {0, -1}}, {1, {1, -1}}, {2, {1, 0}},
                                                        {3, {1, 1}},  {4, {0, 1}},  {5, {-1, 1}},
                                                        {6, {-1, 0}}, {7, {-1, -1}}};
static const std::list<Vec3<float>> angles = {
    {0, -1, 0}, {1, -1, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {-1, 1, 0}, {-1, 0, 0}, {-1, -1, 0},
};
} // namespace

BattleUnitTileHelper::BattleUnitTileHelper(TileMap &map, BattleUnitType type, bool allowJumping)
    : BattleUnitTileHelper(
          map, (type == BattleUnitType::LargeFlyer || type == BattleUnitType::LargeWalker),
          (type == BattleUnitType::LargeFlyer || type == BattleUnitType::SmallFlyer), allowJumping,
          (type == BattleUnitType::LargeFlyer || type == BattleUnitType::LargeWalker) ? 70 : 32,
          nullptr)
{
}

BattleUnitTileHelper::BattleUnitTileHelper(TileMap &map, bool large, bool flying, bool allowJumping,
                                           int maxHeight, sp<TileObjectBattleUnit> tileObject)
    : map(map), large(large), flying(flying), maxHeight(maxHeight), tileObject(tileObject),
      canJump(allowJumping && !flying)
{
}

BattleUnitTileHelper::BattleUnitTileHelper(TileMap &map, BattleUnit &u)
    : BattleUnitTileHelper(map, u.isLarge(), u.canFly(),
                           u.agent->isBodyStateAllowed(BodyState::Jumping),
                           u.agent->type->bodyType->maxHeight, u.tileObject)
{
}

// Returns distance between two positions, in TUs required to move optimally between them (walking)
float BattleUnitTileHelper::getDistanceStatic(Vec3<float> from, Vec3<float> to)
{
	auto diff = to - from;
	auto xDiff = std::abs(diff.x);
	auto yDiff = std::abs(diff.y);
	auto zDiff = std::abs(diff.z);
	return (std::max(std::max(xDiff, yDiff), zDiff) + xDiff + yDiff + zDiff) * 2.0f;
}

// Returns distance between two positions, in TUs required to move optimally between them (walking)
float BattleUnitTileHelper::getDistanceStatic(Vec3<float> from, Vec3<float> toStart,
                                              Vec3<float> toEnd)
{
	auto diffStart = toStart - from;
	auto diffEnd = toEnd - from - Vec3<float>{1.0f, 1.0f, 1.0f};
	auto xDiff = from.x >= toStart.x && from.x < toEnd.x
	                 ? 0.0f
	                 : std::min(std::abs(diffStart.x), std::abs(diffEnd.x));
	auto yDiff = from.y >= toStart.y && from.y < toEnd.y
	                 ? 0.0f
	                 : std::min(std::abs(diffStart.y), std::abs(diffEnd.y));
	auto zDiff = from.z >= toStart.z && from.z < toEnd.z
	                 ? 0.0f
	                 : std::min(std::abs(diffStart.z), std::abs(diffEnd.z));
	return (std::max(std::max(xDiff, yDiff), zDiff) + xDiff + yDiff + zDiff) * 2.0f;
}

float BattleUnitTileHelper::getDistance(Vec3<float> from, Vec3<float> toStart,
                                        Vec3<float> toEnd) const
{
	return getDistanceStatic(from, toStart, toEnd);
}

float BattleUnitTileHelper::getDistance(Vec3<float> from, Vec3<float> to) const
{
	return getDistanceStatic(from, to);
}

BattleUnitType BattleUnitTileHelper::getType() const
{
	if (large)
	{
		if (flying)
		{
			return BattleUnitType::LargeFlyer;
		}
		else
		{
			return BattleUnitType::LargeWalker;
		}
	}
	else
	{
		if (flying)
		{
			return BattleUnitType::SmallFlyer;
		}
		else
		{
			return BattleUnitType::SmallWalker;
		}
	}
}

bool BattleUnitTileHelper::canEnterTile(Tile *from, Tile *to, bool ignoreStaticUnits,
                                        bool ignoreMovingUnits, bool ignoreAllUnits) const
{
	float nothing;
	bool none1;
	bool none2;
	return canEnterTile(from, to, false, none1, nothing, none2, ignoreStaticUnits,
	                    ignoreMovingUnits, ignoreAllUnits);
}

bool BattleUnitTileHelper::canEnterTile(Tile *from, Tile *to, bool allowJumping, bool &jumped,
                                        float &cost, bool &doorInTheWay, bool ignoreStaticUnits,
                                        bool ignoreMovingUnits, bool ignoreAllUnits) const
{
	int costInt = 0;
	doorInTheWay = false;

	// Error checks
	if (!to)
	{
		LogError("No 'to' position supplied");
		return false;
	}
	Vec3<int> toPos = to->position;
	if (!from)
	{
		// Only check if target tile can be occupied
		return to->getPassable(large, maxHeight);
	}
	Vec3<int> fromPos = from->position;
	if (fromPos == toPos)
	{
		LogError("FromPos == ToPos %s", toPos);
		return false;
	}
	if (!map.tileIsValid(fromPos))
	{
		LogError("FromPos %s is not on the map", fromPos);
		return false;
	}
	if (!map.tileIsValid(toPos))
	{
		LogError("ToPos %s is not on the map", toPos);
		return false;
	}

	// Large units can't jump, also can't jump to different z
	allowJumping = allowJumping && canJump && !large && toPos.z == fromPos.z;

	// If tiles not adjacent -> see if we're checking a jump
	if (std::abs(toPos.x - fromPos.x) > 1 || std::abs(toPos.y - fromPos.y) > 1 ||
	    std::abs(toPos.z - fromPos.z) > 1)
	{
		if (allowJumping)
		{
			// Jump must be straight two tiles on either axis or both
			if ((std::abs(toPos.x - fromPos.x) != 2 && std::abs(toPos.x - fromPos.x) != 0) ||
			    (std::abs(toPos.y - fromPos.y) != 2 && std::abs(toPos.y - fromPos.y) != 0))
			{
				return false;
			}
			auto middle = map.getTile(fromPos + (toPos - fromPos) / 2);
			if (canEnterTile(from, middle, true, jumped, cost, doorInTheWay, ignoreStaticUnits,
			                 ignoreMovingUnits, ignoreAllUnits) &&
			    jumped &&
			    canEnterTile(middle, to, false, jumped, cost, doorInTheWay, ignoreStaticUnits,
			                 ignoreMovingUnits, ignoreAllUnits))
			{
				return true;
			}
		}
		return false;
	}

	// Tiles used by big units
	Tile *fromX1 = nullptr;  // from (x-1, y, z)
	Vec3<int> fromX1Pos(0);  // fromPos (x-1, y, z)
	Tile *fromY1 = nullptr;  // from (x, y-1, z)
	Vec3<int> fromY1Pos(0);  // fromPos (x, y-1, z)
	Tile *fromXY1 = nullptr; // from (x-1, y-1, z)
	Vec3<int> fromXY1Pos(0); // fromPos (x-1, y-1, z)
	Tile *toX1 = nullptr;    // to (x-1, y, z)
	Vec3<int> toX1Pos(0);    // toPos (x-1, y, z)
	Tile *toY1 = nullptr;    // to (x, y-1, z)
	Vec3<int> toY1Pos(0);    // toPos (x, y-1, z)
	Tile *toXY1 = nullptr;   // to (x-1, y-1, z)
	Vec3<int> toXY1Pos(0);   // toPos (x-1, y-1, z)
	Tile *toZ1 = nullptr;    // to (x, y, z-1)
	Vec3<int> toZ1Pos(0);    // toPos (x, y, z-1)
	Tile *toXZ1 = nullptr;   // to (x-1, y, z-1)
	Vec3<int> toXZ1Pos(0);   // toPos (x-1, y, z-1)
	Tile *toYZ1 = nullptr;   // to (x, y-1, z-1)
	Vec3<int> toYZ1Pos(0);   // toPos (x, y-1, z-1)
	Tile *toXYZ1 = nullptr;  // to (x-1, y-1, z-1)
	Vec3<int> toXYZ1Pos(0);  // toPos (x-1, y-1, z-1)

	// STEP 01: Check if "to" is passable
	// We could just use Tile::getPassable, however, we need to make some extra calculations
	// Like store the movement cost, look for units and so on
	// Therefore, I think it's better to do it all here
	// Plus, we will re-use some of the tiles we got from the map later down the line

	// STEP 01: Check if "to" is passable [large]
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
		if (!ignoreAllUnits)
		{
			// Actually, in case we are set to ignore static units, we still have to check for large
			// static units,
			// because they don't know how to give way, and therefore are considered permanent
			// obstacles
			if (to->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject, ignoreStaticUnits))
				return false;
			if (toX1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
			                           ignoreStaticUnits))
				return false;
			if (toY1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
			                           ignoreStaticUnits))
				return false;
			if (toXY1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
			                            ignoreStaticUnits))
				return false;
			if (toZ1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
			                           ignoreStaticUnits))
				return false;
			if (toXZ1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
			                            ignoreStaticUnits))
				return false;
			if (toYZ1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
			                            ignoreStaticUnits))
				return false;
			if (toXYZ1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
			                             ignoreStaticUnits))
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
	// STEP 01: Check if "to" is passable [small]
	else
	{
		// Check that no static unit occupies this tile
		// Actually, in case we are set to ignore static units, we still have to check for large
		// static units,
		// because they don't know how to give way, and therefore are considered permanent
		// obstacles
		if (!ignoreAllUnits &&
		    to->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject, ignoreStaticUnits))
			return false;
		// Movement cost into the tiles
		costInt = to->movementCostIn;
	}
	// STEP 01: [Failure condition]
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
		{
			if (allowJumping)
			{
				jumped = true;
			}
			else
			{
				return false;
			}
		}
	}

	// STEP 03: Falling and going down the lift
	// STEP 03.01: Check if falling, and exit immediately
	// If falling, then we can only go down, and for free!
	// However, vanilla disallowed that, and instead never let soldiers pick this option
	// So, unless we allow going into non-solid ground for non-flyers,
	// this will never happen (except when giving orders to a falling unit)
	if (!flying && !jumped)
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
				fromHasLift =
				    from->hasLift || fromX1->hasLift || fromY1->hasLift || fromXY1->hasLift;
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
	if (!to->getHeadFits(large, maxHeight))
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
				/*
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
				*/
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
				if (!ignoreAllUnits &&
				    (bottomLeftZ0->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                    ignoreStaticUnits) ||
				     rightTopZ0->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                  ignoreStaticUnits) ||
				     bottomLeftZ1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                    ignoreStaticUnits) ||
				     rightTopZ1->getUnitIfPresent(true, true, true, tileObject, ignoreStaticUnits)))
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
				/*
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
				*/
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
				if (!ignoreAllUnits &&
				    (topLeftZ0->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                 ignoreStaticUnits) ||
				     bottomRightZ0->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                     ignoreStaticUnits) ||
				     topLeftZ1->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                 ignoreStaticUnits) ||
				     bottomRightZ1->getUnitIfPresent(true, true, true, tileObject,
				                                     ignoreStaticUnits)))
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
			/*
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
			*/
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
				if (!ignoreAllUnits &&
				    (bottomLeft->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                  ignoreStaticUnits) ||
				     topRight->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                ignoreStaticUnits)))
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
				if (!ignoreAllUnits &&
				    (topLeft->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                               ignoreStaticUnits) ||
				     bottomRight->getUnitIfPresent(true, true, ignoreMovingUnits, tileObject,
				                                   ignoreStaticUnits)))
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
	if (costInt == 255 || (doorInTheWay && jumped))
	{
		return false;
	}

	// STEP 07: Calculate movement cost modifier
	// If jumping then cost is preset (2x normal movement cost)
	if (!allowJumping && jumped)
	{
		cost = 2.0f * (float)STANDART_MOVE_TU_COST *
		       ((toPos.x != fromPos.x && toPos.y != fromPos.y) ? 3.0f : 2.0f);
	}
	else
	// It costs 1x to move to adjacent tile, 1.5x to move diagonally,
	// 2x to move diagonally to another layer.
	// Also, it costs 0x to fall, but we have checked for that above
	{
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
	}

	return true;
}

BattleUnitMission *BattleUnitMission::gotoLocation(BattleUnit &u, Vec3<int> target, int facingDelta,
                                                   bool demandGiveWay, bool allowSkipNodes,
                                                   int giveWayAttempts, bool allowRunningAway)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::GotoLocation;
	mission->targetLocation = target;
	mission->giveWayAttemptsRemaining = giveWayAttempts;
	mission->allowSkipNodes = allowSkipNodes;
	mission->demandGiveWay = demandGiveWay;
	mission->allowRunningAway = allowRunningAway;
	mission->targetBodyState = u.target_body_state;
	mission->targetFacing = u.goalFacing;
	if (facingDelta > 4)
	{
		facingDelta -= 8;
	}
	mission->facingDelta = facingDelta;

	return mission;
}

BattleUnitMission *BattleUnitMission::snooze(BattleUnit &, unsigned int snoozeTicks)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::Snooze;
	mission->timeToSnooze = snoozeTicks;
	return mission;
}

BattleUnitMission *BattleUnitMission::restartNextMission(BattleUnit &)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::RestartNextMission;
	return mission;
}

BattleUnitMission *BattleUnitMission::acquireTU(BattleUnit &, bool allowContinue)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::AcquireTU;
	mission->allowContinue = allowContinue;
	return mission;
}

BattleUnitMission *BattleUnitMission::changeStance(BattleUnit &, BodyState state)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::ChangeBodyState;
	mission->targetBodyState = state;
	return mission;
}

BattleUnitMission *BattleUnitMission::throwItem(BattleUnit &u, sp<AEquipment> item,
                                                Vec3<int> target)
{
	float velXY = 0.0f;
	float velZ = 0.0f;
	if (!item->getVelocityForThrow(u, target, velXY, velZ))
	{
		return nullptr;
	}

	auto *mission = new BattleUnitMission();
	mission->type = Type::ThrowItem;
	mission->item = item;
	mission->targetLocation = target;
	mission->targetFacing = getFacing(u, target);
	mission->targetBodyState = BodyState::Throwing;
	mission->freeTurn = true;
	mission->velocityXY = velXY;
	mission->velocityZ = velZ;
	return mission;
}

BattleUnitMission *BattleUnitMission::dropItem(BattleUnit &, sp<AEquipment> item)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::DropItem;
	mission->item = item;
	return mission;
}

int BattleUnitMission::getFacingDelta(Vec2<int> curFacing, Vec2<int> tarFacing)
{
	int curFac = facing_dir_map.at(curFacing);
	int tarFac = facing_dir_map.at(tarFacing);
	int result = curFac - tarFac;
	if (result < 0)
	{
		result += 8;
	}
	return result;
}

Vec2<int> BattleUnitMission::getFacing(BattleUnit &u, Vec3<int> to, int facingDelta)
{
	return getFacing(u, (Vec3<int>)u.position, to, facingDelta);
}

Vec2<int> BattleUnitMission::getFacingStep(BattleUnit &u, Vec2<int> targetFacing, int facingDelta)
{
	Vec2<int> dest = u.facing;

	// Turn
	int curFacing = facing_dir_map.at(u.facing);
	int tarFacing = facing_dir_map.at(targetFacing) + facingDelta;
	if (tarFacing > 7)
		tarFacing -= 8;
	if (curFacing == tarFacing)
		return dest;

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
	return dest;
}

Vec2<int> BattleUnitMission::getFacing(BattleUnit &u, Vec3<float> from, Vec3<float> to,
                                       int facingDelta)
{
	float closestAngle = FLT_MAX;
	Vec3<int> closestVector = {0, 0, 0};
	Vec3<float> targetFacing = (Vec3<float>)(to - from);
	if (targetFacing.x == 0.0f && targetFacing.y == 0.0f)
	{
		closestVector = {u.goalFacing.x, u.goalFacing.y, 0};
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
	if (facingDelta == 0)
	{
		return {closestVector.x, closestVector.y};
	}
	else
	{
		Vec2<int> targetFacing = {closestVector.x, closestVector.y};
		int tarFacing = facing_dir_map.at(targetFacing) + facingDelta;
		if (tarFacing > 7)
			tarFacing -= 8;
		if (tarFacing < 0)
			tarFacing += 8;
		return dir_facing_map.at(tarFacing);
	}
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
	auto *mission = new BattleUnitMission();
	mission->type = Type::Turn;
	mission->requireGoal = requireGoal;
	mission->freeTurn = free;
	mission->targetFacing = getFacing(u, from, to);
	mission->targetBodyState = u.target_body_state;
	return mission;
}

BattleUnitMission *BattleUnitMission::reachGoal(BattleUnit &u, int facingDelta)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::ReachGoal;
	mission->targetLocation = u.goalPosition;
	mission->facingDelta = facingDelta;
	mission->targetFacing = getFacing(u, u.position, u.goalPosition, facingDelta);
	mission->requireGoal = false;
	mission->targetBodyState = u.target_body_state;
	return mission;
}

BattleUnitMission *BattleUnitMission::teleport(BattleUnit &, sp<AEquipment> item, Vec3<int> target)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::Teleport;
	mission->item = item;
	mission->targetLocation = target;
	return mission;
}

BattleUnitMission *BattleUnitMission::brainsuck(BattleUnit &u, StateRef<BattleUnit> target,
                                                int facingDelta)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::Brainsuck;
	mission->targetUnit = target;
	mission->targetBodyState = BodyState::Jumping;
	mission->targetFacing = u.facing;
	mission->facingDelta = facingDelta;
	return mission;
}

BattleUnitMission *BattleUnitMission::jump(BattleUnit &u, Vec3<float> target, BodyState state,
                                           bool requireFacing)
{
	auto *mission = new BattleUnitMission();
	mission->type = Type::Jump;
	mission->jumpTarget = target;
	mission->targetFacing = requireFacing ? getFacing(u, target) : u.goalFacing;
	mission->requireGoal = requireFacing;
	mission->targetBodyState = state;
	return mission;
}

bool BattleUnitMission::spendAgentTUs(GameState &state, BattleUnit &u, int cost, bool cancel,
                                      bool ignoreKneelReserve, bool allowInterrupt)
{
	if (costPaidUpFront > 0)
	{
		if (costPaidUpFront >= cost)
		{
			costPaidUpFront -= cost;
			cost = 0;
		}
		else
		{
			cost -= costPaidUpFront;
			costPaidUpFront = 0;
		}
	}

	if (u.spendTU(state, cost, ignoreKneelReserve, false, allowInterrupt))
	{
		return true;
	}
	if (cancel)
	{
		cancelled = true;
	}
	else
	{
		u.addMission(state, BattleUnitMission::Type::AcquireTU);
	}
	return false;
}

bool BattleUnitMission::getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest)
{
	if (cancelled)
	{
		return false;
	}
	if (type != Type::Brainsuck)
	{
		// If turning or changing body state then we cannot move
		if (u.facing != u.goalFacing || u.current_body_state != u.target_body_state)
		{
			return false;
		}
		// If we have not yet consumed queued up body or turning change, then we cannot move
		if (u.facing != targetFacing || u.current_body_state != targetBodyState)
		{
			return false;
		}
	}
	switch (this->type)
	{
		case Type::GotoLocation:
			if (state.current_battle->mode == Battle::Mode::TurnBased &&
			    ((!state.current_battle->interruptQueue.empty()) ||
			     (u.owner == state.current_battle->currentActiveOrganisation &&
			      !state.current_battle->interruptUnits.empty()) ||
			     (u.owner != state.current_battle->currentActiveOrganisation &&
			      state.current_battle->interruptUnits.find({&state, u.id}) ==
			          state.current_battle->interruptUnits.end())))
			{
				u.addMission(state, BattleUnitMission::acquireTU(u, true));
				return false;
			}
			return advanceAlongPath(state, u, dest);
		case Type::Brainsuck:
			return advanceBrainsucker(state, u, dest);
		default:
			return false;
	}
}

bool BattleUnitMission::getNextFacing(GameState &state, BattleUnit &u, Vec2<int> &dest)
{
	if (cancelled)
	{
		return false;
	}
	// If turning or changing body state then we cannot turn
	if (u.current_body_state != u.target_body_state)
	{
		return false;
	}
	// If we have not yet consumed queued up body change, we cannot turn
	// Unless we're throwing, in which case it's complicated
	if (u.current_body_state != targetBodyState)
	{
		if (this->type == Type::ThrowItem)
		{
			// If we are turning, our priority is: [STAND] -> [TURN] -> [THROW]
			// Therefore, if we're not standing, body takes priority, otherwise turning does
			if (u.current_body_state != BodyState::Standing)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	switch (this->type)
	{
		case Type::Turn:
		case Type::ThrowItem:
		case Type::GotoLocation:
		case Type::ReachGoal:
		case Type::Jump:
		case Type::Brainsuck:
			if (state.current_battle->mode == Battle::Mode::TurnBased &&
			    ((!state.current_battle->interruptQueue.empty()) ||
			     (u.owner == state.current_battle->currentActiveOrganisation &&
			      !state.current_battle->interruptUnits.empty()) ||
			     (u.owner != state.current_battle->currentActiveOrganisation &&
			      state.current_battle->interruptUnits.find({&state, u.id}) ==
			          state.current_battle->interruptUnits.end())))
			{
				u.addMission(state, BattleUnitMission::acquireTU(u, true));
				return false;
			}
			return advanceFacing(state, u, dest);
		default:
			return false;
	}
}

bool BattleUnitMission::getNextBodyState(GameState &state, BattleUnit &u, BodyState &dest)
{
	if (cancelled)
	{
		return false;
	}
	// If we are throwing and ready to do so then body state change must wait until we turn
	if (this->type == Type::ThrowItem && u.current_body_state == BodyState::Standing)
	{
		if (u.facing != u.goalFacing || u.facing != targetFacing)
		{
			return false;
		}
	}

	switch (this->type)
	{
		case Type::Turn:
		case Type::ThrowItem:
		case Type::ChangeBodyState:
		case Type::GotoLocation:
		case Type::ReachGoal:
		case Type::Jump:
		case Type::Brainsuck:
			if (!u.agent->isBodyStateAllowed(targetBodyState))
			{
				if (targetBodyState == BodyState::Flying)
				{
					targetBodyState = BodyState::Standing;
				}
				else
				{
					LogError("Unit %s (%s) (%s) lost capability to attain bodyState %d?", u.id,
					         u.agent->name, u.agent->type->id, (int)targetBodyState);
				}
			}
			return advanceBodyState(state, u, targetBodyState, dest);
		default:
			return false;
	}
}

MovementState BattleUnitMission::getNextMovementState(GameState &, BattleUnit &u)
{
	if (cancelled)
	{
		return MovementState::None;
	}
	switch (this->type)
	{
		case Type::Brainsuck:
			return MovementState::Brainsuck;
		case Type::GotoLocation:
		case Type::ReachGoal:
			if (u.current_body_state == BodyState::Jumping ||
			    u.target_body_state == BodyState::Jumping || targetBodyState == BodyState::Jumping)
			{
				if (u.agent->isMovementStateAllowed(MovementState::Normal))
				{
					return MovementState::Normal;
				}
				else
				{
					LogError("Agent with allowed Jumping body state does not have allowed Normal "
					         "movement");
					return u.current_movement_state;
				}
			}
			switch (facingDelta)
			{
				case 0:
				case 1:
				case -1:
					// Normal movement
					return MovementState::None;
				case 2:
				case -2:
					if (u.agent->isMovementStateAllowed(MovementState::Strafing))
					{
						return MovementState::Strafing;
					}
					else
					{
						cancelled = true;
						break;
					}
				case 3:
				case -3:
				case 4:
					if (u.agent->isMovementStateAllowed(MovementState::Reverse))
					{
						return MovementState::Reverse;
					}
					else
					{
						cancelled = true;
						break;
					}
				default:
					LogError("Invalid facingDelta %d", facingDelta);
					break;
			}
			break;
		default:
			break;
	}
	return MovementState::None;
}

void BattleUnitMission::update(GameState &state, BattleUnit &u, unsigned int ticks, bool finished)
{
	switch (this->type)
	{
		case Type::Jump:
			if (!jumped && !u.falling && u.atGoal && u.facing == u.goalFacing &&
			    u.facing == targetFacing &&
			    (u.current_body_state == u.target_body_state ||
			     targetBodyState == BodyState::Jumping) &&
			    u.target_body_state == targetBodyState)
			{
				// Jumping cost assumed same as walking into tile
				int cost = STANDART_MOVE_TU_COST;
				if (!spendAgentTUs(state, u, cost, true))
				{
					return;
				}
				u.launch(state, jumpTarget, targetBodyState);
				jumped = true;
			}
			return;
		case Type::GotoLocation:
		{
			if (finished)
			{
				if (allowRunningAway)
				{
					if (u.tileObject->getOwningTile()->getHasExit(u.isLarge()))
					{
						u.retreat(state);
					}
				}
			}
			return;
		}
		case Type::Snooze:
		{
			if (ticks >= timeToSnooze)
			{
				this->timeToSnooze = 0;
			}
			else
			{
				this->timeToSnooze -= ticks;
			}
			return;
		}
		case Type::ThrowItem:
			// Half way there - throw the item!
			if (item && u.current_body_state == BodyState::Throwing &&
			    u.target_body_state == BodyState::Throwing)
			{
				// Ensure item still belongs to agent
				if (item->ownerAgent == u.agent)
				{
					item->ownerAgent->removeEquipment(state, item);
					item->ownerUnit = {&state, u.id};
					item->throwItem(state, targetLocation, velocityXY, velocityZ);
				}
				item = nullptr;
				targetBodyState = BodyState::Standing;
			}
			return;
		case Type::ReachGoal:
			if (!finished && u.current_movement_state == MovementState::None &&
			    u.facing == u.goalFacing && u.facing == targetFacing &&
			    u.current_body_state == u.target_body_state &&
			    u.current_body_state == targetBodyState)
			{
				// Reaching goal means we already requested this node and paid the price, so no TU
				// cost
				u.startMoving(state);
			}
			return;
		case Type::AcquireTU:
		case Type::RestartNextMission:
		case Type::ChangeBodyState:
		case Type::DropItem:
		case Type::Turn:
		case Type::Teleport:
			return;
		case Type::Brainsuck:
			// Unit fell down, cancel brainsucker
			if (!targetUnit->isConscious())
			{
				cancelled = true;
				u.resetGoal();
				u.startFalling(state);
				return;
			}
			// Do not get stuck during brainsuck, move!
			if (!finished && u.current_movement_state == MovementState::None)
			{
				u.startMoving(state);
			}

			brainsuckTicksAccumulated += ticks;
			while ((brainsuckTicksAccumulated * 4 + TICKS_TO_BRAINSUCK - 1) / TICKS_TO_BRAINSUCK >
			       brainsuckSoundsPlayed)
			{
				switch (brainsuckSoundsPlayed)
				{
					case 0:
					case 2:
						fw().soundBackend->playSample(
						    state.battle_common_sample_list->brainsuckerSuck, u.position);
						break;
					case 1:
					case 3:
						if (targetUnit->agent->type->fatalWoundSfx.find(
						        targetUnit->agent->gender) !=
						        targetUnit->agent->type->fatalWoundSfx.end() &&
						    !targetUnit->agent->type->fatalWoundSfx.at(targetUnit->agent->gender)
						         .empty())
						{
							fw().soundBackend->playSample(
							    pickRandom(state.rng, targetUnit->agent->type->fatalWoundSfx.at(
							                              targetUnit->agent->gender)),
							    targetUnit->position);
						}
						break;
					default:
						break;
				}
				brainsuckSoundsPlayed++;
			}
			if (brainsuckTicksAccumulated >= TICKS_TO_BRAINSUCK &&
			    state.current_battle->interruptQueue.empty() &&
			    state.current_battle->interruptUnits.empty())
			{
				if (randBoundsExclusive(state.rng, 0, 100) < BRAINSUCK_CHANCE)
				{
					targetUnit->sendAgentEvent(state, GameEventType::AgentBrainsucked, true);
					// Extra score penalty for being brainsucked
					if (targetUnit->agent->owner == state.getPlayer())
					{
						state.current_battle->score.casualtyPenalty -=
						    targetUnit->agent->type->score;
					}
					targetUnit->changeOwner(state, state.getAliens());
					targetUnit->agent->modified_stats.psi_defence = 100;
				}
				u.die(state);
			}
			return;
		default:
			LogWarning("TODO: Implement update");
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

bool BattleUnitMission::isFinishedInternal(GameState &, BattleUnit &u)
{
	if (cancelled)
	{
		return true;
	}
	switch (this->type)
	{
		case Type::AcquireTU:
			return false;
		case Type::ReachGoal:
			return u.atGoal || u.falling;
		case Type::GotoLocation:
			return currentPlannedPath.empty() && u.atGoal && u.goalFacing == u.facing &&
			       u.facing == targetFacing && u.target_body_state == u.current_body_state &&
			       u.current_body_state == targetBodyState;
		case Type::Snooze:
			return timeToSnooze == 0;
		case Type::ChangeBodyState:
			return u.target_body_state == u.current_body_state &&
			       u.current_body_state == targetBodyState;
		case Type::ThrowItem:
			return !item && u.current_body_state == BodyState::Standing;
		case Type::Turn:
			return u.goalFacing == u.facing && u.facing == targetFacing &&
			       u.target_body_state == u.current_body_state &&
			       u.current_body_state == targetBodyState;
		// RestartNextMission is a dud, used to call next mission's start() again
		case Type::RestartNextMission:
			return true;
		// Sanity check for missions that should always complete when start() is called
		case Type::Teleport:
		case Type::DropItem:
			if (item)
			{
				return false;
			}
			return true;
		case Type::Jump:
			return jumped && !u.falling && u.goalFacing == u.facing &&
			       u.target_body_state == u.current_body_state;
		case Type::Brainsuck:
			// Is finished only when unit dies on timer
			return false;
		default:
			LogWarning("TODO: Implement isfinishedinternal");
			return false;
	}
}

void BattleUnitMission::start(GameState &state, BattleUnit &u)
{
	LogWarning("Unit %s mission \"%s\" starting", u.id, getName());

	switch (this->type)
	{
		case Type::Teleport:
		{
			if (item)
			{
				// Check if we can be there
				auto t = u.tileObject->map.getTile(targetLocation);
				bool canStand = t->getCanStand(u.isLarge());
				if (!t->getPassable(u.isLarge(), u.agent->type->bodyType->maxHeight) ||
				    t->getUnitIfPresent(true, true, false, nullptr, false, u.isLarge()) ||
				    (!u.canFly() && !canStand))
				{
					cancelled = true;
					return;
				}
				// Teleportation requires full teleporter ammo
				if (item->ammo != item->type->max_ammo)
				{
					cancelled = true;
					return;
				}
				if (item->type->type != AEquipmentType::Type::Teleporter)
				{
					LogError("Unit is trying to teleport using non-teleporter item %s!?",
					         item->type->name);
					cancelled = true;
					return;
				}
				int cost = u.getTeleporterCost();
				if (!spendAgentTUs(state, u, cost, true))
				{
					return;
				}

				// Process item
				item->ammo = 0;
				item = nullptr;

				// Teleport unit
				// Remove all other missions
				u.missions.remove_if(
				    [this](const up<BattleUnitMission> &mission) { return mission.get() != this; });
				u.stopAttacking();
				u.setPosition(state, t->getRestingPosition(u.isLarge()), true);
				u.resetGoal();
				BodyState teleBodyState = canStand ? BodyState::Standing : BodyState::Flying;
				if (!u.agent->isBodyStateAllowed(teleBodyState))
					teleBodyState = BodyState::Flying;
				if (!u.agent->isBodyStateAllowed(teleBodyState))
					teleBodyState = BodyState::Kneeling;
				if (!u.agent->isBodyStateAllowed(teleBodyState))
					teleBodyState = BodyState::Prone;
				if (!u.agent->isBodyStateAllowed(teleBodyState))
					LogError("Unit has no valid body state? WTF?");
				u.setBodyState(state, teleBodyState);
				u.setMovementState(MovementState::None);
				u.falling = false;

				if (state.battle_common_sample_list->teleport)
				{
					fw().soundBackend->playSample(state.battle_common_sample_list->teleport,
					                              u.getPosition());
				}
			}
			return;
		}
		case Type::Brainsuck:
		{
			if (targetUnit->brainSucker && targetUnit->brainSucker.id != u.id)
			{
				cancelled = true;
				return;
			}
			targetUnit->brainSucker = {&state, u.id};
			u.setBodyState(state, BodyState::Jumping);
			u.falling = false;
			u.launched = false;
			u.resetGoal();
			return;
		}
		case Type::DropItem:
		{
			if (!u.agent->type->inventory)
			{
				cancelled = true;
				return;
			}
			if (item)
			{
				if (item->ownerAgent)
				{
					item->ownerAgent->removeEquipment(state, item);
				}
				item->ownerUnit = {&state, u.id};
				// Drop item
				auto bi = state.current_battle->placeItem(
				    state, item,
				    u.position + Vec3<float>{0.0, 0.0,
				                             (u.current_body_state == BodyState::Downed ||
				                              u.current_body_state == BodyState::Dead)
				                                 ? 0.0f
				                                 : (float)u.getCurrentHeight() / 80.0f});
				bi->falling = true;
			}
			item = nullptr;
			return;
		}
		case Type::ThrowItem:
		{
			if (!u.agent->type->inventory)
			{
				cancelled = true;
				return;
			}
			return;
		}
		case Type::GotoLocation:
			// Check if can move
			if (!u.canMove())
			{
				cancelled = true;
				return;
			}
			// Check if can move in a requested manner
			getNextMovementState(state, u);
			if (cancelled)
			{
				return;
			}

			// Reset target body state and facing
			targetBodyState = u.target_body_state;
			targetFacing = u.goalFacing;

			// If we have already tried to use this mission, see if path is still valid
			if (!currentPlannedPath.empty())
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
			if (currentPlannedPath.empty())
			{
				this->setPathTo(state, u, targetLocation);
			}
			return;
		case Type::Turn:
			// Reset target body state
			targetBodyState = u.target_body_state;
			return;
		case Type::ReachGoal:
			// Reset target body state
			targetBodyState = u.target_body_state;
			// If can't move reach immediately
			if (!u.canMove())
			{
				u.setPosition(state, u.goalPosition);
				u.atGoal = true;
			}
			return;
		case Type::Jump:
			if (!requireGoal)
			{
				u.setFacing(state, u.goalFacing);
			}
			cancelled = u.isLarge() || !u.canLaunch(jumpTarget);
			return;
		case Type::ChangeBodyState:
		case Type::AcquireTU:
		case Type::RestartNextMission:
		case Type::Snooze:
			return;
		default:
			LogWarning("TODO: Implement start");
			return;
	}
}

void BattleUnitMission::setPathTo(GameState &state, BattleUnit &u, Vec3<int> target)
{
	auto unitTile = u.tileObject;
	if (unitTile)
	{
		auto &map = u.tileObject->map;
		auto to = map.getTile(target);
		bool approachOnly = false;
		// Check if target tile is valid
		while (true)
		{
			// If unit cannot move at all - cancel
			if (!u.canMove())
			{
				LogInfo("Cannot move to %d %d %d, unit has no movement ability", target.x, target.y,
				        target.z);
				cancelled = true;
				return;
			}
			// If target occupied - try to find an adjacent unoccupied tile
			if (!to->getPassable(u.isLarge(), u.agent->type->bodyType->maxHeight) ||
			    to->getUnitIfPresent(true, true, false, u.tileObject, false, u.isLarge()))
			{
				approachOnly = false;
				for (int x = -1; x <= 1; x++)
				{
					for (int y = -1; y <= 1; y++)
					{
						for (int z = -1; z <= 1; z++)
						{
							if (x == 0 && y == 0 && z == 0)
							{
								continue;
							}
							auto targetPos = target + Vec3<int>(x, y, z);
							if (!map.tileIsValid(targetPos))
							{
								continue;
							}
							auto targetTile = map.getTile(targetPos);
							if (targetTile->getPassable(u.isLarge(),
							                            u.agent->type->bodyType->maxHeight) &&
							    !targetTile->getUnitIfPresent(true, true, false, u.tileObject,
							                                  false, u.isLarge()))
							{
								LogInfo("Cannot move to %d %d %d, found an adjacent free tile, "
								        "moving to an adjacent tile",
								        target.x, target.y, target.z);
								approachOnly = true;
								break;
							}
						}
						if (approachOnly)
						{
							break;
						}
					}
					if (approachOnly)
					{
						break;
					}
				}
				if (!approachOnly)
				{
					LogInfo("Cannot move to %d %d %d, impassable", target.x, target.y, target.z);
					cancelled = true;
					return;
				}
			}
			if (approachOnly)
			{
				break;
			}
			// If target is in the air and unit cannot fly
			if (!u.canFly() && !to->getCanStand(u.isLarge()))
			{
				target.z--;
				if (target.z == -1)
				{
					LogError("Solid ground missing on level 0? Reached %d %d %d", target.x,
					         target.y, target.z);
					cancelled = true;
					return;
				}
				to = map.getTile(target);
				continue;
			}
			// Reached the end - everything's fine, path to target
			break;
		}

		auto path = state.current_battle->findShortestPath(
		    u.goalPosition, target, BattleUnitTileHelper{map, u}, approachOnly, demandGiveWay,
		    !blockedByMovingUnit);

		// Cancel movement if the closest path ends at the current position
		if (path.size() == 1 && path.back() == Vec3<int>{u.position})
		{
			LogInfo("Cannot move to %s, closest path ends at origin", Vec3<int>{u.goalPosition});
			cancelled = true;
			return;
		}

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
		LogError("Mission %s: Unit without tileobject attempted pathfinding!", getName());
		cancelled = true;
		return;
	}
}

bool BattleUnitMission::advanceAlongPath(GameState &state, BattleUnit &u, Vec3<float> &dest)
{
	bool realTime = state.current_battle->mode == Battle::Mode::RealTime;

	if (u.isUnconscious() || u.isDead() || currentPlannedPath.empty())
	{
		return false;
	}

	// Go to goal if not on goal (can happen if jumped)
	auto restingPos = u.tileObject->getOwningTile()->getRestingPosition(u.isLarge());
	if (restingPos != u.position)
	{
		// Cancel jumping state
		if (targetBodyState == BodyState::Jumping)
		{
			targetBodyState = BodyState::Standing;
		}
		dest = restingPos;
		return true;
	}

	// Reached end of path
	if (currentPlannedPath.size() == 1)
	{
		currentPlannedPath.clear();
		return false;
	}

	// Reset body and facing settings
	targetBodyState = u.target_body_state;
	targetFacing = u.goalFacing;
	requireGoal = true;
	freeTurn = false;

	// Get target location
	auto it = ++currentPlannedPath.begin();
	auto pos = *it++;

	// See if we can actually go there
	auto tFrom = u.tileObject->getOwningTile();
	auto &map = tFrom->map;
	auto tTo = map.getTile(pos);
	float cost = 0;
	bool jumped = false;
	bool closedDoorInTheWay = false;
	if (tFrom->position != pos &&
	    !BattleUnitTileHelper{map, u}.canEnterTile(tFrom, tTo, !u.canFly(), jumped, cost,
	                                               closedDoorInTheWay, true))
	{
		// Next tile became impassable, pick a new path
		currentPlannedPath.clear();
		u.addMission(state, BattleUnitMission::snooze(u, 1));
		u.addMission(state, Type::RestartNextMission);
		return false;
	}

	// See if we can make a shortcut
	// --
	// When ordering move to a unit already on the move, we can have a situation
	// where going directly to 2nd step in the path is faster than going to the first
	// In this case, we should skip unnecessary steps
	// --
	// Start with position after next
	// If the next position has a node and we can go directly to that node,
	// then update current position and iterator
	float newCost = 0;
	bool newDoorInWay = false;
	bool newJumped = false;
	while (it != currentPlannedPath.end() &&
	       (tFrom->position == *it || (allowSkipNodes && BattleUnitTileHelper{map, u}.canEnterTile(
	                                                         tFrom, map.getTile(*it), !u.canFly(),
	                                                         newJumped, newCost, newDoorInWay))))
	{
		currentPlannedPath.pop_front();
		it = ++currentPlannedPath.begin();
		pos = *it++;
		tTo = map.getTile(pos);
		cost = newCost;
		closedDoorInTheWay = newDoorInWay;
		if (newJumped)
		{
			jumped = true;
			break;
		}
	}

	// Do we need to turn? If so, skip trying to fix the body
	auto nextFacing = getFacing(u, pos, jumped ? 0 : facingDelta);
	if (nextFacing == u.facing)
	{
		// Do we need to change our body state?
		if (jumped)
		{
			if (u.current_body_state != BodyState::Standing)
			{
				targetBodyState = BodyState::Standing;
				return false;
			}
			// We will change to jumping after we ensure noone is blocking
		}
		else
		{
			switch (u.movement_mode)
			{
				// If we want to move prone but are not prone - go prone if we can
				case MovementMode::Prone:
					if (facingDelta == 0)
					{
						if (u.current_body_state == BodyState::Prone)
						{
							// Ensure we can go prone at target tile
							if (u.canProne(pos, u.facing))
							{
								break;
							}
							// If we can't go prone we will fall-through into walking below
						}
						else if (u.canProne(u.position, u.facing) && u.canProne(pos, u.facing))
						{
							targetBodyState = BodyState::Prone;
							return false;
						}
					}
				// Intentional fall-though.
				// - If we are in strafe mode - we never go prone
				// - If we want to go prone but cannot go prone - we should act as if
				//    we're told to walk/run
				// - We can expect that we can enter the target tile
				// - We can expect agent to be able to either stand, fly or prone
				// ----
				// If we want to move standing up but not standing/flying - go standing/flying
				// appropriately to the terrain
				case MovementMode::Walking:
				case MovementMode::Running:
				{
					auto t = u.tileObject->getOwningTile();
					targetBodyState =
					    t->getCanStand(u.isLarge()) && map.getTile(pos)->getCanStand(u.isLarge())
					        ? BodyState::Standing
					        : BodyState::Flying;
					if (!u.agent->isBodyStateAllowed(targetBodyState))
					{
						if (u.agent->isBodyStateAllowed(BodyState::Flying))
						{
							targetBodyState = BodyState::Flying;
						}
						else
						{
							// We have to move standing up, but we cannot as we can only prone
							// This will temporarily allow us to go prone violating prone rules
							// Otherwise prone units would be immobile when cornered
							targetBodyState = BodyState::Prone;
						}
					}
					if (targetBodyState != u.current_body_state)
					{
						return false;
					}
					break;
				}
			}
		}
	}

	// Is there a unit in the target tile other than us?
	auto blockingUnits = tTo->getUnits(true, true, false, u.tileObject, false, u.isLarge());
	bool needSnooze = false;
	for (auto &blockingUnit : blockingUnits)
	{
		// FIXME: Check unit's allegiance? Will neutrals give way? I think they did in vanilla!
		u.current_movement_state = MovementState::None;
		// If this unit is still patient enough, and we can ask that unit to give way
		if (giveWayAttemptsRemaining-- > 0 &&
		    blockingUnit->owner == u.owner
		    // and we're not trying to stay there
		    && currentPlannedPath.size() > 1
		    // and unit we're asking is not big
		    // (coding giving way for large units is too much of a fuss,
		    // and ain't going to be used a lot anyway, just path around them)
		    && !blockingUnit->isLarge() && !u.isLarge())
		{
			// Ask unit to give way to us
			blockingUnit->requestGiveWay(u, currentPlannedPath, pos);
			needSnooze = true;
		}
		else if (u.owner->isRelatedTo(blockingUnit->owner) == Organisation::Relation::Hostile)
		{
			// Unit bumped into an enemy, stop moving immediately
			currentPlannedPath.clear();
			cancelled = true;
			return false;
		}
		else
		{
			// Unit's patience has ran out
			currentPlannedPath.clear();
			demandGiveWay = false;
			giveWayAttemptsRemaining = 1;
			blockedByMovingUnit = !blockingUnit->isStatic();
			u.addMission(state, Type::RestartNextMission);
			return false;
		}
	}
	if (needSnooze)
	{
		// Snooze for a moment and try again
		u.addMission(state, snooze(u, 16));
		return false;
	}

	// Is there a door?
	if (closedDoorInTheWay)
	{
		// Snooze for a moment and try again
		u.addMission(state, snooze(u, 8));
		return false;
	}

	// Running and Proning adjusts cost
	if (u.agent->canRun() && facingDelta == 0 && !jumped &&
	    u.movement_mode == MovementMode::Running)
	{
		cost /= 2;
	}
	if (u.current_body_state == BodyState::Prone)
	{
		cost *= 3;
		cost /= 2;
	}
	int intCost = (int)cost;
	// See if we can afford doing this move
	if (!spendAgentTUs(state, u, intCost))
	{
		return false;
	}

	// Do we need to turn?
	if (nextFacing != u.facing)
	{
		targetFacing = nextFacing;
		costPaidUpFront += intCost;
		freeTurn = true;
		// If we need to turn, do we need to change stance first?
		switch (u.current_body_state)
		{
			case BodyState::Prone:
				targetBodyState = BodyState::Kneeling;
				break;
			case BodyState::Jumping:
				targetBodyState = BodyState::Standing;
				break;
			default:
				break;
		}
		return false;
	}

	// Spend stamina TB.  As per Mell from forums it takes:
	// - 0.6 vanilla stamina to run regardless of diagonal or not
	// - 0.85 vanilla stamina to go prone regardless of diagonal or not
	if (!realTime)
	{
		int staCost = 0;
		if (u.current_body_state == BodyState::Prone)
		{
			staCost = randBoundsInclusive(state.rng, 8, 9);
		}
		else if (u.movement_mode == MovementMode::Running)
		{
			staCost = 6;
		}
		if (u.agent->modified_stats.stamina < staCost)
		{
			u.agent->modified_stats.stamina = 0;
		}
		else
		{
			u.agent->modified_stats.stamina -= staCost;
		}
	}

	// Now finally we can go jumping
	if (jumped)
	{
		// Can change to jumping on-the-go
		targetBodyState = BodyState::Jumping;
	}

	// Finally, we're moving!
	currentPlannedPath.pop_front();

	dest = map.getTile(pos)->getRestingPosition(u.isLarge());

	// Land on the edge if jumping
	if (jumped)
	{
		auto targetVector = dest - u.position;
		targetVector.x = clamp(targetVector.x, -0.4f, 0.4f);
		targetVector.y = clamp(targetVector.y, -0.4f, 0.4f);
		dest -= targetVector;
	}
	return true;
}

bool BattleUnitMission::advanceBrainsucker(GameState &state, BattleUnit &u, Vec3<float> &dest)
{
	std::ignore = state;
	dest = targetUnit->getMuzzleLocation();
	targetFacing =
	    getFacing(u, Vec3<float>{0.0f, 0.0f, 0.0f},
	              Vec3<float>{targetUnit->facing.x, targetUnit->facing.y, 0.0f}, facingDelta);
	return true;
}

bool BattleUnitMission::advanceFacing(GameState &state, BattleUnit &u, Vec2<int> &dest)
{
	// Already facing properly?
	if (u.facing == targetFacing)
		return false;

	// Go to goal first
	if (!u.atGoal && requireGoal)
	{
		u.addMission(state, Type::ReachGoal);
		return false;
	}

	if (u.current_movement_state != MovementState::Brainsuck)
	{
		// If we need to turn, do we need to change stance first?
		switch (u.current_body_state)
		{
			case BodyState::Prone:
				targetBodyState = BodyState::Kneeling;
				return false;
			case BodyState::Jumping:
				targetBodyState = BodyState::Standing;
				return false;
			default:
				break;
		}
	}

	// If throwing then pay up front so that we can't turn for free
	if (targetBodyState == BodyState::Throwing)
	{
		int cost = u.getBodyStateChangeCost(u.current_body_state, targetBodyState);
		if (!spendAgentTUs(state, u, cost, true, true, true))
		{
			return false;
		}
		costPaidUpFront += cost;
	}

	dest = getFacingStep(u, targetFacing);

	// Calculate and spend cost
	int cost = freeTurn ? 0 : u.getTurnCost();
	if (!spendAgentTUs(state, u, cost, true))
	{
		return false;
	}

	return true;
}

bool BattleUnitMission::advanceBodyState(GameState &state, BattleUnit &u, BodyState targetState,
                                         BodyState &dest)
{
	if (targetState == u.target_body_state)
	{
		return false;
	}
	if (u.current_body_state != u.target_body_state)
	{
		LogError("Requesting to change body state during another body state change?");
		u.setBodyState(state, u.target_body_state);
	}

	// Transition for stance changes
	if (targetBodyState != BodyState::Dead && targetBodyState != BodyState::Downed)
	{
		// If trying to fly stand up first
		if (targetState == BodyState::Flying && u.current_body_state != BodyState::Standing &&
		    u.agent->isBodyStateAllowed(BodyState::Standing))
		{
			return advanceBodyState(state, u, BodyState::Standing, dest);
		}
		// If trying to stop flying stand up first
		if (targetState != BodyState::Standing && u.current_body_state == BodyState::Flying &&
		    u.agent->isBodyStateAllowed(BodyState::Standing))
		{
			return advanceBodyState(state, u, BodyState::Standing, dest);
		}
		// If trying to go anywhere from prone go kneel first
		if (targetState != BodyState::Kneeling && u.current_body_state == BodyState::Prone &&
		    u.agent->isBodyStateAllowed(BodyState::Kneeling))
		{
			return advanceBodyState(state, u, BodyState::Kneeling, dest);
		}
		// If trying to go prone from anywhere then kneel first
		if (targetState == BodyState::Prone && u.current_body_state != BodyState::Kneeling &&
		    u.agent->isBodyStateAllowed(BodyState::Kneeling))
		{
			return advanceBodyState(state, u, BodyState::Kneeling, dest);
		}
		// If trying to throw then stand up first
		if (targetState == BodyState::Throwing && u.current_body_state != BodyState::Standing)
		{
			return advanceBodyState(state, u, BodyState::Standing, dest);
		}
	}

	// Calculate and spend cost

	// Cost to reach goal is free
	int cost =
	    type == Type::ReachGoal ? 0 : u.getBodyStateChangeCost(u.target_body_state, targetState);
	// If insufficient TUs - cancel missions other than GotoLocation
	if (!spendAgentTUs(state, u, cost, type != Type::GotoLocation,
	                   targetState == BodyState::Kneeling))
	{
		return false;
	}

	// Finished
	dest = targetState;
	return true;
}

UString BattleUnitMission::getName()
{
	UString name = "UNKNOWN";
	switch (this->type)
	{
		case Type::AcquireTU:
			name = "AcquireTUs";
			break;
		case Type::GotoLocation:
			name = "GotoLocation " + format(" %s", targetLocation);
			break;
		case Type::Teleport:
			name = "Teleport to " + format(" %s", targetLocation);
			break;
		case Type::RestartNextMission:
			name = "Restart next mission";
			break;
		case Type::Snooze:
			name = "Snooze " + format(" for %u ticks", timeToSnooze);
			break;
		case Type::ChangeBodyState:
			name = "ChangeBodyState " + format("%d", (int)this->targetBodyState);
			break;
		case Type::ThrowItem:
			name = "ThrowItem " +
			       format("%s at %s", item ? item->type->name : "(item is gone)", targetLocation);
			break;
		case Type::DropItem:
			name = "DropItem " + format("%s", item ? item->type->name : "(item is gone)");
			break;
		case Type::ReachGoal:
			name = "ReachGoal";
			break;
		case Type::Turn:
			name = "Turn " + format(" %s", targetFacing);
			break;
		case Type::Brainsuck:
			name = "Brainsuck " + format(" %s", targetUnit.id);
			break;
		case Type::Jump:
			name = "Jump to " + format(" %s", jumpTarget);
			break;
	}
	return name;
}

} // namespace OpenApoc
