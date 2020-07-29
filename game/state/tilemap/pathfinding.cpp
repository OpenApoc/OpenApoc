#include "game/state/battle/battle.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/city/city.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/city/vehiclemission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlemap.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include "limits.h"
#include <algorithm>
#include <glm/glm.hpp>

// Show debug pathfinding output
//#define PATHFINDING_DEBUG

namespace OpenApoc
{

namespace
{
class PathNode
{
  public:
	PathNode(float costToGetHere, float trueCost, float distanceToGoal, PathNode *parentNode,
	         Tile *thisTile)
	    : costToGetHere(costToGetHere), trueCost(trueCost), parentNode(parentNode),
	      thisTile(thisTile), distanceToGoal(distanceToGoal)
	{
	}

	std::list<Vec3<int>> getPathToNode()
	{
		std::list<Vec3<int>> path;
		path.push_back(thisTile->position);
		PathNode *t = parentNode;
		while (t)
		{
			path.push_front(t->thisTile->position);
			t = t->parentNode;
		}

		return path;
	}

	float costToGetHere;
	float trueCost;
	PathNode *parentNode;
	Tile *thisTile;
	float distanceToGoal;
};

class LosNode
{
  public:
	LosNode(float costToGetHere, float distanceToGoal, LosNode *parentNode, int block)
	    : costToGetHere(costToGetHere), parentNode(parentNode), block(block),
	      distanceToGoal(distanceToGoal)
	{
	}

	std::list<int> getPathToNode()
	{
		std::list<int> path;
		path.push_back(block);
		LosNode *t = parentNode;
		while (t)
		{
			path.push_front(t->block);
			t = t->parentNode;
		}

		return path;
	}

	float costToGetHere;
	LosNode *parentNode;
	int block;
	float distanceToGoal;
};

class RoadSegmentNode
{
  public:
	RoadSegmentNode(float costToGetHere, float distanceToGoal, RoadSegmentNode *parentNode,
	                int segment)
	    : costToGetHere(costToGetHere), parentNode(parentNode), segment(segment),
	      distanceToGoal(distanceToGoal)
	{
	}

	std::list<int> getPathToNode()
	{
		std::list<int> path;
		path.push_back(segment);
		RoadSegmentNode *t = parentNode;
		while (t)
		{
			path.push_front(t->segment);
			t = t->parentNode;
		}

		return path;
	}

	float costToGetHere;
	RoadSegmentNode *parentNode;
	int segment;
	float distanceToGoal;
};

Vec3<int> rotate(Vec3<int> vec, int rotation)
{
	switch (rotation)
	{
		case 1:
			return {-vec.y, vec.x, vec.z};
		case 2:
			return {-vec.x, -vec.y, vec.z};
		case 3:
			return {vec.y, -vec.x, vec.z};
		default:
			return vec;
	}
}

} // anonymous namespace

std::list<Vec3<int>> TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destinationStart,
                                               Vec3<int> destinationEnd, int iterationLimit,
                                               const CanEnterTileHelper &canEnterTileHelper,
                                               bool approachOnly, bool ignoreStaticUnits,
                                               bool ignoreMovingUnits, bool ignoreAllUnits,
                                               float *cost, float maxCost)
{
#ifdef PATHFINDING_DEBUG
	for (auto &t : tiles)
		t.pathfindingDebugFlag = false;
#endif

	maxCost /= canEnterTileHelper.pathOverheadAlloawnce();
	// Faster than looking up in a set
	std::vector<bool> visitedTiles = std::vector<bool>(size.x * size.y * size.z, false);
	int strideZ = size.x * size.y;
	int strideY = size.x;
	std::list<PathNode *> nodesToDelete;
	std::list<PathNode *> fringe;
	Vec3<float> goalPositionStart;
	Vec3<float> goalPositionEnd;
	bool destinationIsSingleTile = destinationStart == destinationEnd - Vec3<int>{1, 1, 1};
	int iterationCount = 0;

	// Approach Only makes no sense with pathing into a block, but we'll fix it anyway
	if (approachOnly && !destinationIsSingleTile)
	{
		LogWarning("Trying to route from %s to %s-%s in approachOnly mode? Extending destination's "
		           "xy boundaries by 1.",
		           origin, destinationStart, destinationEnd);
		approachOnly = false;
		destinationStart -=
		    Vec3<int>(destinationStart.x > 0 ? 1 : 0, destinationStart.y > 0 ? 1 : 0, 0);
		destinationEnd +=
		    Vec3<int>(destinationEnd.x < size.x ? 1 : 0, destinationEnd.y < size.y ? 1 : 0, 0);
	}

	if (destinationIsSingleTile)
	{
		LogInfo("Trying to route from %s to %s", origin, destinationStart);
	}
	else
	{
		LogInfo("Trying to route from %s to %s-%s", origin, destinationStart, destinationEnd);
	}

	if (!tileIsValid(origin))
	{
		LogError("Bad origin %s", origin);
		return {};
	}
	if (!tileIsValid(destinationStart))
	{
		LogError("Bad destinationStart %s", destinationStart);
		return {};
	}
	if (destinationEnd.x <= destinationStart.x || destinationEnd.x > this->size.x ||
	    destinationEnd.y <= destinationStart.y || destinationEnd.y > this->size.y ||
	    destinationEnd.z <= destinationStart.z || destinationEnd.z > this->size.z)
	{
		LogError("Bad destinationEnd %s", destinationEnd);
		return {};
	}

	goalPositionStart = destinationStart;
	goalPositionEnd = destinationEnd;

	Tile *startTile = this->getTile(origin);
	if (!startTile)
	{
		LogError("Failed to get origin tile at %s", origin);
		return {};
	}

	if (origin.x >= destinationStart.x && origin.x < destinationEnd.x &&
	    origin.y >= destinationStart.y && origin.y < destinationEnd.y &&
	    origin.z >= destinationStart.z && origin.z < destinationEnd.z)
	{
		LogInfo("Origin is within destination!");
		return {startTile->position};
	}

	auto startNode = new PathNode(
	    0.0f, 0.0f, canEnterTileHelper.getDistance(origin, goalPositionStart, goalPositionEnd),
	    nullptr, startTile);
	nodesToDelete.push_back(startNode);
	fringe.emplace_back(startNode);

	auto closestNodeSoFar = *fringe.begin();

	while (iterationCount++ < iterationLimit)
	{
		auto first = fringe.begin();
		if (first == fringe.end())
		{
			LogInfo("No more tiles to expand after %d iterations", iterationCount);
			break;
		}
		auto nodeToExpand = *first;
		fringe.erase(first);

		// Skip if we've already expanded this, as in a 3d-grid we know the first
		// expansion will be the shortest route
		if (visitedTiles[nodeToExpand->thisTile->position.z * strideZ +
		                 nodeToExpand->thisTile->position.y * strideY +
		                 nodeToExpand->thisTile->position.x])
		{
			iterationCount--;
			continue;
		}
		visitedTiles[nodeToExpand->thisTile->position.z * strideZ +
		             nodeToExpand->thisTile->position.y * strideY +
		             nodeToExpand->thisTile->position.x] = true;

#ifdef PATHFINDING_DEBUG
		LogInfo("EXPAND %s", nodeToExpand->thisTile->position);
		nodeToExpand->thisTile->pathfindingDebugFlag = true;
#endif

		// Make it so we always try to move at least one tile
		if (closestNodeSoFar->parentNode == nullptr)
			closestNodeSoFar = nodeToExpand;

		if (nodeToExpand->distanceToGoal == 0 ||
		    (approachOnly && nodeToExpand->thisTile->position.z == goalPositionStart.z &&
		     std::max(std::abs(nodeToExpand->thisTile->position.x - goalPositionStart.x),
		              std::abs(nodeToExpand->thisTile->position.y - goalPositionStart.y)) <= 1))
		{
			closestNodeSoFar = nodeToExpand;
			break;
		}
		else if (nodeToExpand->distanceToGoal < closestNodeSoFar->distanceToGoal)
		{
			closestNodeSoFar = nodeToExpand;
		}
		Vec3<int> currentPosition = nodeToExpand->thisTile->position;
		for (int z = -1; z <= 1; z++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					if (x == 0 && y == 0 && z == 0)
					{
						continue;
					}
					auto nextPosition = currentPosition;
					nextPosition.x += x;
					nextPosition.y += y;
					nextPosition.z += z;
					if (!tileIsValid(nextPosition))
					{
						continue;
					}
					if (visitedTiles[nextPosition.z * strideZ + nextPosition.y * strideY +
					                 nextPosition.x])
					{
						continue;
					}
					Tile *tile = this->getTile(nextPosition);
					float thisCost = 0.0f;
					bool unused = false;
					bool jumped = false;
					if (!canEnterTileHelper.canEnterTile(nodeToExpand->thisTile, tile, true, jumped,
					                                     thisCost, unused, ignoreStaticUnits,
					                                     ignoreMovingUnits, ignoreAllUnits))
						continue;
					// Jumped flag set, must immediately land
					if (jumped)
					{
						auto nextNextPosition = nextPosition + Vec3<int>{x, y, 0};
						if (!tileIsValid(nextNextPosition))
						{
							continue;
						}
						auto nextTile = this->getTile(nextNextPosition);
						if (!canEnterTileHelper.canEnterTile(tile, nextTile, false, jumped,
						                                     thisCost, unused, ignoreStaticUnits,
						                                     ignoreMovingUnits, ignoreAllUnits))
						{
							continue;
						}
						// Jump success, replace values
						nextPosition = nextNextPosition;
						tile = nextTile;
					}
					float newNodeCost = nodeToExpand->costToGetHere;
					float newTrueCost = nodeToExpand->trueCost;

					newNodeCost += thisCost /* * (jumped ? 2 : 1) */
					               / canEnterTileHelper.pathOverheadAlloawnce();
					newTrueCost += thisCost;

					// make pathfinder biased towards vehicle's altitude preference
					newNodeCost += canEnterTileHelper.adjustCost(nextPosition, z);

					// Do not add to the fringe if too far
					if (maxCost != 0.0f && newNodeCost >= maxCost)
						continue;

					auto newNode = new PathNode(
					    newNodeCost, newTrueCost,
					    destinationIsSingleTile
					        ? canEnterTileHelper.getDistance(nextPosition, goalPositionStart)
					        : canEnterTileHelper.getDistance(nextPosition, goalPositionStart,
					                                         goalPositionEnd),
					    nodeToExpand, tile);
					nodesToDelete.push_back(newNode);

#ifdef PATHFINDING_DEBUG
					LogInfo("NEW ND %s [%f, %f]", nextPosition, newNode->costToGetHere,
					        newNode->distanceToGoal);
#endif

					// Put node at appropriate place in the list
					auto it = fringe.begin();
					while (it != fringe.end() &&
					       ((*it)->costToGetHere + (*it)->distanceToGoal) <
					           (newNode->costToGetHere + newNode->distanceToGoal))
						it++;
					fringe.emplace(it, newNode);
				}
			}
		}
	}
	if (iterationCount > iterationLimit)
	{
		if (approachOnly && closestNodeSoFar->thisTile->position.z == goalPositionStart.z &&
		    std::max(std::abs(closestNodeSoFar->thisTile->position.x - goalPositionStart.x),
		             std::abs(closestNodeSoFar->thisTile->position.y - goalPositionStart.y)) <= 1)
		{
			// Nothing?
		}
		else if (maxCost > 0.0f)
		{
			LogInfo("No route from %s to %s-%s found after %d iterations, returning "
			        "closest path %s",
			        origin, destinationStart, destinationEnd, iterationCount,
			        closestNodeSoFar->thisTile->position);
		}
		else
		{
			LogInfo("No route from %s to %s-%s found after %d iterations, returning "
			        "closest path %s",
			        origin, destinationStart, destinationEnd, iterationCount,
			        closestNodeSoFar->thisTile->position);
		}
	}
	else if (closestNodeSoFar->distanceToGoal > 0)
	{
		if (maxCost > 0.0f)
		{
			LogInfo("Could not find path within maxPath, returning closest path ending at %d",
			        closestNodeSoFar->thisTile->position.x);
		}
		else
		{
			LogInfo("Surprisingly, no nodes to expand! Closest path ends at %s",
			        closestNodeSoFar->thisTile->position);
		}
	}
	/*else
	{
	    LogInfo("Path of length %d found in %d iterations", (int)(closestNodeSoFar->costToGetHere *
	canEnterTile.pathOverheadAlloawnce() / 4.0f), iterationCount);
	}*/

	auto result = closestNodeSoFar->getPathToNode();
	if (cost)
	{
		*cost = closestNodeSoFar->trueCost;
	}

	for (auto &p : nodesToDelete)
	{
		delete p;
	}

	return result;
}

std::list<Vec3<int>> Battle::findShortestPath(Vec3<int> origin, Vec3<int> destination,
                                              const BattleUnitTileHelper &canEnterTile,
                                              bool approachOnly, bool ignoreStaticUnits,
                                              bool ignoreMovingUnits, int iterationLimitDirect,
                                              bool forceDirect, bool ignoreAllUnits, float *cost,
                                              float maxCost)
{
	// Maximum distance, in tiles, that will result in trying the direct pathfinding first
	// Otherwise, we start with pathfinding using LOS blocks immediately
	static const int MAX_DISTANCE_TO_PATHFIND_DIRECTLY = 20;

	// How much attempts are given to the pathfinding until giving up and concluding that
	// there is no simple path between orig and dest. This is a multiplier for "distance", which is
	// a minimum number of iterations required to pathfind between two locations
	static const int PATH_ITERATION_LIMIT_MULTIPLIER = 2;

	LogInfo("Trying to route (battle) from %s to %s", origin, destination);

	if (!map->tileIsValid(origin))
	{
		LogError("Bad origin %s", origin);
		return {};
	}
	if (!map->tileIsValid(destination))
	{
		LogError("Bad destination %s", destination);
		return {};
	}

	// Try to pathfind directly if close enough
	int distance = canEnterTile.getDistance(origin, destination) / 4.0f;
	std::list<Vec3<int>> result;
	if (forceDirect || distance < MAX_DISTANCE_TO_PATHFIND_DIRECTLY)
	{
		result = map->findShortestPath(origin, destination,
		                               iterationLimitDirect > 0
		                                   ? iterationLimitDirect
		                                   : distance * PATH_ITERATION_LIMIT_MULTIPLIER,
		                               canEnterTile, approachOnly, ignoreStaticUnits,
		                               ignoreMovingUnits, ignoreAllUnits, cost, maxCost);
		auto finalTile = result.back();
		if (forceDirect || finalTile == destination ||
		    (approachOnly &&
		     std::max(std::abs(finalTile.x - destination.x),
		              std::abs(finalTile.y - destination.y)) <= 1 &&
		     finalTile.z == destination.z))
		{
			return result;
		}
	}

	// If we need to use LOS blocks, a special check is required
	//
	// If we are in "approach" mode, then any adjacent tile is going to be acceptable
	// However, if we are pathing via los blocks, and destination is on an edge?
	//
	// Example: say there's an LB at 3,3,3 to 4,4,4 and we aim at 3,3,3.
	// 2,3,3 would be an adequate goal, however, we would first path via LOS blocks
	// and only then would be try to move towards 3,3,3 and we would never end up in 2,3,3.
	//
	// Therefore, in such cases, we must break this up into different queries

	if (approachOnly)
	{
		auto startLB = losBlocks[getLosBlockID(origin.x, origin.y, origin.z)];
		if (origin.x == startLB->start.x || origin.y == startLB->start.y ||
		    origin.x == startLB->end.x - 1 || origin.y == startLB->end.y - 1)
		{
			// Vector to target, determines order in which we will try things
			auto targetVector = destination - origin;
			// Whether positive or negative tile is in front of destination relative to our position
			int xSign = targetVector.x < 0 ? 1 : -1;
			int ySign = targetVector.y < 0 ? 1 : -1;
			// Which is "front" to us, x or y
			bool xFirst = std::abs(targetVector.x) > std::abs(targetVector.y);
			// Are we approaching straight, or from a diagonal?
			bool diagonalFirst = xFirst ? targetVector.y != 0 : targetVector.x != 0;
			// Idea here is that there are two distinct possibilities
			// - We can approach from an angle				| Target: (100, 50)	| Target (10,50)  |
			// - We can approach directly from a side		|					|			  	  |
			// Based on that, two possible orders happen,	|		0 2 4		|	   2 0 1	  |
			// as illustrated to the right					|		1 + 6		|	   4 + 3	  |
			// Assume we are at (10,10)						|		3 5	7		|	   6 7 5	  |
			std::list<Vec3<int>> possibleDestinations;
			if (diagonalFirst)
			{
				possibleDestinations.push_back({origin.x + xSign, origin.y + ySign, origin.z});
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? xSign : 0), origin.y + (xFirst ? 0 : ySign), origin.z});
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? 0 : xSign), origin.y + (xFirst ? ySign : 0), origin.z});
				possibleDestinations.push_back({origin.x + (xFirst ? xSign : -xSign),
				                                origin.y + (xFirst ? -ySign : ySign), origin.z});
				possibleDestinations.push_back({origin.x + (xFirst ? -xSign : xSign),
				                                origin.y + (xFirst ? ySign : -ySign), origin.z});
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? 0 : -xSign), origin.y + (xFirst ? -ySign : 0), origin.z});
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? -xSign : 0), origin.y + (xFirst ? 0 : -ySign), origin.z});
				possibleDestinations.push_back({origin.x - xSign, origin.y - ySign, origin.z});
			}
			else
			{
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? xSign : 0), origin.y + (xFirst ? 0 : ySign), origin.z});
				possibleDestinations.push_back({origin.x + xSign, origin.y + ySign, origin.z});
				possibleDestinations.push_back({origin.x + (xFirst ? xSign : -xSign),
				                                origin.y + (xFirst ? -ySign : ySign), origin.z});
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? 0 : xSign), origin.y + (xFirst ? ySign : 0), origin.z});
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? 0 : -xSign), origin.y + (xFirst ? -ySign : 0), origin.z});
				possibleDestinations.push_back({origin.x + (xFirst ? -xSign : xSign),
				                                origin.y + (xFirst ? ySign : -ySign), origin.z});
				possibleDestinations.push_back({origin.x - xSign, origin.y - ySign, origin.z});
				possibleDestinations.push_back(
				    {origin.x + (xFirst ? -xSign : 0), origin.y + (xFirst ? 0 : -ySign), origin.z});
			}

			// Store closest result that did not end up adjacent to target
			std::list<Vec3<int>> closestResult;
			float closestDistance = FLT_MAX;
			bool destinationEncountered = false;
			while (!possibleDestinations.empty())
			{
				auto curDest = possibleDestinations.front();
				possibleDestinations.pop_front();
				// Skip tile if does not exist
				if (!map->tileIsValid(curDest))
				{
					continue;
				}
				// Only try to path to target LB once when first encountering a destination inside
				// it
				if (startLB->contains(curDest))
				{
					if (!destinationEncountered)
					{
						destinationEncountered = true;
						result = findShortestPathUsingLB(
						    origin, destination, canEnterTile, approachOnly, ignoreStaticUnits,
						    ignoreMovingUnits, ignoreAllUnits, cost, maxCost);
					}
				}
				// Otherwise path specifically to the destination if it can be entered
				else if (canEnterTile.canEnterTile(nullptr, map->getTile(curDest)))
				{
					result = findShortestPathUsingLB(origin, curDest, canEnterTile, false,
					                                 ignoreStaticUnits, ignoreMovingUnits,
					                                 ignoreAllUnits, cost, maxCost);
				}
				// Process results
				if (result.empty())
				{
					continue;
				}
				// Check if final tile is acceptable
				auto finalTile = result.back();
				if (std::max(std::abs(finalTile.x - destination.x),
				             std::abs(finalTile.y - destination.y)) <= 1 &&
				    finalTile.z == destination.z)
				{
					return result;
				}
				// Store result if it's the closest so far
				auto distance = glm::length((Vec3<float>)(finalTile - destination));
				if (distance < closestDistance)
				{
					closestDistance = distance;
					closestResult = result;
				}
				result.clear();
			}

			return closestResult;
		}
	}

	return findShortestPathUsingLB(origin, destination, canEnterTile, approachOnly,
	                               ignoreStaticUnits, ignoreMovingUnits,

	                               ignoreAllUnits, cost, maxCost);
}

std::list<Vec3<int>> Battle::findShortestPathUsingLB(Vec3<int> origin, Vec3<int> destination,
                                                     const BattleUnitTileHelper &canEnterTile,
                                                     bool approachOnly, bool ignoreStaticUnits,
                                                     bool ignoreMovingUnits, bool ignoreAllUnits,
                                                     float *cost, float maxCost)
{
	// How much attempts are given to the pathfinding until giving up and concluding that
	// there is no simple path between orig and dest. This is a multiplier for "distance", which is
	// a minimum number of iterations required to pathfind between two locations
	static const int PATH_ITERATION_LIMIT_MULTIPLIER = 2;

	// Same as PATH_ITERATION_LIMIT_MULTIPLIER but for when navigating to next los block
	static const int GRAPH_ITERATION_LIMIT_MULTIPLIER = 2;

	// Extra iterations allowed when pathing to a los block, because if we need
	// to find a door we can have a hard time doing so
	static const int GRAPH_ITERATION_LIMIT_EXTRA = 50;

	int distance = canEnterTile.getDistance(origin, destination) / 4.0f;
	std::list<Vec3<int>> result;

	// Pathfind on graphs of los blocks
	int startLB = getLosBlockID(origin.x, origin.y, origin.z);
	int destLB = getLosBlockID(destination.x, destination.y, destination.z);
	auto pathLB = findLosBlockPath(startLB, destLB, canEnterTile.getType());

	// If pathfinding on graphs failed - return short part of the path towards target
	if ((*pathLB.rbegin()) != destLB)
	{
		if (!result.empty())
		{
			return result;
		}
		else
		{
			return map->findShortestPath(
			    origin, destination, distance * PATH_ITERATION_LIMIT_MULTIPLIER, canEnterTile,
			    approachOnly, ignoreStaticUnits, ignoreMovingUnits, ignoreAllUnits, cost, maxCost);
		}
	}

	// Step 01: Pathfind using path among blocks

	result.clear();
	result.push_back(origin);
	if (cost)
	{
		*cost = 0.0f;
	}
	float curMaxCost = maxCost;
	while (!pathLB.empty())
	{
		auto curOrigin = *result.rbegin();
		float curCost = 0.0f;
		auto lbID = pathLB.front();
		pathLB.pop_front();
		auto &lb = *losBlocks[lbID];
		auto distToNext = canEnterTile.getDistance(curOrigin, (lb.start + lb.end) / 2) / 4.0f;

		// Pathfind to next LOS Block
		auto path = map->findShortestPath(curOrigin, lb.start, lb.end,
		                                  distToNext * GRAPH_ITERATION_LIMIT_MULTIPLIER +
		                                      GRAPH_ITERATION_LIMIT_EXTRA,
		                                  canEnterTile, false, ignoreStaticUnits, ignoreMovingUnits,
		                                  ignoreAllUnits, &curCost, curMaxCost);
		// Include new entries into result
		while (!path.empty())
		{
			result.push_back(path.front());
			path.pop_front();
		}
		// Update costs
		if (cost)
		{
			*cost += curCost;
		}
		if (maxCost != 0)
		{
			curMaxCost -= curCost;
		}

		// If we did not reach the block - exit
		if (getLosBlockID(result.back().x, result.back().y, result.back().z) != lbID)
		{
			return result;
		}
	}

	// Step 02: Pathfind to destination

	auto curOrigin = *result.rbegin();
	float curCost = 0.0f;
	auto distToNext = canEnterTile.getDistance(curOrigin, destination) / 4.0f;
	auto path = map->findShortestPath(
	    curOrigin, destination,
	    distToNext * GRAPH_ITERATION_LIMIT_MULTIPLIER + GRAPH_ITERATION_LIMIT_EXTRA, canEnterTile,
	    approachOnly, ignoreStaticUnits, ignoreMovingUnits, ignoreAllUnits, &curCost, curMaxCost);
	// Include new entries into result
	while (!path.empty())
	{
		result.push_back(path.front());
		path.pop_front();
	}
	// Update costs
	if (cost)
	{
		*cost += curCost;
	}

	// Step 03: Profit!

	return result;
}

// FIXME: This can be improved with caching of results, though I am not sure if it would be worth
// it.
//
// The way to improve is as follows:
//
// After we find a path, we know for sure that this path is optimal for all the subpaths in it.
// For example, if optimal path from A to D is A->B->C->D, then the optimal path from B to D
// would certainly be B->C->D, and from A to C will be A->B->C etc. Additionally, since paths
// are bi-directional, we can also be sure that D->B is D->C->B etc.
//
// Knowing this, we can create a 2d vector that tells us, which is the next LB when pathing to an LB
// For example, if we established A to D is A->B->C->D, then we can record, that
// for B, when pathing to C, next is C, when pathing to D, next is C.
//
// We could then use this when pathing. When starting, if the value for origin->dest is filled,
// we already know the path and just return it.
//
// Obviously, when any update of the LOS block pathfinding happens, this would have to be cleared.
std::list<int> Battle::findLosBlockPath(int origin, int destination, BattleUnitType type,
                                        int iterationLimit)
{
	int lbCount = losBlocks.size();
	std::vector<bool> visitedBlocks = std::vector<bool>(lbCount, false);
	std::list<LosNode *> nodesToDelete;
	std::list<LosNode *> fringe;
	int iterationCount = 0;

	LogInfo("Trying to route from lb %d to lb %d", origin, destination);

	if (origin == destination)
	{
		LogInfo("Origin is destination!");
		return {destination};
	}

	if (!blockAvailable[type][origin])
	{
		LogInfo("Origin unavailable!");
		return {};
	}

	if (!blockAvailable[type][destination])
	{
		LogInfo("Destination unavailable!");
		return {};
	}

	auto startNode =
	    new LosNode(0.0f,
	                BattleUnitTileHelper::getDistanceStatic(blockCenterPos[type][origin],
	                                                        blockCenterPos[type][destination]),
	                nullptr, origin);
	nodesToDelete.push_back(startNode);
	fringe.emplace_back(startNode);

	auto closestNodeSoFar = *fringe.begin();

	while (iterationCount++ < iterationLimit)
	{
		auto first = fringe.begin();
		if (first == fringe.end())
		{
			LogInfo("No more blocks to expand after %d iterations", iterationCount);
			break;
		}
		auto nodeToExpand = *first;
		fringe.erase(first);

		// Skip if we've already expanded this
		if (visitedBlocks[nodeToExpand->block])
		{
			iterationCount--;
			continue;
		}
		visitedBlocks[nodeToExpand->block] = true;

		// Return goal / store closest
		if (nodeToExpand->distanceToGoal == 0)
		{
			closestNodeSoFar = nodeToExpand;
			break;
		}
		else if (nodeToExpand->distanceToGoal < closestNodeSoFar->distanceToGoal)
		{
			closestNodeSoFar = nodeToExpand;
		}

		// Try every possible connection
		int i = nodeToExpand->block;
		for (int j = 0; j < lbCount; j++)
		{
			if (j == i || linkCost[type][i + j * lbCount] == -1 || visitedBlocks[j])
			{
				continue;
			}

			float newNodeCost = nodeToExpand->costToGetHere;
			newNodeCost += linkCost[type][i + j * lbCount];

			auto newNode =
			    new LosNode(newNodeCost,
			                BattleUnitTileHelper::getDistanceStatic(
			                    blockCenterPos[type][j], blockCenterPos[type][destination]),
			                nodeToExpand, j);
			nodesToDelete.push_back(newNode);

			// Put node at appropriate place in the list
			auto it = fringe.begin();
			while (it != fringe.end() && ((*it)->costToGetHere + (*it)->distanceToGoal) <
			                                 (newNode->costToGetHere + newNode->distanceToGoal))
				it++;
			fringe.emplace(it, newNode);
		}
	}

	if (iterationCount > iterationLimit)
	{
		LogWarning("No route from lb %d to %d found after %d iterations, returning "
		           "closest path ending in %d",
		           origin, destination, iterationCount, closestNodeSoFar->block);
	}
	else if (closestNodeSoFar->distanceToGoal > 0)
	{
		LogInfo("Surprisingly, no nodes to expand! Closest path ends in %d",
		        closestNodeSoFar->block);
	}

	auto result = closestNodeSoFar->getPathToNode();

	for (auto &p : nodesToDelete)
	{
		delete p;
	}

	return result;
}

// FIXME: Implement usage of teleporters in group move
void Battle::groupMove(GameState &state, std::list<StateRef<BattleUnit>> &selectedUnits,
                       Vec3<int> targetLocation, int facingDelta, bool demandGiveWay,
                       bool useTeleporter)
{
	std::ignore = useTeleporter;
	// Legend:
	//
	// (arrive from the southwest)						(arrive from the south)
	//
	//         6			G = goal					         7			G = goal
	//       5   6			F = flanks					       7   7		1 = 1s back row
	//     4   5   6		1 = 1st back row			     6   6   6		2 = 2nd back row
	//   3   F   5   6		2 = 2nd back row			   5   5   5   5	3 = 3rd back row
	// 2   1   G   5   6	3 = sides of 1st back row	 F   F   G   F   F	4 = sides of 1st bk row
	//   2   1   F   5		4 = sides of flanks			   4   1   1   4	F = flanks
	//     2   1   4		5 = 1st front row			     2   2   2		5 = 1st front row
	//       2   3			6 = 2nd front row			       3   3		6 = 2nd front row
	//         2										         3			7 = 3rd front row
	//
	// We will of course rotate this accordingly with the direction from which units come

	static const std::list<Vec3<int>> targetOffsetsDiagonal = {
	    // Two locations to the flanks
	    {-1, -1, 0},
	    {1, 1, 0},
	    // Three locations in the 1st back row
	    {-1, 1, 0},
	    {-2, 0, 0},
	    {0, 2, 0},
	    // 2nd Back row
	    {-2, 2, 0},
	    {-3, 1, 0},
	    {-1, 3, 0},
	    {-4, 0, 0},
	    {0, 4, 0},
	    // Two locations to the side of the 1st back row
	    {-3, -1, 0},
	    {1, 3, 0},
	    // Two locations to the side of the flanks
	    {-2, -2, 0},
	    {2, 2, 0},
	    // 1st Front row
	    {1, -1, 0},
	    {0, -2, 0},
	    {2, 0, 0},
	    {-1, -3, 0},
	    {3, 1, 0},
	    // 2nd Front row
	    {2, -2, 0},
	    {1, -3, 0},
	    {3, -1, 0},
	    {0, -4, 0},
	    {4, 0, 0},
	};
	static const std::map<Vec2<int>, int> rotationDiagonal = {
	    {{1, -1}, 0},
	    {{1, 1}, 1},
	    {{-1, 1}, 2},
	    {{-1, -1}, 3},
	};
	static const std::list<Vec3<int>> targetOffsetsLinear = {
	    // Two locations in the 1st back row
	    {-1, 1, 0},
	    {1, 1, 0},
	    // Three locations in the 2nd back row
	    {0, 2, 0},
	    {-2, 2, 0},
	    {2, 2, 0},
	    // 3rd Back row
	    {-1, 3, 0},
	    {1, 3, 0},
	    {0, 4, 0},
	    // Sides of the 1st back row
	    {-3, 1, 0},
	    {3, 1, 0},
	    // Flans
	    {-2, 0, 0},
	    {2, 0, 0},
	    {-4, 0, 0},
	    {4, 0, 0},
	    // 1st front row
	    {-1, -1, 0},
	    {1, -1, 0},
	    {-3, -1, 0},
	    {3, -1, 0},
	    // 2nd front row
	    {0, -2, 0},
	    {-2, -2, 0},
	    {2, -2, 0},
	    // 3rd front row
	    {-1, -3, 0},
	    {1, -3, 0},
	    {0, -4, 0},
	};
	static const std::map<Vec2<int>, int> rotationLinear = {
	    {{0, -1}, 0},
	    {{1, 0}, 1},
	    {{0, 1}, 2},
	    {{-1, 0}, 3},
	};

	if (selectedUnits.empty())
	{
		return;
	}
	if (selectedUnits.size() == 1)
	{
		selectedUnits.front()->setMission(
		    state, BattleUnitMission::gotoLocation(*selectedUnits.front(), targetLocation,
		                                           facingDelta, demandGiveWay, true, 20, false));
		return;
	}

	UString log = ";";
	log += format("\nGroup move order issued to %d, %d, %d. Looking for the leader. Total number "
	              "of units: %d",
	              targetLocation.x, targetLocation.y, targetLocation.z, (int)selectedUnits.size());

	// Sort units based on proximity to target and speed

	auto localUnits = selectedUnits;
	localUnits.sort([targetLocation](const StateRef<BattleUnit> &a, const StateRef<BattleUnit> &b) {
		return BattleUnitTileHelper::getDistanceStatic((Vec3<int>)a->position, targetLocation) /
		           a->agent->modified_stats.getActualSpeedValue() <
		       BattleUnitTileHelper::getDistanceStatic((Vec3<int>)b->position, targetLocation) /
		           b->agent->modified_stats.getActualSpeedValue();
	});

	// Find the unit that will lead the group

	StateRef<BattleUnit> leadUnit;
	BattleUnitMission *leadMission = nullptr;
	int minDistance = INT_MAX;
	auto itUnit = localUnits.begin();
	while (itUnit != localUnits.end())
	{
		auto curUnit = *itUnit;
		log += format("\nTrying unit %s for leader", curUnit.id);

		auto mission = BattleUnitMission::gotoLocation(*curUnit, targetLocation, facingDelta,
		                                               demandGiveWay, true, 20, false);
		bool missionAdded = curUnit->setMission(state, mission);
		if (missionAdded)
		{
			mission->start(state, *curUnit);
			if (mission->currentPlannedPath.empty())
			{
				mission->cancelled = true;
			}
			else
			{
				auto unitTarget = mission->currentPlannedPath.back();
				int absX = std::abs(targetLocation.x - unitTarget.x);
				int absY = std::abs(targetLocation.y - unitTarget.y);
				int absZ = std::abs(targetLocation.z - unitTarget.z);
				int distance = std::max(std::max(absX, absY), absZ) + absX + absY + absZ;
				if (distance < minDistance)
				{
					log += format("\nUnit was the closest to target yet, remembering him.");
					// Cancel last leader's mission
					if (leadMission)
					{
						leadMission->cancelled = true;
					}
					minDistance = distance;
					leadUnit = curUnit;
					leadMission = mission;
					if (distance == 0)
					{
						log += format("\nUnit could reach target, chosen to be the leader.");
						break;
					}
				}
				else
				{
					mission->cancelled = true;
				}
			}
		}
		if (missionAdded)
		{
			log += format("\nUnit could not path to target, trying next one.");
			// Unit cannot path to target but maybe he can path to something near it, leave him in
			itUnit++;
		}
		else
		{
			log += format("\nUnit could not set a goto mission, removing him.");
			// Unit cannot add a movement mission - remove him
			itUnit = localUnits.erase(itUnit);
		}
	}
	if (itUnit == localUnits.end() && !leadUnit)
	{
		log += format("\nNoone could path to target, aborting");
		LogWarning("%s", log);
		return;
	}

	// In case we couldn't reach it, change our target
	targetLocation = leadMission->currentPlannedPath.back();
	// Remove leader from list of units that require pathing
	localUnits.remove(leadUnit);
	// Determine our direction and rotation
	auto fromIt = leadMission->currentPlannedPath.rbegin();
	int fromLimit = std::min(3, (int)leadMission->currentPlannedPath.size());
	for (int i = 0; i < fromLimit; i++)
	{
		fromIt++;
	}
	Vec2<int> dir = {clamp(targetLocation.x - fromIt->x, -1, 1),
	                 clamp(targetLocation.y - fromIt->y, -1, 1)};
	if (dir.x == 0 && dir.y == 0)
	{
		dir.y = -1;
	}
	bool diagonal = dir.x != 0 && dir.y != 0;
	auto &targetOffsets = diagonal ? targetOffsetsDiagonal : targetOffsetsLinear;
	int rotation = diagonal ? rotationDiagonal.at(dir) : rotationLinear.at(dir);

	// Sort remaining units based on proximity to target and speed
	auto h = BattleUnitTileHelper(*map, *leadUnit);
	localUnits.sort(
	    [h, targetLocation](const StateRef<BattleUnit> &a, const StateRef<BattleUnit> &b) {
		    return h.getDistance((Vec3<int>)a->position, targetLocation) /
		               a->agent->modified_stats.getActualSpeedValue() <
		           h.getDistance((Vec3<int>)b->position, targetLocation) /
		               b->agent->modified_stats.getActualSpeedValue();
	    });

	// Path every other unit to areas around target
	log += format("\nTarget location is now %d, %d, %d. Leader is %s", targetLocation.x,
	              targetLocation.y, targetLocation.z, leadUnit.id);

	auto itOffset = targetOffsets.begin();
	for (auto &unit : localUnits)
	{
		if (itOffset == targetOffsets.end())
		{
			log += format("\nRan out of location offsets, exiting");
			LogWarning("%s", log);
			return;
		}
		log += format("\nPathing unit %s", unit.id);
		while (itOffset != targetOffsets.end())
		{
			auto offset = rotate(*itOffset, rotation);
			auto targetLocationOffsetted = targetLocation + offset;
			if (targetLocationOffsetted.x < 0 || targetLocationOffsetted.x >= map->size.x ||
			    targetLocationOffsetted.y < 0 || targetLocationOffsetted.y >= map->size.y ||
			    targetLocationOffsetted.z < 0 || targetLocationOffsetted.z >= map->size.z)
			{
				log += format("\nLocation was outside map bounds, trying next one");
				itOffset++;
				continue;
			}

			log += format("\nTrying location %d, %d, %d at offset %d, %d, %d",
			              targetLocationOffsetted.x, targetLocationOffsetted.y,
			              targetLocationOffsetted.z, offset.x, offset.y, offset.z);
			float costLimit = 1.50f * 2.0f *
			                  (float)(std::max(std::abs(offset.x), std::abs(offset.y)) +
			                          std::abs(offset.x) + std::abs(offset.y));
			auto path =
			    map->findShortestPath(targetLocation, targetLocationOffsetted, costLimit / 2.0f, h,
			                          false, true, true, false, nullptr, costLimit);
			itOffset++;
			if (!path.empty() && path.back() == targetLocationOffsetted)
			{
				log += format("\nLocation checks out, pathing to it");
				unit->setMission(state, BattleUnitMission::gotoLocation(
				                            *unit, targetLocationOffsetted, facingDelta,
				                            demandGiveWay, true, 20, false));
				break;
			}
			log += format("\nLocation was unreachable, trying next one");
		}
	}
	log += format("\nSuccessfully pathed everybody to target");
	LogWarning("%s", log);
}

std::list<Vec3<int>> City::findShortestPath(Vec3<int> origin, Vec3<int> destination,
                                            const GroundVehicleTileHelper &canEnterTile
                                            [[maybe_unused]],
                                            bool approachOnly [[maybe_unused]], bool, bool, bool)
{
	int originID = getRoadSegmentID(origin);
	int destinationID = getRoadSegmentID(destination);
	auto &originSeg = roadSegments[originID];

	//
	// Quick checks before we begin
	//

	// Origin not intact?
	if (!originSeg.getIntactByTile(origin))
	{
		return {origin};
	}

	// Same segment quick path
	if (originID == destinationID)
	{
		auto path = originSeg.findPath(origin, destination);
		if (path.back() == destination)
		{
			return path;
		}
	}

	//
	// Part 1: Pathfinding on RoadSegments
	//

	int iterationLimit = 9001;
	int rsCount = roadSegments.size();
	std::vector<bool> visitetSegmentsC1 = std::vector<bool>(rsCount, false);
	std::vector<bool> visitetSegmentsC2 = std::vector<bool>(rsCount, false);
	std::list<RoadSegmentNode *> nodesToDelete;
	std::list<RoadSegmentNode *> fringe;
	int iterationCount = 0;

	LogInfo("Trying to route from rs %d to rs %d", originID, destinationID);

	auto startNode = new RoadSegmentNode(
	    0.0f, GroundVehicleTileHelper::getDistanceStatic(origin, destination), nullptr, originID);
	nodesToDelete.push_back(startNode);
	auto closestNodeSoFar = startNode;
	// If starting in a road segment then try both connections for start nodes
	if (originSeg.length > 1)
	{
		auto pathToFront = originSeg.findPath(origin, originSeg.getFirst());
		if (!pathToFront.empty() && pathToFront.back() == originSeg.getFirst())
		{
			visitetSegmentsC1[originID] = true;
			// Check if segment can be entered
			// For non-roads check tile number 0
			// For roads check based on where we came from
			auto nextSeg = roadSegments[originSeg.connections[0]];
			int intoConnect = nextSeg.length == 1 || nextSeg.connections[0] == originID ? 0 : 1;
			// Entrance intact
			if (nextSeg.getIntactByConnectID(intoConnect))
			{
				auto intoTile = nextSeg.getByConnectID(intoConnect);
				auto secondNode = new RoadSegmentNode(
				    pathToFront.size() - 1,
				    GroundVehicleTileHelper::getDistanceStatic(intoTile, destination), startNode,
				    originSeg.connections[0]);
				nodesToDelete.push_back(secondNode);
				fringe.emplace_back(secondNode);
			}
		}
		auto pathToBack = originSeg.findPath(origin, originSeg.getLast());
		if (!pathToBack.empty() && pathToBack.back() == originSeg.getLast())
		{
			visitetSegmentsC2[originID] = true;
			// Check if segment can be entered
			// For non-roads check tile number 0
			// For roads check based on where we came from
			auto nextSeg = roadSegments[originSeg.connections[1]];
			int intoConnect = nextSeg.length == 1 || nextSeg.connections[0] == originID ? 0 : 1;
			// Entrance intact
			if (nextSeg.getIntactByConnectID(intoConnect))
			{
				auto intoTile = nextSeg.getByConnectID(intoConnect);
				auto secondNode = new RoadSegmentNode(
				    pathToBack.size() - 1,
				    GroundVehicleTileHelper::getDistanceStatic(intoTile, destination), startNode,
				    originSeg.connections[1]);
				nodesToDelete.push_back(secondNode);
				fringe.emplace_back(secondNode);
			}
		}
	}
	// Else if starting on intersection just try this node
	else
	{
		fringe.emplace_back(startNode);
	}

	while (iterationCount++ < iterationLimit)
	{
		auto first = fringe.begin();
		if (first == fringe.end())
		{
			LogInfo("No more road segments to expand after %d iterations", iterationCount);
			break;
		}
		auto nodeToExpand = *first;
		fringe.erase(first);

		int thisID = nodeToExpand->segment;

		// Skip if we've already expanded this
		if (visitetSegmentsC1[thisID] && visitetSegmentsC2[thisID])
		{
			iterationCount--;
			continue;
		}

		// Step 01: Mark as visited (continue if visited broken road from this entrance)

		auto &thisSeg = roadSegments[thisID];
		// For roads remember where we came from
		int fromConnect = -1;
		if (thisSeg.length > 1)
		{
			// Find out where we came from (assuming we have a proper segment map)
			fromConnect = thisSeg.connections[0] == nodeToExpand->parentNode->segment ? 0 : 1;
		}
		// If intersection or intact road - mark both exits
		if (thisSeg.length == 1 || thisSeg.intact)
		{
			visitetSegmentsC1[thisID] = true;
			visitetSegmentsC2[thisID] = true;
		}
		else // road not intact - mark one exit that we came from or continue if marked
		{
			if (fromConnect == 0)
			{
				if (visitetSegmentsC1[thisID])
				{
					iterationCount--;
					continue;
				}
				visitetSegmentsC1[thisID] = true;
			}
			else
			{
				if (visitetSegmentsC2[thisID])
				{
					iterationCount--;
					continue;
				}
				visitetSegmentsC2[thisID] = true;
			}
		}

		// Step 02: Return goal / store closest

		// If at goal segment
		if (thisID == destinationID)
		{
			// If segment intact or intersection - return
			if (thisSeg.length == 1 || thisSeg.intact)
			{
				nodeToExpand->distanceToGoal = 0.0f;
				closestNodeSoFar = nodeToExpand;
				break;
			}
			// If a broken road - see if we can path
			else
			{
				auto from = thisSeg.getByConnectID(fromConnect);
				auto path = thisSeg.findPath(from, destination);
				// Expecting path to have at least our position as it can only be empty
				// if the entrance tile is not intact, but in that case we wouldn't be here
				if (path.back() == destination)
				{
					nodeToExpand->distanceToGoal = 0.0f;
					closestNodeSoFar = nodeToExpand;
					break;
				}
				// Can't path - adjust distances
				else
				{
					nodeToExpand->costToGetHere += path.size();
					nodeToExpand->distanceToGoal =
					    GroundVehicleTileHelper::getDistanceStatic(path.back(), destination);
				}
			}
		}
		// If at goal but broken or not at goal
		if (nodeToExpand->distanceToGoal < closestNodeSoFar->distanceToGoal)
		{
			closestNodeSoFar = nodeToExpand;
		}

		// Step 03: Try our connections

		// If broken road we can't go any further (broken intersections aren't even considered)
		if (!thisSeg.intact)
		{
			continue;
		}

		auto length = thisSeg.length;

		// Now try every connection
		for (auto &c : thisSeg.connections)
		{
			// If visited then continue (if we came from here then we also have visited)
			if (visitetSegmentsC1[c] && visitetSegmentsC2[c])
			{
				continue;
			}

			// Check if segment can be entered
			// For non-roads check tile number 0
			// For roads check based on where we came from
			auto nextSeg = roadSegments[c];
			int intoConnect = nextSeg.length == 1 || nextSeg.connections[0] == thisID ? 0 : 1;
			// Entrance not intact
			if (!nextSeg.getIntactByConnectID(intoConnect))
			{
				continue;
			}
			auto intoTile = nextSeg.getByConnectID(intoConnect);

			// Can be entered and didn't visit - add
			auto newNode = new RoadSegmentNode(
			    nodeToExpand->costToGetHere + length,
			    GroundVehicleTileHelper::getDistanceStatic(intoTile, destination), nodeToExpand, c);
			nodesToDelete.push_back(newNode);

			// Put node at appropriate place in the list
			auto it = fringe.begin();
			while (it != fringe.end() && ((*it)->costToGetHere + (*it)->distanceToGoal) <
			                                 (newNode->costToGetHere + newNode->distanceToGoal))
				it++;
			fringe.emplace(it, newNode);
		}
	}

	if (iterationCount > iterationLimit)
	{
		LogWarning("No route from lb %d to %d found after %d iterations, returning "
		           "closest path ending at %d",
		           origin, destination, iterationCount, closestNodeSoFar->segment);
	}
	else if (closestNodeSoFar->distanceToGoal > 0)
	{
		LogInfo("Surprisingly, no nodes to expand! Closest path ends at %d",
		        closestNodeSoFar->segment);
	}

	auto segmentPath = closestNodeSoFar->getPathToNode();
	for (auto &p : nodesToDelete)
	{
		delete p;
	}

	//
	// Part 2: Pathfinding using RoadSegment path
	//

	std::list<Vec3<int>> result;

	// Expecting non-zero path here as we at least have our own node
	// we don't need it though
	segmentPath.pop_front();

	// Step 01: Path to exit of origin segment

	// If road then path to road end
	if (originSeg.length > 1 && !segmentPath.empty())
	{
		int toConnect = originSeg.connections[0] == segmentPath.front() ? 0 : 1;
		auto pathToExit = originSeg.findPath(origin, originSeg.getByConnectID(toConnect));
		for (auto &p : pathToExit)
		{
			result.push_back(p);
		}
	}
	else
	{
		// If we're intersection just add origin
		// If we're a non-connected road we will path to closest to destination point
		// When we reach step 3, so just add origin for now
		result.push_back(origin);
	}

	// Step 02: Path through segments

	int thisID = originID;
	int lastID = destinationID;
	if (!segmentPath.empty())
	{
		lastID = segmentPath.back();
	}
	for (auto &segID : segmentPath)
	{
		auto &nextSeg = roadSegments[segID];
		int intoConnect = nextSeg.length == 1 || nextSeg.connections[0] == thisID ? 0 : 1;
		// If not last and not broken then path through
		if (segID != lastID && nextSeg.intact)
		{
			auto pathThrough = nextSeg.findPathThrough(intoConnect);
			for (auto &p : pathThrough)
			{
				result.push_back(p);
			}
		}
		// If last or broken we need to add entrance point and we will exit the cycle now
		// Expecting broken to be the last one as otherwise we have no point visiting it!
		else
		{
			result.push_back(nextSeg.getByConnectID(intoConnect));
		}
		thisID = segID;
	}

	// Step 03: Path within segment to destination

	auto &destSeg = roadSegments[thisID];
	// 1) Reached destination? Path to destination, and we don't care if it's broken,
	// as the result will be the closest path even if it is broken
	// 2) Didn't reach destination? Path to closest point, this is the closest we get to it
	auto pathToDestination = thisID == destinationID
	                             ? destSeg.findPath(result.back(), destination)
	                             : destSeg.findClosestPath(result.back(), destination);

	// Expecting to have at least our pos at the start, as our pos must be intact
	pathToDestination.pop_front();
	for (auto &p : pathToDestination)
	{
		result.push_back(p);
	}

	return result;
}

std::list<Vec3<int>> RoadSegment::findPath(Vec3<int> origin, Vec3<int> destination) const
{
	// Expecting to contain both points
	int originIndex = -1;
	int destinationIndex = -1;
	for (int i = 0; i < tilePosition.size(); i++)
	{
		if (tilePosition.at(i) == origin)
		{
			originIndex = i;
		}
		if (tilePosition.at(i) == destination)
		{
			destinationIndex = i;
		}
	}
	std::list<Vec3<int>> path;
	int diff = destinationIndex > originIndex ? 1 : -1;
	for (int i = originIndex; i != destinationIndex + diff; i += diff)
	{
		if (!tileIntact.at(i))
		{
			return path;
		}
		path.push_back(tilePosition.at(i));
	}

	return path;
}

std::list<Vec3<int>> RoadSegment::findClosestPath(Vec3<int> origin, Vec3<int> destination) const
{
	int closestDistance = INT_MAX;
	auto closestPoint = origin;

	for (auto &p : tilePosition)
	{
		int distance = GroundVehicleTileHelper::getDistanceStatic(p, destination);
		if (distance < closestDistance)
		{
			closestDistance = distance;
			closestPoint = p;
		}
	}

	return findPath(origin, closestPoint);
}

std::list<Vec3<int>> RoadSegment::findPathThrough(int id) const
{
	// Expecting to be intact as we have checked before
	// Path through intersection is just that point
	if (length == 1)
	{
		return {tilePosition[0]};
	}
	// Path through road
	if (id == 0)
	{
		return findPath(getFirst(), getLast());
	}
	else
	{
		return findPath(getLast(), getFirst());
	}
}

void City::groupMove(GameState &state, std::list<StateRef<Vehicle>> &selectedVehicles,
                     Vec3<int> targetLocation, bool useTeleporter)
{
	// Legend:
	//
	// (arrive from the southwest)						(arrive from the south)
	//
	//         6			G = goal					         7			G = goal
	//       5   6			F = flanks					       7   7		1 = 1s back row
	//     4   5   6		1 = 1st back row			     6   6   6		2 = 2nd back row
	//   3   F   5   6		2 = 2nd back row			   5   5   5   5	3 = 3rd back row
	// 2   1   G   5   6	3 = sides of 1st back row	 F   F   G   F   F	4 = sides of 1st bk row
	//   2   1   F   5		4 = sides of flanks			   4   1   1   4	F = flanks
	//     2   1   4		5 = 1st front row			     2   2   2		5 = 1st front row
	//       2   3			6 = 2nd front row			       3   3		6 = 2nd front row
	//         2										         3			7 = 3rd front row
	//
	// We will of course rotate this accordingly with the direction from which units come

	static const std::list<Vec3<int>> targetOffsetsDiagonal = {
	    {0, 0, 0},
	    // Two locations to the flanks
	    {-1, -1, 0},
	    {1, 1, 0},
	    // Three locations in the 1st back row
	    {-1, 1, 0},
	    {-2, 0, 0},
	    {0, 2, 0},
	    // 2nd Back row
	    {-2, 2, 0},
	    {-3, 1, 0},
	    {-1, 3, 0},
	    {-4, 0, 0},
	    {0, 4, 0},
	    // Two locations to the side of the 1st back row
	    {-3, -1, 0},
	    {1, 3, 0},
	    // Two locations to the side of the flanks
	    {-2, -2, 0},
	    {2, 2, 0},
	    // 1st Front row
	    {1, -1, 0},
	    {0, -2, 0},
	    {2, 0, 0},
	    {-1, -3, 0},
	    {3, 1, 0},
	    // 2nd Front row
	    {2, -2, 0},
	    {1, -3, 0},
	    {3, -1, 0},
	    {0, -4, 0},
	    {4, 0, 0},
	};
	static const std::map<Vec2<int>, int> rotationDiagonal = {
	    {{1, -1}, 0},
	    {{1, 1}, 1},
	    {{-1, 1}, 2},
	    {{-1, -1}, 3},
	};
	static const std::list<Vec3<int>> targetOffsetsLinear = {
	    {0, 0, 0},
	    // Two locations in the 1st back row
	    {-1, 1, 0},
	    {1, 1, 0},
	    // Three locations in the 2nd back row
	    {0, 2, 0},
	    {-2, 2, 0},
	    {2, 2, 0},
	    // 3rd Back row
	    {-1, 3, 0},
	    {1, 3, 0},
	    {0, 4, 0},
	    // Sides of the 1st back row
	    {-3, 1, 0},
	    {3, 1, 0},
	    // Flanks
	    {-2, 0, 0},
	    {2, 0, 0},
	    {-4, 0, 0},
	    {4, 0, 0},
	    // 1st front row
	    {-1, -1, 0},
	    {1, -1, 0},
	    {-3, -1, 0},
	    {3, -1, 0},
	    // 2nd front row
	    {0, -2, 0},
	    {-2, -2, 0},
	    {2, -2, 0},
	    // 3rd front row
	    {-1, -3, 0},
	    {1, -3, 0},
	    {0, -4, 0},
	};
	static const std::map<Vec2<int>, int> rotationLinear = {
	    {{0, -1}, 0},
	    {{1, 0}, 1},
	    {{0, 1}, 2},
	    {{-1, 0}, 3},
	};

	if (selectedVehicles.empty())
	{
		return;
	}
	if (selectedVehicles.size() == 1 ||
	    (selectedVehicles.size() == 2 && selectedVehicles.front()->owner != state.getPlayer()))
	{
		auto v = selectedVehicles.front();
		if (v->owner == state.getPlayer())
		{
			auto targetPos = v->getPreferredPosition(targetLocation);
			// FIXME: Don't clear missions if not replacing current mission
			v->setMission(state, VehicleMission::gotoLocation(state, *v, targetPos, useTeleporter));
		}
		return;
	}

	auto it = selectedVehicles.begin();
	if ((*it)->owner != state.getPlayer())
	{
		it++;
	}
	auto dir = Vec2<int>{(int)targetLocation.x - (*it)->position.x,
	                     (int)targetLocation.y - (*it)->position.y};
	if (dir.x != 0 || dir.y != 0)
	{
		auto targetVectorNorm =
		    glm::normalize(Vec2<float>{(float)targetLocation.x - (*it)->position.x,
		                               (float)targetLocation.y - (*it)->position.y});
		dir = {roundf(targetVectorNorm.x), roundf(targetVectorNorm.y)};
	}
	if (dir.x == 0 && dir.y == 0)
	{
		dir.y = -1;
	}
	bool diagonal = dir.x != 0 && dir.y != 0;
	auto &targetOffsets = diagonal ? targetOffsetsDiagonal : targetOffsetsLinear;
	int rotation = diagonal ? rotationDiagonal.at(dir) : rotationLinear.at(dir);

	auto itOffset = targetOffsets.begin();
	while (it != selectedVehicles.end())
	{
		if (itOffset == targetOffsets.end())
		{
			// FIXME: Generate more offsets in an enlarging diamond shape
			LogWarning("\nRan out of location offsets while pathing vehicles, exiting");
			return;
		}
		while (itOffset != targetOffsets.end())
		{
			auto offset = rotate(*itOffset, rotation);
			auto targetLocationOffsetted = targetLocation + offset;
			if (targetLocationOffsetted.x < 0 || targetLocationOffsetted.x >= map->size.x ||
			    targetLocationOffsetted.y < 0 || targetLocationOffsetted.y >= map->size.y ||
			    targetLocationOffsetted.z < 0 || targetLocationOffsetted.z >= map->size.z)
			{
				itOffset++;
				continue;
			}
			itOffset++;

			auto targetPos = (*it)->getPreferredPosition(targetLocationOffsetted);
			// FIXME: Don't clear missions if not replacing current mission
			(*it)->setMission(state,
			                  VehicleMission::gotoLocation(state, **it, targetPos, useTeleporter));
			it++;
			break;
		}
	}
}

void City::fillRoadSegmentMap(GameState &state [[maybe_unused]])
{
	LogWarning("Begun filling road segment map");
	// Expecting this to be done on clean intact map
	tileToRoadSegmentMap.clear();
	roadSegments.clear();
	auto &m = *map;
	auto helper = GroundVehicleTileHelper{m, VehicleType::Type::Road};

	// -2 means not processed, -1 means no road, otherwise segment index
	tileToRoadSegmentMap.resize(m.size.x * m.size.y * m.size.z, -2);

	// [First Loop]
	// Try every tile on the map as new segment
	// - on first pass only try terminals
	// - on second pass only try crossings
	// - on third pass try anything
	Vec3<int> tileToTryNext;
	int nextSegmentToProcess = 0;
	int currentPass = 0;
	while (currentPass++ < 3)
	{
		for (tileToTryNext.x = 0; tileToTryNext.x < m.size.x; tileToTryNext.x++)
		{
			for (tileToTryNext.y = 0; tileToTryNext.y < m.size.y; tileToTryNext.y++)
			{
				for (tileToTryNext.z = 0; tileToTryNext.z < m.size.z; tileToTryNext.z++)
				{
					// Already processed
					if (tileToRoadSegmentMap[tileToTryNext.z * map->size.x * map->size.y +
					                         tileToTryNext.y * map->size.x + tileToTryNext.x] != -2)
					{
						continue;
					}
					// Get data
					auto tile = m.getTile(tileToTryNext);
					auto scenery = tile->presentScenery ? tile->presentScenery->type : nullptr;
					// Not a road
					if (!scenery || scenery->tile_type != SceneryTileType::TileType::Road)
					{
						tileToRoadSegmentMap[tileToTryNext.z * map->size.x * map->size.y +
						                     tileToTryNext.y * map->size.x + tileToTryNext.x] = -1;
						continue;
					}
					// Not fit for this pass
					switch (currentPass)
					{
						// First pass only terminals
						case 1:
							if (scenery->road_type != SceneryTileType::RoadType::Terminal)
							{
								continue;
							}
							break;
						// Second pass only junctions
						case 2:
							if (scenery->road_type != SceneryTileType::RoadType::Junction)
							{
								continue;
							}
							LogWarning("Pass 2: Tile %s disconnected from some exits network",
							           tileToTryNext);
							break;
						// Anything goes, coming up OOOs!
						case 3:
							LogWarning("Pass 3: Tile %s disconnected from main network!",
							           tileToTryNext);
							break;
					}
					// Checks out, add and process it next
					tileToRoadSegmentMap[tileToTryNext.z * map->size.x * map->size.y +
					                     tileToTryNext.y * map->size.x + tileToTryNext.x] =
					    nextSegmentToProcess;
					roadSegments.emplace_back(tileToTryNext);

					// [Second Loop]
					// Process new segments
					while (nextSegmentToProcess < roadSegments.size())
					{
						if (roadSegments[nextSegmentToProcess].empty())
						{
							LogWarning("Skipping empty segment %d", nextSegmentToProcess);
							nextSegmentToProcess++;
							continue;
						}
						auto initialConnections =
						    roadSegments[nextSegmentToProcess].connections.size();
						LogInfo("Segment %d Connections %d First %d", nextSegmentToProcess,
						        initialConnections,
						        initialConnections == 0
						            ? -1
						            : roadSegments[nextSegmentToProcess].connections.front());
						// Until we reach an intersection or become one
						do
						{
							// Try to connect to every adjacent tile
							auto currentPosition =
							    roadSegments[nextSegmentToProcess].tilePosition.back();
							auto thisTile = m.getTile(currentPosition);
							bool existing = false;
							std::list<int> newSegment;
							for (int z = -1; z <= 1; z++)
							{
								for (int y = -1; y <= 1; y++)
								{
									for (int x = -1; x <= 1; x++)
									{
										if (x == 0 && y == 0 && z == 0)
										{
											continue;
										}
										auto nextPosition = currentPosition;
										nextPosition.x += x;
										nextPosition.y += y;
										nextPosition.z += z;
										if (!m.tileIsValid(nextPosition))
										{
											continue;
										}
										// Skip if already a part of segment
										if (std::find(roadSegments[nextSegmentToProcess]
										                  .tilePosition.begin(),
										              roadSegments[nextSegmentToProcess]
										                  .tilePosition.end(),
										              nextPosition) !=
										    roadSegments[nextSegmentToProcess].tilePosition.end())
										{
											continue;
										}
										// Skip if already connected to it
										int idx =
										    tileToRoadSegmentMap[nextPosition.z * map->size.x *
										                             map->size.y +
										                         nextPosition.y * map->size.x +
										                         nextPosition.x];
										if (std::find(roadSegments[nextSegmentToProcess]
										                  .connections.begin(),
										              roadSegments[nextSegmentToProcess]
										                  .connections.end(),
										              idx) !=
										    roadSegments[nextSegmentToProcess].connections.end())
										{
											continue;
										}
										auto nextTile = m.getTile(nextPosition);
										if (!helper.canEnterTile(thisTile, nextTile))
										{
											continue;
										}
										// New connection
										if (idx == -2)
										{
											// New segment, connected to us
											// First try to find an empty one
											for (idx = nextSegmentToProcess + 1;
											     idx < roadSegments.size(); idx++)
											{
												if (roadSegments[idx].empty())
												{
													roadSegments[idx].tilePosition.emplace_back(
													    nextPosition);
													roadSegments[idx].connections.push_back(
													    nextSegmentToProcess);
													break;
												}
											}
											tileToRoadSegmentMap[nextPosition.z * map->size.x *
											                         map->size.y +
											                     nextPosition.y * map->size.x +
											                     nextPosition.x] = idx;
											// If have not found an empty one then emplace a new one
											if (idx == (int)roadSegments.size())
											{
												roadSegments.emplace_back(nextPosition,
												                          nextSegmentToProcess);
											}
										}
										else
										{
											// Some sanity
											if (idx == -1)
											{
												LogError("Linking from %s to %s: Non road, wtf?",
												         currentPosition, nextPosition);
												break;
											}
											if (roadSegments[idx].tilePosition.size() > 1)
											{
												LogWarning("Linking from %s to %s: Existing road "
												           "segment, wtf?",
												           currentPosition, nextPosition);
												// break;
											}
											// Existing segment, link to us
											existing = true;
											roadSegments[idx].connections.emplace_back(
											    nextSegmentToProcess);
										}
										newSegment.push_back(idx);
										// Link us to it
										roadSegments[nextSegmentToProcess].connections.emplace_back(
										    idx);
									}
								}
							} // For every adjacent tile

							// If more than two connections then this is a junction, must detach
							// tail and break
							// If no new connections or less than two we have reached a road end
							// (terminus) and must detach it and break
							if (roadSegments[nextSegmentToProcess].connections.size() != 2 ||
							    roadSegments[nextSegmentToProcess].connections.size() ==
							        initialConnections)
							{
								// Only if we're not single tile ourselves
								if (roadSegments[nextSegmentToProcess].tilePosition.size() > 1)
								{
									// Take our last tile
									auto lastTilePosition =
									    roadSegments[nextSegmentToProcess].tilePosition.back();
									// Remove it
									roadSegments[nextSegmentToProcess].tilePosition.pop_back();
									// Make a new segment out of it
									// First try to find an empty one
									int idx;
									for (idx = nextSegmentToProcess + 1; idx < roadSegments.size();
									     idx++)
									{
										if (roadSegments[idx].empty())
										{
											roadSegments[idx].tilePosition.emplace_back(
											    lastTilePosition);
											roadSegments[idx].connections.push_back(
											    nextSegmentToProcess);
											break;
										}
									}
									tileToRoadSegmentMap[lastTilePosition.z * map->size.x *
									                         map->size.y +
									                     lastTilePosition.y * map->size.x +
									                     lastTilePosition.x] = idx;
									// If have not found an empty one then emplace a new one
									if (idx == (int)roadSegments.size())
									{
										roadSegments.emplace_back(lastTilePosition,
										                          nextSegmentToProcess);
									}
									// Change all new links to us to this new tile
									for (auto j : newSegment)
									{
										if (roadSegments[j].empty())
										{
											continue;
										}
										for (int k = 0; k < roadSegments[j].connections.size(); k++)
										{
											if (roadSegments[j].connections[k] ==
											    nextSegmentToProcess)
											{
												roadSegments[j].connections[k] = idx;
											}
										}
									}
									// Transfer all links acquired this time
									for (int i = initialConnections;
									     i < roadSegments[nextSegmentToProcess].connections.size();
									     i++)
									{
										roadSegments[idx].connections.push_back(
										    roadSegments[nextSegmentToProcess].connections[i]);
									}
									// Lose all connection we transferred
									roadSegments[nextSegmentToProcess].connections.resize(
									    initialConnections);
									// Link us to it
									roadSegments[nextSegmentToProcess].connections.emplace_back(
									    idx);
								}
								break;
							}
							// Otherwise this is part of a road
							// If linked to existing tile this road ends here
							else if (existing)
							{
								break;
							}
							// Otherwise consume last connection and continue
							else
							{
								// Find our last connection
								int lastConnection =
								    roadSegments[nextSegmentToProcess].connections.back();
								auto lastConnectionPosition =
								    roadSegments[lastConnection].tilePosition.front();
								// Erase our last connection
								roadSegments[nextSegmentToProcess].connections.pop_back();
								// Erase that segment
								roadSegments[lastConnection].tilePosition.clear();
								roadSegments[lastConnection].connections.clear();
								// Mark last connection position as belonging to us
								tileToRoadSegmentMap[lastConnectionPosition.z * map->size.x *
								                         map->size.y +
								                     lastConnectionPosition.y * map->size.x +
								                     lastConnectionPosition.x] =
								    nextSegmentToProcess;
								// Add last connection position to our tiles
								roadSegments[nextSegmentToProcess].tilePosition.emplace_back(
								    lastConnectionPosition);
								// We will now re-try this segment again and continue building road
								// until we finally reach an intersection or a terminus
								continue;
							}
						} while (true); // End of loop for this segment
						nextSegmentToProcess++;
					} // End of second loop
				}
			}
		} // End of loop for current pass
	}     // End of first loop

	for (int i = 0; i < roadSegments.size(); i++)
	{
		roadSegments[i].finalizeStats();
	}
	LogWarning("Finished filling road segment map");
}
} // namespace OpenApoc
