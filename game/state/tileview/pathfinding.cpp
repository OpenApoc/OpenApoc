#include "framework/trace.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemap.h"
#include "game/state/battle/battleunit.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/tileview/tile.h"
#include <glm/glm.hpp>
#include "limits.h"
#include <algorithm>

namespace OpenApoc
{

namespace
{
class PathNode
{
  public:
	PathNode(float costToGetHere, float distanceToGoal, PathNode *parentNode, Tile *thisTile)
	    : costToGetHere(costToGetHere), parentNode(parentNode), thisTile(thisTile),
	      distanceToGoal(distanceToGoal)
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

std::list<Vec3<int>>
TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destinationStart, Vec3<int> destinationEnd,
                          unsigned int iterationLimit, const CanEnterTileHelper &canEnterTile,
						  bool approachOnly, bool ignoreStaticUnits, bool ignoreAllUnits, float *cost, float maxCost)
{
#ifdef PATHFINDING_DEBUG
	for (auto &t : tiles)
		t.pathfindingDebugFlag = false;
#endif

	TRACE_FN;
	maxCost /= canEnterTile.pathOverheadAlloawnce();
	// Faster than looking up in a set
	std::vector<bool> visitedTiles = std::vector<bool>(size.x * size.y * size.z, false);
	int strideZ = size.x * size.y;
	int strideY = size.x;
	std::list<PathNode *> nodesToDelete;
	std::list<PathNode *> fringe;
	Vec3<float> goalPositionStart;
	Vec3<float> goalPositionEnd;
	bool destinationIsSingleTile = destinationStart == destinationEnd - Vec3<int>{1, 1, 1};
	unsigned int iterationCount = 0;

	// Approach Only makes no sense with pathing into a block, but we'll fix it anyway
	if (approachOnly && !destinationIsSingleTile)
	{
		LogWarning("Trying to route from %s to %s-%s in approachOnly mode? Extending destination's xy boundaries by 1.", origin, destinationStart, destinationEnd);
		approachOnly = false;
		destinationStart -= Vec3<int>(destinationStart.x > 0 ? 1 : 0, destinationStart.y > 0 ? 1 : 0, 0);
		destinationEnd += Vec3<int>(destinationEnd.x < size.x ? 1 : 0, destinationEnd.y < size.y ? 1 : 0, 0);
	}

	LogInfo("Trying to route from %s to %s-%s", origin, destinationStart, destinationEnd);

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

	auto startNode =
	    new PathNode(0.0f, canEnterTile.getDistance(origin, goalPositionStart, goalPositionEnd),
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
		nodeToExpand->thisTile->pathfindingDebugFlag = true;
#endif

		// Make it so we always try to move at least one tile
		if (closestNodeSoFar->parentNode == nullptr)
			closestNodeSoFar = nodeToExpand;

		if (nodeToExpand->distanceToGoal == 0 || 
			(approachOnly && nodeToExpand->thisTile->position.z == goalPositionStart.z 
				&& std::max(std::abs(nodeToExpand->thisTile->position.x - goalPositionStart.x), std::abs(nodeToExpand->thisTile->position.y - goalPositionStart.y)) <= 1))
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
						continue;

					Tile *tile = this->getTile(nextPosition);
					if (visitedTiles[tile->position.z * strideZ + tile->position.y * strideY +
					                 tile->position.x])
					{
						continue;
					}
					float cost = 0.0f;
					bool unused = false;
					if (!canEnterTile.canEnterTile(nodeToExpand->thisTile, tile, cost, unused,
					                               ignoreStaticUnits, ignoreAllUnits))
						continue;
					float newNodeCost = nodeToExpand->costToGetHere;

					newNodeCost += cost / canEnterTile.pathOverheadAlloawnce();

					// make pathfinder biased towards vehicle's altitude preference
					newNodeCost += canEnterTile.adjustCost(nextPosition, z);

					// Do not add to the fringe if too far
					if (maxCost != 0.0f && newNodeCost >= maxCost)
						continue;

					auto newNode = new PathNode(
					    newNodeCost, destinationIsSingleTile
					                     ? canEnterTile.getDistance(nextPosition, goalPositionStart)
					                     : canEnterTile.getDistance(nextPosition, goalPositionStart,
					                                                goalPositionEnd),
					    nodeToExpand, tile);
					nodesToDelete.push_back(newNode);

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
		if (approachOnly && closestNodeSoFar->thisTile->position.z == goalPositionStart.z
			&& std::max(std::abs(closestNodeSoFar->thisTile->position.x - goalPositionStart.x), std::abs(closestNodeSoFar->thisTile->position.y - goalPositionStart.y)) <= 1)
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
			LogWarning("No route from %s to %s-%s found after %d iterations, returning "
			           "closest path %s",
			           origin, destinationStart, destinationEnd, iterationCount,
			           closestNodeSoFar->thisTile->position);
		}
	}
	else if (closestNodeSoFar->distanceToGoal > 0)
	{
		if (maxCost > 0.0f)
		{
			LogInfo("Could not find path within maxPath, returning closest path %s",
			        closestNodeSoFar->thisTile->position.x);
		}
		else
		{
			LogInfo("Surprisingly, no nodes to expand! Closest path %s",
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
		*cost = closestNodeSoFar->costToGetHere * canEnterTile.pathOverheadAlloawnce();
	}

	for (auto &p : nodesToDelete)
		delete p;

	return result;
}

std::list<Vec3<int>> Battle::findShortestPath(Vec3<int> origin, Vec3<int> destination,
                                              const BattleUnitTileHelper &canEnterTile,
                                              bool approachOnly, bool ignoreStaticUnits, int iterationLimitDirect,
                                              bool forceDirect, bool ignoreAllUnits, float *cost,
                                              float maxCost)
{
	// Maximum distance, in tiless, that will result in trying the direct pathfinding first
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
		result = map->findShortestPath(
		    origin, destination,
		    iterationLimitDirect > 0 ? iterationLimitDirect
		                             : distance * PATH_ITERATION_LIMIT_MULTIPLIER,
		    canEnterTile, approachOnly, ignoreStaticUnits, ignoreAllUnits, cost, maxCost);

		auto finalTile = result.back();
		if (forceDirect 
			|| finalTile == destination
			|| (approachOnly && std::max(std::abs(finalTile.x - destination.x), std::abs(finalTile.y - destination.y)) <= 1 && finalTile.z == destination.z))
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
		if (origin.x == startLB->start.x || origin.y == startLB->start.y || origin.x == startLB->end.x - 1 ||
			origin.y == startLB->end.y - 1)
		{
			// Vector to target, determines order in which we will try things
			auto targetVector = destination - origin;
			// Wether positive or negative tile is in front of destination relative to our position
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
				possibleDestinations.push_back({ origin.x + xSign, origin.y + ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? xSign : 0, origin.y + xFirst ? 0 : ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? 0 : xSign, origin.y + xFirst ? ySign : 0, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? xSign : -xSign, origin.y + xFirst ? -ySign : ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? -xSign : xSign, origin.y + xFirst ? ySign : -ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? 0 : -xSign, origin.y + xFirst ? -ySign : 0, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? -xSign : 0, origin.y + xFirst ? 0 : -ySign, origin.z });
				possibleDestinations.push_back({ origin.x - xSign, origin.y - ySign, origin.z });
			}
			else
			{
				possibleDestinations.push_back({ origin.x + xFirst ? xSign : 0, origin.y + xFirst ? 0 : ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xSign, origin.y + ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? xSign : -xSign, origin.y + xFirst ? -ySign : ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? 0 : xSign, origin.y + xFirst ? ySign : 0, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? 0 : -xSign, origin.y + xFirst ? -ySign : 0, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? -xSign : xSign, origin.y + xFirst ? ySign : -ySign, origin.z });
				possibleDestinations.push_back({ origin.x - xSign, origin.y - ySign, origin.z });
				possibleDestinations.push_back({ origin.x + xFirst ? -xSign : 0, origin.y + xFirst ? 0 : -ySign, origin.z });
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
				// Only try to path to target LB once when first encountering a destination inside it
				if (startLB->contains(curDest))
				{
					if (!destinationEncountered)
					{
						destinationEncountered = true;
						result = findShortestPathUsingLB(origin, destination, canEnterTile, approachOnly, ignoreAllUnits, iterationLimitDirect, forceDirect, ignoreAllUnits, cost, maxCost);
					}
				}
				// Otherwise path specifically to the destination if it can be entered
				else if (canEnterTile.canEnterTile(nullptr, map->getTile(curDest)))
				{
					result = findShortestPathUsingLB(origin, curDest, canEnterTile, false, ignoreAllUnits, iterationLimitDirect, forceDirect, ignoreAllUnits, cost, maxCost);
				}
				// Process results
				if (result.empty())
				{
					continue;
				}
				// Check if final tile is acceptable
				auto finalTile = result.back();
				if (std::max(std::abs(finalTile.x - destination.x), std::abs(finalTile.y - destination.y)) <= 1 && finalTile.z == destination.z)
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

	return findShortestPathUsingLB(origin, destination, canEnterTile, approachOnly, ignoreAllUnits, iterationLimitDirect, forceDirect, ignoreAllUnits, cost, maxCost);
}

std::list<Vec3<int>> Battle::findShortestPathUsingLB(Vec3<int> origin, Vec3<int> destination,
	const BattleUnitTileHelper &canEnterTile,
	bool approachOnly, bool ignoreStaticUnits, int iterationLimitDirect,
	bool forceDirect, bool ignoreAllUnits, float *cost,
	float maxCost)
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
	auto pathLB = findLosBlockPath(startLB, destLB,
		canEnterTile.getType());

	// If pathfinding on graphs failed - return short part of the path towards target
	if ((*pathLB.rbegin()) != destLB)
	{
		if (!result.empty())
		{
			return result;
		}
		else
		{
			return map->findShortestPath(origin, destination,
				distance * PATH_ITERATION_LIMIT_MULTIPLIER, canEnterTile, approachOnly,
				ignoreStaticUnits, ignoreAllUnits, cost, maxCost);
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
		auto path = map->findShortestPath(
			curOrigin, lb.start, lb.end,
			distToNext * GRAPH_ITERATION_LIMIT_MULTIPLIER + GRAPH_ITERATION_LIMIT_EXTRA,
			canEnterTile, false, ignoreStaticUnits, ignoreAllUnits, &curCost, curMaxCost);
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
		if (getLosBlockID(result.rbegin()->x, result.rbegin()->y, result.rbegin()->z) != lbID)
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
		approachOnly, ignoreStaticUnits, ignoreAllUnits, &curCost, curMaxCost);
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
	unsigned int iterationCount = 0;

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
	    new LosNode(0.0f, BattleUnitTileHelper::getDistanceStatic(
	                          blockCenterPos[type][origin], blockCenterPos[type][destination]),
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
			if (j == i || linkCost[type][i + j * lbCount] == -1)
			{
				continue;
			}

			float newNodeCost = nodeToExpand->costToGetHere;
			newNodeCost += linkCost[type][i + j * lbCount];

			auto newNode = new LosNode(
			    newNodeCost, BattleUnitTileHelper::getDistanceStatic(
			                     blockCenterPos[type][j], blockCenterPos[type][destination]),
			    nodeToExpand, j);
			nodesToDelete.push_back(newNode);

			// Put node at appropriate place in the list
			auto it = fringe.begin();
			while (it != fringe.end() &&
			       ((*it)->costToGetHere + (*it)->distanceToGoal) <
			           (newNode->costToGetHere + newNode->distanceToGoal))
				it++;
			fringe.emplace(it, newNode);
		}
	}

	if (iterationCount > iterationLimit)
	{
		LogWarning("No route from lb %d to %d found after %d iterations, returning "
		           "closest path %d",
		           origin, destination, iterationCount, closestNodeSoFar->block);
	}
	else if (closestNodeSoFar->distanceToGoal > 0)
	{
		LogInfo("Surprisingly, no nodes to expand! Closest path %d", closestNodeSoFar->block);
	}

	auto result = closestNodeSoFar->getPathToNode();

	for (auto &p : nodesToDelete)
		delete p;

	return result;
}

// FIXME: Implement usage of teleporters in group move
void Battle::groupMove(GameState &state, std::list<StateRef<BattleUnit>> &selectedUnits,
                       Vec3<int> targetLocation, bool demandGiveWay, bool useTeleporter)
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
	    {{1, -1}, 0}, {{1, 1}, 1}, {{-1, 1}, 2}, {{-1, -1}, 3},
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
	    {{0, -1}, 0}, {{1, 0}, 1}, {{0, 1}, 2}, {{-1, 0}, 3},
	};

	if (selectedUnits.empty())
	{
		return;
	}
	else if (selectedUnits.size() == 1)
	{
		selectedUnits.front()->setMission(
		    state, BattleUnitMission::gotoLocation(*selectedUnits.front(), targetLocation, 0,
		                                           demandGiveWay, true, 20, false));
		return;
	}

	UString log = ";";
	log += format("\nGroup move order issued to %d, %d, %d. Looking for the leader. Total number "
	              "of units: %d",
	              targetLocation.x, targetLocation.y, targetLocation.z, (int)selectedUnits.size());

	// Sort units based on proximity to target and speed

	auto &map = selectedUnits.front()->tileObject->map;
	auto units = selectedUnits;
	units.sort([targetLocation](const StateRef<BattleUnit> &a, const StateRef<BattleUnit> &b) {
		return BattleUnitTileHelper::getDistanceStatic((Vec3<int>)a->position, targetLocation) /
		           a->agent->modified_stats.getActualSpeedValue() <
		       BattleUnitTileHelper::getDistanceStatic((Vec3<int>)b->position, targetLocation) /
		           b->agent->modified_stats.getActualSpeedValue();
	});

	// Find the unit that will lead the group

	StateRef<BattleUnit> leadUnit;
	BattleUnitMission *leadMission = nullptr;
	int minDistance = INT_MAX;
	auto itUnit = units.begin();
	while (itUnit != units.end())
	{
		auto curUnit = *itUnit;
		log += format("\nTrying unit %s for leader", curUnit.id);

		auto mission = BattleUnitMission::gotoLocation(*curUnit, targetLocation, 0, demandGiveWay,
		                                               true, 20, false);
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
			itUnit = units.erase(itUnit);
		}
	}
	if (itUnit == units.end() && !leadUnit)
	{
		log += format("\nNoone could path to target, aborting");
		LogWarning("%s", log);
		return;
	}

	// In case we couldn't reach it, change our target
	targetLocation = leadMission->currentPlannedPath.back();
	// Remove leader from list of units that require pathing
	units.remove(leadUnit);
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
	auto h = BattleUnitTileHelper(map, *leadUnit);
	units.sort([h, targetLocation](const StateRef<BattleUnit> &a, const StateRef<BattleUnit> &b) {
		return h.getDistance((Vec3<int>)a->position, targetLocation) /
		           a->agent->modified_stats.getActualSpeedValue() <
		       h.getDistance((Vec3<int>)b->position, targetLocation) /
		           b->agent->modified_stats.getActualSpeedValue();
	});

	// Path every other unit to areas around target
	log += format("\nTarget location is now %d, %d, %d. Leader is %s", targetLocation.x,
	              targetLocation.y, targetLocation.z, leadUnit.id);

	auto itOffset = targetOffsets.begin();
	for (auto unit : units)
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
			if (targetLocationOffsetted.x < 0 || targetLocationOffsetted.x >= map.size.x ||
			    targetLocationOffsetted.y < 0 || targetLocationOffsetted.y >= map.size.y ||
			    targetLocationOffsetted.z < 0 || targetLocationOffsetted.z >= map.size.z)
			{
				log += format("\nLocation was outside map bounds, trying next one");
				itOffset++;
				continue;
			}

			log += format("\nTrying location %d, %d, %d at offset %d, %d, %d",
			              targetLocationOffsetted.x, targetLocationOffsetted.y,
			              targetLocationOffsetted.z, offset.x, offset.y, offset.z);
			float costLimit =
			    1.50f * 2.0f * (float)(std::max(std::abs(offset.x), std::abs(offset.y)) +
			                           std::abs(offset.x) + std::abs(offset.y));
			auto path = map.findShortestPath(targetLocation, targetLocationOffsetted,
			                                 costLimit / 2.0f, h, false, true, false, nullptr, costLimit);
			itOffset++;
			if (!path.empty() && path.back() == targetLocationOffsetted)
			{
				log += format("\nLocation checks out, pathing to it");
				unit->setMission(state,
				                 BattleUnitMission::gotoLocation(*unit, targetLocationOffsetted, 0,
				                                                 demandGiveWay, true, 20, false));
				break;
			}
			log += format("\nLocation was unreachable, trying next one");
		}
	}
	log += format("\nSuccessfully pathed everybody to target");
	LogWarning("%s", log);
}
}
