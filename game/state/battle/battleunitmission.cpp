#include "game/state/battle/battleunitmission.h"
#include "game/state/battle/battleunit.h"
#include "game/state/gamestate.h"
#include "game/state/tileview/tile.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_battleitem.h"

namespace OpenApoc
{
class BattleUnitTileHelper : public CanEnterTileHelper
{
private:
    TileMap &map;
    BattleUnit &u;

public:
	BattleUnitTileHelper(TileMap &map, BattleUnit &u) : map(map), u(u) {}
        
	bool canEnterTile(Tile *from, Tile *to) const override { float nothing; return canEnterTile(from, to, nothing); }

	// Support 'from' being nullptr for if a vehicle is being spawned in the map
	bool canEnterTile(Tile *from, Tile *to, float &cost) const override
	{
		int costInt = 0;

		// Quick check for movement ability
		if (!u.canMove())
			return false;
		
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
		bool large = u.agent->type->large;
		bool flying = u.canFly();

		// Tiles used by big units
		Tile* toX1 = nullptr;
		Vec3<int> toX1Pos;
		Tile* toY1 = nullptr;
		Vec3<int> toY1Pos;
		Tile* toXY1 = nullptr;
		Vec3<int> toXY1Pos;
		Tile* toZ1 = nullptr;
		Vec3<int> toZ1Pos;
		Tile* toXZ1 = nullptr;
		Vec3<int> toXZ1Pos;
		Tile* toYZ1 = nullptr;
		Vec3<int> toYZ1Pos;
		Tile* toXYZ1 = nullptr;
		Vec3<int> toXYZ1Pos;

		// Check if "to" is passable
		costInt = to->movementCostIn;
		if (large)
		{
			// Can we fit?
			if (toPos.x < 1 || toPos.y < 1 || toPos.z > map.size.z - 2)
			{
				return false;
			}
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
			// Movement cost into the tiles
			costInt = std::max(costInt, toX1->movementCostIn);
			costInt = std::max(costInt, toY1->movementCostIn);
			costInt = std::max(costInt, toXY1->movementCostIn);
			costInt = std::max(costInt, toZ1->movementCostIn);
			costInt = std::max(costInt, toXZ1->movementCostIn);
			costInt = std::max(costInt, toYZ1->movementCostIn);
			costInt = std::max(costInt, toXYZ1->movementCostIn);
			// Movement cost into the walls of the tiles
			costInt = std::max(costInt, to->movementCostLeft);
			costInt = std::max(costInt, to->movementCostRight);
			costInt = std::max(costInt, toX1->movementCostRight);
			costInt = std::max(costInt, toY1->movementCostLeft);
			costInt = std::max(costInt, toZ1->movementCostLeft);
			costInt = std::max(costInt, toZ1->movementCostRight);
			costInt = std::max(costInt, toXZ1->movementCostRight);
			costInt = std::max(costInt, toYZ1->movementCostLeft);
		}
		if (costInt == 255)
		{
			return false;
		}
		// This line prevents soldiers from picking a route that will make them fall
		// Disabling it will allow paths with falling
		if (!flying)
		{
			bool solid = to->canStand;
			if (large)
			{
				solid = solid || toX1->canStand;
				solid = solid || toY1->canStand;
				solid = solid || toXY1->canStand;
			}
			if (!solid)
				return false;
		}
		// If falling, then we can only go down, and for free!
		// However, vanilla disallowed that, and instead never let soldiers pick this option
		// So, unless we allow going into non-solid ground for non-flyers,
		// this will never happen (except when giving orders to a falling unit)
		if (!from->canStand && !flying)
		{
			if (fromPos.x != toPos.x || fromPos.y != toPos.y || fromPos.z >= toPos.z)
			{
				return false;
			}
			cost = 0.0f;
			return true;
		}
		// Can we ascend to target tile
		if (toPos.z > fromPos.z)
		{
			if (!flying)
			{
				// Alexey Andronov (Istrebitel):
				// As per my experiments, having a height value of 26 or more is sufficient 
				// to ascend to the next level, no matter how high is the ground level there
				// Since we're storing values from 1 to 40, not from 0 to 39,
				// as in the mappart_type, and store them in float, we compare to 27/40 here
				// This doesn't work on lifts. To ascend into a lift, we must be under it
				// We can always ascend if we're on a lift and going above into a lift
				// We can only ascend into a lift if we're flying or standing beneath it
				if (!(from->height >= 0.675f && !to->hasLift)
					&& !(from->hasLift && to->hasLift && toPos.x == fromPos.x && toPos.y == fromPos.y))
				{
					return false;
				}
			}
			// Will we bump our head on departure?
			if (large)
			{
				// Will we bump our head when leaving current spot?
				// Check four tiles above our "from"'s head
				if (map.getTile(Vec3<int>{fromPos.x, fromPos.y, fromPos.z + 2})->solidGround
					|| map.getTile(Vec3<int>{fromPos.x - 1, fromPos.y, fromPos.z + 2})->solidGround
					|| map.getTile(Vec3<int>{fromPos.x, fromPos.y - 1, fromPos.z + 2})->solidGround
					|| map.getTile(Vec3<int>{fromPos.x - 1, fromPos.y - 1, fromPos.z + 2})->solidGround
					)
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
		// Will we bump our head on arrival?
		if (large)
		{
			// Check four tiles above our "to"'s head
			if (map.getTile(Vec3<int>{toPos.x, toPos.y, toPos.z + 2})->solidGround
				|| map.getTile(Vec3<int>{toPos.x - 1, toPos.y, toPos.z + 2})->solidGround
				|| map.getTile(Vec3<int>{toPos.x, toPos.y - 1, toPos.z + 2})->solidGround
				|| map.getTile(Vec3<int>{toPos.x - 1, toPos.y - 1, toPos.z + 2})->solidGround
				)
			{
				// If we have solid ground upon arriving - check if we fit
				float maxHeight = to->height;
				maxHeight = std::max(maxHeight, toX1->height);
				maxHeight = std::max(maxHeight, toY1->height);
				maxHeight = std::max(maxHeight, toXY1->height);
				if (u.agent->type->height + maxHeight * 40 - 1 > 80)
				{
					return false;
				}
			}
		}
		else
		{
			// If we have solid ground upon arriving - check if we fit
			if (map.getTile(Vec3<int>{toPos.x, toPos.y, toPos.z + 1})->solidGround
				&& u.agent->type->height + to->height * 40 - 1 > 40)
			{
				return false;
			}
		}
		// Now check what walls are we intersecting and find out their cost
		// Also check if we bump into floor upon descending
		// If going up, check on upper level, otherwise check on current level
		int z = std::max(fromPos.z, toPos.z);
		// If going down, additionally check that tiles are empty
		bool goingDown = fromPos.z > toPos.z;
		if (large)
		{
			// Large units
			// If moving diagonally
			if (fromPos.x != toPos.x && fromPos.y != toPos.y)
			{
				// If moving: down-right or up-left / SE or NW
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

					Tile* rightTopZ0 = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y) - 2, z);
					Tile* rightBottomZ0 = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y) - 1, z);
					Tile* bottomLeftZ0 = map.getTile(std::max(fromPos.x, toPos.x) - 2, std::max(fromPos.y, toPos.y), z);
					Tile* bottomRightZ0 = map.getTile(std::max(fromPos.x, toPos.x) - 1, std::max(fromPos.y, toPos.y), z);
					Tile* rightTopZ1 = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y) - 2, z + 1);
					Tile* rightBottomZ1 = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y) - 1, z + 1);
					Tile* bottomLeftZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 2, std::max(fromPos.y, toPos.y), z + 1);
					Tile* bottomRightZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 1, std::max(fromPos.y, toPos.y), z + 1);

					costInt = std::max(costInt, rightTopZ0->movementCostLeft);
					costInt = std::max(costInt, rightBottomZ0->movementCostRight);
					costInt = std::max(costInt, bottomLeftZ0->movementCostRight);
					costInt = std::max(costInt, bottomRightZ0->movementCostLeft);
					costInt = std::max(costInt, rightTopZ1->movementCostLeft);
					costInt = std::max(costInt, rightBottomZ1->movementCostRight);
					costInt = std::max(costInt, bottomLeftZ1->movementCostRight);
					costInt = std::max(costInt, bottomRightZ1->movementCostLeft);

					if (goingDown)
					{
						// Going down-right
						if (toPos.x > fromPos.x)
						{
							// Legend: * = from, + = "to" tile, X = tiles we already have
							//  **X
							//  **X
							//  XX+
							// Must check 5 tiles above our head, already have 4 of them
							if (map.getTile(toPos.x, toPos.y, z + 1)->solidGround
								|| rightBottomZ1->solidGround
								|| bottomRightZ1->solidGround
								|| rightTopZ1->solidGround
								|| bottomLeftZ1->solidGround)
							{
								return false;
							}
						}
						// Going up-left
						else
						{
							// Legend: * = from, + = "to" tile, X = tiles we already have
							//  xxX
							//  x+*
							//  X**
							// Must check 5 tiles above our head, already have 2 of them
							if (map.getTile(toPos.x - 1, toPos.y - 1, z + 1)->solidGround
								|| map.getTile(toPos.x - 1, toPos.y, z + 1)->solidGround
								|| map.getTile(toPos.x, toPos.y - 1, z + 1)->solidGround
								|| rightTopZ1->solidGround
								|| bottomLeftZ1->solidGround)
							{
								return false;
							}
						}
					}
				}
				// If moving: down-left or up-right / NE or SW
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

					Tile* topZ0 = map.getTile(std::max(fromPos.x, toPos.x) - 1, std::max(fromPos.y, toPos.y) - 2, z);
					Tile* leftZ0 = map.getTile(std::max(fromPos.x, toPos.x) - 2, std::max(fromPos.y, toPos.y) - 1, z);
					Tile* bottomRightZ0 = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);
					Tile* topZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 1, std::max(fromPos.y, toPos.y) - 2, z + 1);
					Tile* leftZ1 = map.getTile(std::max(fromPos.x, toPos.x) - 2, std::max(fromPos.y, toPos.y) - 1, z + 1);
					Tile* bottomRightZ1 = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z + 1);

					costInt = std::max(costInt, topZ0->movementCostLeft);
					costInt = std::max(costInt, leftZ0->movementCostRight);
					costInt = std::max(costInt, bottomRightZ0->movementCostLeft);
					costInt = std::max(costInt, bottomRightZ0->movementCostRight);
					costInt = std::max(costInt, topZ1->movementCostLeft);
					costInt = std::max(costInt, leftZ1->movementCostRight);
					costInt = std::max(costInt, bottomRightZ1->movementCostLeft);
					costInt = std::max(costInt, bottomRightZ1->movementCostRight);

					if (goingDown)
					{
						// Going up-right
						if (toPos.x > fromPos.x)
						{
							// Legend: * = from, + = "to" tile, X = tiles we already have
							//  xXx
							//  **+
							//  **X
							// Must check 5 tiles above our head, already have 2 of them
							if (map.getTile(toPos.x, toPos.y, z + 1)->solidGround
								|| map.getTile(toPos.x, toPos.y - 1, z + 1)->solidGround
								|| map.getTile(toPos.x - 2, toPos.y - 1, z + 1)->solidGround
								|| topZ1->solidGround
								|| bottomRightZ1->solidGround)
							{
								return false;
							}
						}
						// Going bottom-left
						else
						{
							// Legend: * = from, + = "to" tile, X = tiles we already have
							//  x**
							//  X**
							//  x+X
							// Must check 5 tiles above our head, already have 2 of them
							if (map.getTile(toPos.x, toPos.y, z + 1)->solidGround
								|| map.getTile(toPos.x - 1, toPos.y, z + 1)->solidGround
								|| map.getTile(toPos.x - 1, toPos.y - 2, z + 1)->solidGround
								|| leftZ1->solidGround
								|| bottomRightZ1->solidGround)
							{
								return false;
							}
						}
					}
				}
			}	
			// If moving linearly
			else
			{
				// If moving along X
				if (fromPos.x != toPos.x)
				{
					Tile* topZ0 = map.getTile(toPos.x, toPos.y - 1, z);
					Tile* bottomz0 = map.getTile(toPos.x, toPos.y, z);
					Tile* topZ1 = map.getTile(toPos.x, toPos.y - 1, z + 1);
					Tile* bottomZ1 = map.getTile(toPos.x, toPos.y, z + 1);

					costInt = std::max(costInt, topZ0->movementCostLeft);
					costInt = std::max(costInt, bottomz0->movementCostLeft);
					costInt = std::max(costInt, topZ1->movementCostLeft);
					costInt = std::max(costInt, bottomZ1->movementCostLeft);

					// If going down must check each tile above our head
					if (goingDown)
					{
						if (topZ1->solidGround 
							|| bottomZ1->solidGround
							|| map.getTile(toPos.x - 1, toPos.y - 1, z + 1)->solidGround
							|| map.getTile(toPos.x - 1, toPos.y, z + 1)->solidGround)
						{
							return false;
						}
					}
				}
				// If moving along Y
				else if (fromPos.y != toPos.y)
				{
					Tile* leftZ0 = map.getTile(toPos.x - 1, toPos.y, z);
					Tile* rightZ0 = map.getTile(toPos.x, toPos.y, z);
					Tile* leftZ1 = map.getTile(toPos.x - 1, toPos.y, z + 1);
					Tile* rightZ1 = map.getTile(toPos.x, toPos.y, z + 1);

					costInt = std::max(costInt, leftZ0->movementCostRight);
					costInt = std::max(costInt, rightZ0->movementCostRight);
					costInt = std::max(costInt, leftZ1->movementCostRight);
					costInt = std::max(costInt, rightZ1->movementCostRight);

					// If going down must check each tile above our head
					if (goingDown)
					{
						if (leftZ1->solidGround
							|| leftZ1->solidGround
							|| map.getTile(toPos.x - 1, toPos.y - 1, z + 1)->solidGround
							|| map.getTile(toPos.x, toPos.y - 1, z + 1)->solidGround)
						{
							return false;
						}
					}
				}
				// Moving up-down
				else if (goingDown)
				{
					// Cannot descend if on solid ground
					if (from->solidGround
						|| map.getTile(Vec3<int>{fromPos.x - 1, fromPos.y, fromPos.z})->solidGround
						|| map.getTile(Vec3<int>{fromPos.x, fromPos.y - 1, fromPos.z})->solidGround
						|| map.getTile(Vec3<int>{fromPos.x - 1, fromPos.y - 1, fromPos.z})->solidGround
						)
					{
						return false;
					}
				}
			}
		}
		// Small units
		else
		{
			// If moving diagonally
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
					
				Tile* topRight = map.getTile(std::max(fromPos.x, toPos.x), std::min(fromPos.y, toPos.y), z);
				Tile* bottomLeft = map.getTile(std::min(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);
				Tile* bottomRight = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);

				costInt = std::max(costInt, topRight->movementCostLeft);
				costInt = std::max(costInt, bottomLeft->movementCostRight);
				costInt = std::max(costInt, bottomRight->movementCostLeft);
				costInt = std::max(costInt, bottomRight->movementCostRight);

				if (goingDown)
				{
					// We cannot have solid ground in any of the three tiles besides ours
					if (!(toPos.x > fromPos.x && toPos.y > fromPos.y)
						&& map.getTile(std::min(fromPos.x, toPos.x), std::min(fromPos.y, toPos.y), z)->solidGround)
					{
						return false;
					}
					if (!(toPos.x < fromPos.x && toPos.y > fromPos.y) && topRight->solidGround)
					{
						return false;
					}
					if (!(toPos.x > fromPos.x && toPos.y < fromPos.y) && bottomLeft->solidGround)
					{
						return false;
					}
					if (!(toPos.x < fromPos.x && toPos.y < fromPos.y) && bottomRight->solidGround)
					{
						return false;
					}
				}
			}
			// If moving linearly
			else
			{
				Tile* bottomRight = map.getTile(std::max(fromPos.x, toPos.x), std::max(fromPos.y, toPos.y), z);

				if (fromPos.x != toPos.x)
				{
					costInt = std::max(costInt, bottomRight->movementCostLeft);
					
					// Cannot go down if above target tile is solid ground
					if (goingDown && map.getTile(toPos.x, toPos.y, toPos.z + 1)->solidGround)
					{
						return false;
					}
				}
				else if (fromPos.y != toPos.y)
				{
					costInt = std::max(costInt, bottomRight->movementCostRight);
					
					// Cannot go down if above target tile is solid ground
					if (goingDown && map.getTile(toPos.x, toPos.y, toPos.z + 1)->solidGround)
					{
						return false;
					}
				}
				else if (goingDown)
				{
					// Cannot descend if on solid ground
					if (from->solidGround)
					{
						return false;
					}
				}
			}
		}
		if (costInt == 255)
		{
			return false;
		}

		// It costs 1x to move to adjacent tile, 1.5x to move diagonally, 
		// 2x to move diagonally to another layer.
		// Also, it costs 0x to fall, but we have checked for that above
		cost = costInt;
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
		// This cost is in TUs. We should convert it to cost in tiles
		cost /= 4.0f;
        return true;
	}
};

BattleUnitMission *BattleUnitMission::gotoLocation(BattleUnit &u, Vec3<int> target, int facingDelta)
{
	auto t = u.tileObject->getOwningTile()->map.getTile(target);
	if (!t->getPassable(u.agent->type->large) && target.z < u.tileObject->getOwningTile()->map.size.z - 1)
	{
		LogInfo("Tried moving to impassable %d %d %d, incrementing once", target.x, target.y, target.z);
		target.z++;
	}

	// Check if target tile is valid
	while (!u.canFly())
	{
		if (!t->getPassable(u.agent->type->large))
		{
			LogInfo("Cannot move to %d %d %d, impassable", target.x, target.y, target.z);
			return restartNextMission(u);
		}
		if (t->getCanStand(u.agent->type->large))
		{
			break;
		}
		target.z--;
		if (target.z == -1)
		{
			LogError("Solid ground missing on level 0? Reached %d %d %d", target.x, target.y, target.z);
			return restartNextMission(u);
		}
		t = u.tileObject->getOwningTile()->map.getTile(target);
	}
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::GotoLocation;
	mission->targetLocation = target;
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

BattleUnitMission *BattleUnitMission::throwItem(BattleUnit &u, sp<AEquipment> item)
{
	auto *mission = new BattleUnitMission();
	mission->type = MissionType::ThrowItem;
	mission->item = item;
	return mission;
}

BattleUnitMission *BattleUnitMission::turn(BattleUnit &u, Vec3<int> target)
{
	return turn(u, u.tileObject->getOwningTile()->position, target, true);
}

BattleUnitMission *BattleUnitMission::turn(BattleUnit &u, Vec3<float> target)
{
	return turn(u, u.position, target, false);
}

BattleUnitMission *BattleUnitMission::turn(BattleUnit &u, Vec3<float> from, Vec3<float> to, bool requireGoal)
{
	static const std::list<Vec3<float>> angles = {
		{ 0, -1, 0 },
		{ 1, -1, 0 },
		{ 1, 0, 0 },
		{ 1, 1, 0 },
		{ 0, 1, 0 },
		{ -1, 1, 0 },
		{ -1, 0, 0 },
		{ -1, -1, 0 },
	};

	auto *mission = new BattleUnitMission();
	mission->type = MissionType::Turn;
	mission->requireGoal = requireGoal;
	float closestAngle = FLT_MAX;
	Vec3<int> closestVector = { 0,0,0 };
	Vec3<float> targetFacing = (Vec3<float>)(to - from);
	if (targetFacing.x == 0.0f && targetFacing.y == 0.0f)
	{
		return restartNextMission(u);
	}
	else
	{
		targetFacing.z = 0;
		for (auto &a : angles)
		{
			float angle =
				glm::angle(glm::normalize(targetFacing), glm::normalize(a));
			if (angle < closestAngle && u.agent->isFacingAllowed(Vec2<int>{a.x, a.y}))
			{
				closestAngle = angle;
				closestVector = a;
			}
		}
	}
	mission->targetFacing = { closestVector.x, closestVector.y };
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

bool BattleUnitMission::spendAgentTUs(GameState &state, BattleUnit &u, int cost)
{
	if (cost > u.agent->modified_stats.time_units)
	{
		u.missions.emplace_front(acquireTU(u, cost));
		u.missions.front()->start(state, u);
		return false;
	}
	u.agent->modified_stats.time_units -= cost;
	return true;
}

bool BattleUnitMission::getNextDestination(GameState &state, BattleUnit &u, Vec3<float> &dest)
{
    switch (this->type)
    {
	case MissionType::AcquireTU:
	{
		return false;
	}
	case MissionType::ReachGoal:
	{
		return false;
	}
	case MissionType::Turn:
	{
		return false;
	}
    case MissionType::GotoLocation: 
    {
        return advanceAlongPath(state, dest, u);
    }
	case MissionType::RestartNextMission:
	{
		return false;
	}
	case MissionType::Snooze:
    {
        return false;
    }
	case MissionType::ChangeBodyState:
	{
		return false;
	}
	case MissionType::ThrowItem:
	{
		return false;
	}
	case MissionType::Fall:
	{
		return false;
	}
	default:
        LogWarning("TODO: Implement");
        return false;
    }
    return false;
}

void BattleUnitMission::update(GameState &state, BattleUnit &u, unsigned int ticks)
{
    switch (this->type)
    {
	case MissionType::AcquireTU:
	{
		return;
	}
	case MissionType::GotoLocation:
	{  
		// Update movement speed if we're already moving
		if (u.current_movement_state != AgentType::MovementState::None)
			startMoving(u);
		return;
	}
	case MissionType::RestartNextMission:
	{
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
	case MissionType::ChangeBodyState:
	{
		return;
	}
	case MissionType::ThrowItem:
	{
		return;
	}
	case MissionType::Fall:
	{
		return;
	}
	case MissionType::ReachGoal:
	{
		return;
	}
	case MissionType::Turn:
	{
		return;
	}
    default:
        LogWarning("TODO: Implement");
        return;
    }
}

bool BattleUnitMission::isFinished(GameState &state, BattleUnit &u)
{
    switch (this->type)
    {
	case MissionType::AcquireTU:
		return u.agent->modified_stats.time_units >= timeUnits;
	case MissionType::ReachGoal:
		if (u.atGoal)
		{
			if (u.current_movement_state != AgentType::MovementState::None)
			{
				u.setMovementState(AgentType::MovementState::None);
			}
			return true;
		}
		return false;
    case MissionType::GotoLocation:
		// If finished moving - stop moving
		if (this->currentPlannedPath.empty() || u.isDead())
		{
			if (u.current_movement_state != AgentType::MovementState::None)
			{
				u.setMovementState(AgentType::MovementState::None);
			}
			return true;
		}
		return false;
	case MissionType::RestartNextMission:
		return true;
	case MissionType::Snooze:
        return this->timeToSnooze == 0;
	case MissionType::ChangeBodyState:
		return u.current_body_state == this->bodyState;
	case MissionType::ThrowItem:
		// If died or went unconscious while throwing - drop thrown item
		if (!u.isConscious() && item)
		{
			// FIXME: Drop item properly
			auto bi = mksp<BattleItem>();
			bi->battle = u.battle;
			bi->item = item;
			item = nullptr;
			bi->position = u.position + Vec3<float>{0.0, 0.0, 0.5f};
			bi->strategy_icon_list = state.battle_strategy_icon_list;
			bi->supported = false;
			u.battle.lock()->items.push_back(bi);
			u.tileObject->map.addObjectToMap(bi);
			return true;
		}
		return (u.current_body_state == AgentType::BodyState::Standing && !item);
	case MissionType::Turn:
		if (u.facing == targetFacing || u.isDead())
		{
			return true;
		}
		// Hack to make unit try to turn again
		if (u.turning_animation_ticks_remaining == 0)
		{
			u.missions.emplace_front(restartNextMission(u));
			u.missions.front()->start(state, u);
		}
		return false;
	case MissionType::Fall:
		return !u.falling;
	default:
        LogWarning("TODO: Implement");
        return false;
    }
}

void BattleUnitMission::start(GameState &state, BattleUnit &u)
{
	LogWarning("Unit mission \"%s\" starting",
		getName().cStr());

    switch (this->type)
    {
	case MissionType::Turn:
	{
		// Go to goal first
		if (!u.atGoal && requireGoal)
		{
			u.missions.emplace_front(reachGoal(u));
			u.missions.front()->start(state, u);
			return;
		}
		// If we need to turn, do we need to change stance first?
		if (u.current_body_state == AgentType::BodyState::Prone)
		{
			u.missions.emplace_front(changeStance(u, AgentType::BodyState::Kneeling));
			u.missions.front()->start(state, u);
			return;
		}
		// Calculate and spend cost
		int cost = 0;
		if (targetFacing != u.facing)
		{
			cost = 1;
		}
		if (!spendAgentTUs(state, u, cost))
		{
			LogWarning("Unit mission \"%s\" could not start: unsufficient TUs",
				getName().cStr());
			return;
		}
		Vec2<int> dest;
		if (getNextFacing(state, u, dest))
		{
			u.goalFacing = dest;
			u.turning_animation_ticks_remaining = TICKS_PER_FRAME;
		}
		return;
	}
	case MissionType::ChangeBodyState:
	{
		bool instant = false;
		if (u.agent->getAnimationPack()->getFrameCountBody(u.agent->getItemInHands(),
			u.target_body_state, bodyState,
			u.current_hand_state, u.current_movement_state, u.facing) == 0)
		{
			if (u.agent->getAnimationPack()->getFrameCountBody(u.agent->getItemInHands(),
				u.target_body_state, bodyState,
				u.current_hand_state, AgentType::MovementState::None, u.facing) != 0)
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
		if (bodyState == AgentType::BodyState::Flying && u.current_body_state != AgentType::BodyState::Standing)
		{
			u.missions.emplace_front(changeStance(u, AgentType::BodyState::Standing));
			u.missions.front()->start(state, u);
			return;
		}
		// If trying to stop flying stand up first
		if (bodyState != AgentType::BodyState::Standing && u.current_body_state == AgentType::BodyState::Flying
			&& u.agent->isBodyStateAllowed(AgentType::BodyState::Standing))
		{
			u.missions.emplace_front(changeStance(u, AgentType::BodyState::Standing));
			u.missions.front()->start(state, u);
			return;
		}
		// If trying to stand up from prone go kneel first
		if (bodyState != AgentType::BodyState::Kneeling && u.current_body_state == AgentType::BodyState::Prone
			&& u.agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
		{
			u.missions.emplace_front(changeStance(u, AgentType::BodyState::Kneeling));
			u.missions.front()->start(state, u);
			return;
		}
		// If trying to go prone from not kneeling then kneel first
		if (bodyState == AgentType::BodyState::Prone && u.current_body_state != AgentType::BodyState::Kneeling
			&& u.agent->isBodyStateAllowed(AgentType::BodyState::Kneeling))
		{
			u.missions.emplace_front(changeStance(u, AgentType::BodyState::Kneeling));
			u.missions.front()->start(state, u);
			return;
		}
		if (bodyState == u.target_body_state)
		{
			LogError("WTF, trying to change to the body state we're already in? Should this ever happen?");
			return;
		}
		// Calculate and spend cost
		int cost = 0;
		switch (bodyState)
		{
		case  AgentType::BodyState::Flying:
		case  AgentType::BodyState::Standing:
			if (u.target_body_state == AgentType::BodyState::Prone
				|| u.target_body_state == AgentType::BodyState::Kneeling)
				cost = 8;
			break;
		case  AgentType::BodyState::Kneeling:
			if (u.target_body_state == AgentType::BodyState::Prone
				|| u.target_body_state == AgentType::BodyState::Standing
				|| u.target_body_state == AgentType::BodyState::Flying)
				cost = 8;
			break;
		case  AgentType::BodyState::Prone:
			if (u.target_body_state == AgentType::BodyState::Kneeling
				|| u.target_body_state == AgentType::BodyState::Standing
				|| u.target_body_state == AgentType::BodyState::Flying)
				cost = 8;
			break;
		}
		if (!spendAgentTUs(state, u, cost))
		{
			LogWarning("Unit mission \"%s\" could not start: unsufficient TUs",
				getName().cStr());
			return;
		}
		// Change state actually
		if (instant)
		{
			u.setBodyState(bodyState);
		}
		else
		{
			u.setBodyState(u.target_body_state);
			u.target_body_state = bodyState;
			u.body_animation_ticks_remaining =
				u.agent->getAnimationPack()->getFrameCountBody(u.agent->getItemInHands(),
					u.current_body_state, bodyState,
					u.current_hand_state, u.current_movement_state, u.facing)
				* TICKS_PER_FRAME;
		}
		return;
	}
	case MissionType::ThrowItem:
		// Half way there - throw the item!
		if (u.current_body_state == AgentType::BodyState::Throwing)
		{
			// FIXME: Play throwing sound
			// FIXME: Throw item properly
			auto bi = mksp<BattleItem>();
			bi->battle = u.battle;
			bi->item = item;
			item = nullptr;
			bi->position = u.position + Vec3<float>{0.0, 0.0, 0.5f};
			bi->velocity = ((Vec3<float>)targetLocation - bi->position) / 2.0f + Vec3<float>{0.0, 0.0, 3.0f};
			bi->strategy_icon_list = state.battle_strategy_icon_list;
			bi->supported = false;
			u.battle.lock()->items.push_back(bi);
			u.tileObject->map.addObjectToMap(bi);

			u.missions.emplace_front(changeStance(u, AgentType::BodyState::Standing));
			u.missions.front()->start(state, u);
			return;
		} 
		// Just starting the throw
		else if (item)
		{
			if (u.current_body_state != AgentType::BodyState::Standing)
			{
				// FIXME: actually read the option
				bool USER_OPTION_ALLOW_INSTANT_THROWS = false;
				if (!USER_OPTION_ALLOW_INSTANT_THROWS)
				{
					u.missions.emplace_front(BattleUnitMission::changeStance(u, AgentType::BodyState::Standing));
					u.missions.front()->start(state, u);
					return;
				}
				u.current_body_state = AgentType::BodyState::Standing;
			}
			// Calculate cost
			// I *think* this is correct? 18 TUs at 100 time units
			int cost = 1800 / u.agent->current_stats.time_units;
			if (!spendAgentTUs(state, u, cost))
			{
				LogWarning("Unit mission \"%s\" could not start: unsufficient TUs",
					getName().cStr());
				return;
			}
			// Start throw animation
			u.missions.emplace_front(BattleUnitMission::changeStance(u, AgentType::BodyState::Throwing));
			u.missions.front()->start(state, u);
			return;
		}
		// Throw finished - nothing else to do
		else
		{
			// Nothing else to do
		}
		return;
	case MissionType::GotoLocation:
		// If we have already tried to use this mission, see if path is still valid
		if (currentPlannedPath.size() > 0)
		{
			auto t = u.tileObject->getOwningTile();
			auto pos = *currentPlannedPath.begin();
			// If we're not far enough and can enter first tile in path
			if (t->position == pos)
			{
				// Start wtith first position
			}
			else if (std::abs(t->position.x - pos.x) <= 1
				&& std::abs(t->position.y - pos.y) <= 1
				&& std::abs(t->position.z - pos.z) <= 1
				&& BattleUnitTileHelper{ t->map, u }.canEnterTile(t, t->map.getTile(pos)))
			{
				// Start with our position
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
		// Starting GotoLocation mission never costs TU's, getting next path node does
		return;
	case MissionType::ReachGoal:
		// Reaching goal means we already requested this node and paid the price, so no TU cost
		startMoving(u);
		return;
	case MissionType::Fall:
		// Falling is free :)))
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
		if (!unitTile->getOwningTile()->map.getTile(target)->getPassable(u.agent->type->large))
		{
			return;
		}
        auto &map = unitTile->map;
        // FIXME: Change findShortestPath to return Vec3<int> positions?
        auto path =
            map.findShortestPath(u.goalPosition, target, maxIterations,
                BattleUnitTileHelper {map, u});

        // Always start with the current position
        this->currentPlannedPath.push_back(u.goalPosition);
        for (auto *t : path)
        {
            this->currentPlannedPath.push_back(t->position);
        }
		targetLocation = currentPlannedPath.back();
    }
    else
    {
        LogError("Mission %s: Unit without tileobject attempted pathfinding!", this->getName().cStr());
    }
}

bool BattleUnitMission::advanceAlongPath(GameState &state, Vec3<float> &dest, BattleUnit &u)
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
	if (tFrom->position != pos
		&&( std::abs(tFrom->position.x - pos.x) > 1
		|| std::abs(tFrom->position.y - pos.y) > 1
		|| std::abs(tFrom->position.z - pos.z) > 1
		|| !BattleUnitTileHelper{ tFrom->map, u }.canEnterTile(tFrom, tTo, cost)))
	{
		// Next tile became impassable, snooze for a moment and then pick a new path
		currentPlannedPath.clear();
		u.missions.emplace_front(restartNextMission(u));
		u.missions.front()->start(state, u);
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
	while (it != currentPlannedPath.end() &&  (tFrom->position == *it ||
		(std::abs(tFrom->position.x - it->x) <= 1
			&& std::abs(tFrom->position.y - it->y) <= 1
			&& std::abs(tFrom->position.z - it->z) <= 1
			&& BattleUnitTileHelper{ tFrom->map, u }.canEnterTile(tFrom, tFrom->map.getTile(*it)))))
	{
		currentPlannedPath.pop_front();
		poppedPos = pos;
		pos = currentPlannedPath.front();
		tTo = tFrom->map.getTile(pos);
		it = ++currentPlannedPath.begin();
	}

	// Do we need to turn?
	auto m = turn(u, pos);
	if (!m->isFinished(state, u))
	{
		u.missions.emplace_front(m);
		u.missions.front()->start(state, u);
		return false;
	}

	// Do we need to change stance?
	switch (u.movement_mode)
	{
		// If want to move prone but not prone - go prone
		case BattleUnit::MovementMode::Prone:
			if (u.current_body_state != AgentType::BodyState::Prone)
			{
				u.missions.emplace_front(changeStance(u, AgentType::BodyState::Prone));
				u.missions.front()->start(state, u);
				return false;
			}
			break;
		// If want to move standing up but not standing/flying - go standing/flying where appropriate
		case BattleUnit::MovementMode::Walking:
		case BattleUnit::MovementMode::Running:
			auto t = u.tileObject->getOwningTile();
			if (u.current_body_state != AgentType::BodyState::Standing && u.current_body_state != AgentType::BodyState::Flying)
			{
				if (!t->canStand && !t->map.getTile(pos)->canStand)
				{
					u.missions.emplace_front(changeStance(u, AgentType::BodyState::Flying));
					u.missions.front()->start(state, u);
				}
				else
				{
					u.missions.emplace_front(changeStance(u, AgentType::BodyState::Standing));
					u.missions.front()->start(state, u);
				}
				return false;
			}
			// Stop flying if we no longer require it
			if (u.current_body_state == AgentType::BodyState::Flying
				&& t->canStand && t->map.getTile(pos)->canStand)
			{
				u.missions.emplace_front(changeStance(u, AgentType::BodyState::Standing));
				u.missions.front()->start(state, u);
				return false;
			}
			// Start flying if we require it
			if (u.current_body_state != AgentType::BodyState::Flying
				&& !(t->canStand) || !(t->map.getTile(pos)->canStand)
				&& u.canFly())
			{
				u.missions.emplace_front(changeStance(u, AgentType::BodyState::Flying));
				u.missions.front()->start(state, u);
				return false;
			}
			break;
	}

	// Is there a unit in the target tile other than us?  
	if (tTo->getUnitIfPresent(true, u.tileObject))
	{
		u.current_movement_state = AgentType::MovementState::None;
		// Snooze for a second and try again
		u.missions.emplace_front(snooze(u, 60));
		u.missions.front()->start(state, u);
		return false;
	}

	// Running decreaes cost
	// FIXME: Handle strafing and going backwards influencing TU spent
	if (u.agent->canRun() && u.movement_mode == BattleUnit::MovementMode::Running)
	{
		cost /= 2;
	}
	// See if we can afford doing this move
	if (!spendAgentTUs(state, u, cost))
	{
		currentPlannedPath.push_front(poppedPos);
		return false;
	}
	// FIXME: Deplete stamina according to encumbrance if running
	startMoving(u);

	dest = u.tileObject->map.getTile(pos)->getRestingPosition(u.agent->type->large);
    return true;
}

void BattleUnitMission::startMoving(BattleUnit &u)
{
	// FIXME: Account for different movement ways (strafing, backwards etc.)
	if (u.movement_mode == BattleUnit::MovementMode::Running && u.current_body_state != AgentType::BodyState::Prone)
	{
		u.setMovementState(AgentType::MovementState::Running);
	}
	else if (u.current_body_state != AgentType::BodyState::Kneeling && u.current_body_state != AgentType::BodyState::Throwing)
	{
		u.setMovementState(AgentType::MovementState::Normal);
	}
	else
	{
		LogError("Trying to move while kneeling or throwing, wtf?");
	}
}

bool BattleUnitMission::getNextFacing(GameState &state, BattleUnit &u, Vec2<int> &dest)
{
	static const std::map<Vec2<int>, int> facing_dir_map = {
		{ { 0,-1 },	0 },
		{ { 1,-1 },	1 },
		{ { 1,0 },	2 },
		{ { 1,1 },	3 },
		{ { 0,1 },	4 },
		{ { -1,1 },	5 },
		{ { -1,0 },	6 },
		{ { -1,-1 },7 } };
	static const std::map<int, Vec2<int>> dir_facing_map = {
		{ 0 ,{ 0,-1 } },
		{ 1 ,{ 1,-1 } },
		{ 2 ,{ 1,0  } },
		{ 3 ,{ 1,1  } },
		{ 4 ,{ 0,1  } },
		{ 5 ,{ -1,1 } },
		{ 6 ,{ -1,0 } },
		{ 7 ,{ -1,-1} } };

	if (targetFacing == u.facing)
		return false;

	int curFacing = facing_dir_map.at(u.facing);
	int tarFacing = facing_dir_map.at(targetFacing);
	if (curFacing == tarFacing)
		return false;

	// If we need to turn, do we need to change stance first?
	if (u.current_body_state == AgentType::BodyState::Prone)
	{
		u.missions.emplace_front(changeStance(u, AgentType::BodyState::Kneeling));
		u.missions.front()->start(state, u);
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
	return true;
}
	
UString BattleUnitMission::getName()
{
    UString name = "UNKNOWN";
    switch (this->type)
    {
	case MissionType::AcquireTU:
		name = "AcquireTUs %d" +UString::format(" for %u ticks", this->timeUnits);
		break;
    case MissionType::GotoLocation:
        name = "GotoLocation "  + UString::format(" {%d,%d,%d}", this->targetLocation.x, this->targetLocation.y,
            this->targetLocation.z);
        break;
	case MissionType::RestartNextMission:
		name = "Restart next mission";
		break;
	case MissionType::Snooze:
        name = "Snooze "+ UString::format(" for %u ticks", this->timeToSnooze);
        break;
    case MissionType::ChangeBodyState:
        name = "ChangeBodyState " + UString::format("%d", (int)this->bodyState);
        break;
	case MissionType::ThrowItem:
		name = "ThrowItem " + UString::format("%s", (int)this->item->type->name.cStr());
		break;
	case MissionType::ReachGoal:
		name = "ReachGoal";
		break;
	case MissionType::Turn:
		name = "Turn " + UString::format(" {%d,%d}", this->targetFacing.x, this->targetFacing.y);
		break;
	case MissionType::Fall:
		name = "Fall!...FALL!!";
		break;
    }
    return name;
}

} // namespace OpenApoc
