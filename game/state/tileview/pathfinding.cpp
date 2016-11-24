#include "game/state/battle/battleunitmission.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlemap.h"
#include "framework/trace.h"
#include "game/state/tileview/tile.h"
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

} // anonymous namespace

std::list<Vec3<int>> TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destinationStart, Vec3<int> destinationEnd,
	unsigned int iterationLimit,
	const CanEnterTileHelper &canEnterTile,
	bool ignoreStaticUnits, bool ignoreAllUnits, float *cost, float maxCost)
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

	LogInfo("Trying to route from %s to %s-%s", origin, destinationStart, destinationEnd);

	if (origin.x < 0 || origin.x >= this->size.x || origin.y < 0 || origin.y >= this->size.y ||
	    origin.z < 0 || origin.z >= this->size.z)
	{
		LogError("Bad origin %s", origin);
		return {};
	}
	if (destinationStart.x < 0 || destinationStart.x >= this->size.x || destinationStart.y < 0 ||
	    destinationStart.y >= this->size.y || destinationStart.z < 0 || destinationStart.z >= this->size.z)
	{
		LogError("Bad destinationStart %s", destinationStart);
		return {};
	}
	if (destinationEnd.x <= destinationStart.x || destinationEnd.x > this->size.x || destinationEnd.y <= destinationStart.y ||
		destinationEnd.y > this->size.y || destinationEnd.z <= destinationStart.z || destinationEnd.z > this->size.z)
	{
		LogError("Bad destinationEnd %s", destinationEnd);
		return{};
	}

	goalPositionStart = destinationStart;
	goalPositionEnd = destinationEnd;
	
	Tile *startTile = this->getTile(origin);
	if (!startTile)
	{
		LogError("Failed to get origin tile at %s", origin);
		return {};
	}

	if (origin.x >= destinationStart.x && origin.x < destinationEnd.x 
		&& origin.y >= destinationStart.y && origin.y < destinationEnd.y 
		&& origin.z >= destinationStart.z && origin.z < destinationEnd.z)
	{
		LogInfo("Origin is within destination!");
		return {startTile->position};
	}

	auto startNode =
	    new PathNode(0.0f, canEnterTile.getDistance(origin, goalPositionStart, goalPositionEnd), nullptr, startTile);
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

		if (nodeToExpand->distanceToGoal == 0)
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
					    newNodeCost, destinationIsSingleTile ? canEnterTile.getDistance(nextPosition, goalPositionStart) : canEnterTile.getDistance(nextPosition, goalPositionStart, goalPositionEnd),
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
		if (maxCost > 0.0f)
		{
			LogInfo("No route from %s to %s-%s found after %d iterations, returning "
				"closest path %s",
				origin, destinationStart, destinationEnd, iterationCount, closestNodeSoFar->thisTile->position);
		}
		else
		{
			LogWarning("No route from %s to %s-%s found after %d iterations, returning "
				"closest path %s",
				origin, destinationStart, destinationEnd, iterationCount, closestNodeSoFar->thisTile->position);
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
	bool ignoreStaticUnits, bool ignoreAllUnits, float *cost,
	float maxCost)
{
	// Maximum distance, in tiless, that will result in trying the direct pathfinding first
	// Otherwise, we start with pathfinding using LOS blocks immediately
	static const int MAX_DISTANCE_TO_PATH_DIRECTLY = 20;

	// How much attempts are given to the pathfinding until giving up and concluding that
	// there is no simple path between orig and dest. This is a multiplier for "distance", which is
	// a minimum number of iterations required to pathfind between two locations
	static const int PATH_ITERATION_LIMIT_MULTIPLIER = 2;

	// Same as PATH_... but for navigating on los blocks
	static const int GRAPH_ITERATION_LIMIT_MULTIPLIER = 10;


	LogInfo("Trying to route (battle) from %s to %s", origin, destination);

	if (origin.x < 0 || origin.x >= this->size.x || origin.y < 0 || origin.y >= this->size.y ||
		origin.z < 0 || origin.z >= this->size.z)
	{
		LogError("Bad origin %s", origin);
		return{};
	}
	if (destination.x < 0 || destination.x >= this->size.x || destination.y < 0 ||
		destination.y >= this->size.y || destination.z < 0 || destination.z >= this->size.z)
	{
		LogError("Bad destination %s", destination);
		return{};
	}

	std::list<Vec3<int>> result;

	// Try to pathfind directly if close enough
	int distance = canEnterTile.getDistance(origin, destination) / 4.0f;
	if (distance < MAX_DISTANCE_TO_PATH_DIRECTLY)
	{
		result = map->findShortestPath(origin, destination, distance * PATH_ITERATION_LIMIT_MULTIPLIER, canEnterTile, ignoreStaticUnits, ignoreAllUnits, cost, maxCost);

		if ((*result.rbegin()) == destination)
		{
			return result;
		}
	}

	// Pathfind on graphs of los blocks
	int destLB = getLosBlockID(destination.x, destination.y, destination.z);
	auto pathLB = findLosBlockPath(getLosBlockID(origin.x, origin.y, origin.z), destLB, canEnterTile.getType());

	// If pathfinding on graphs failed - return short part of the path towards target
	if ((*pathLB.rbegin()) != destLB)
	{
		if (!result.empty())
		{
			return result;
		}
		else
		{
			return map->findShortestPath(origin, destination, distance * PATH_ITERATION_LIMIT_MULTIPLIER, canEnterTile, ignoreStaticUnits, ignoreAllUnits, cost, maxCost);
		}
	}

	//Debug output
	UString log;
	log += format("\nDebug output of los block path, count: %d", (int)pathLB.size());
	for (auto &i : pathLB)
	log += format("\n LB %d : %s - %s", (int)i, losBlocks[i]->start, losBlocks[i]->end);
	LogWarning("%s", log);

	// Pathfind using path among blocks
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
		auto path = map->findShortestPath(curOrigin, lb.start, lb.end, distToNext * GRAPH_ITERATION_LIMIT_MULTIPLIER, canEnterTile, ignoreStaticUnits, ignoreAllUnits, &curCost, curMaxCost);
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

	// Pathfind to destination

	auto curOrigin = *result.rbegin();
	float curCost = 0.0f;
	auto distToNext = canEnterTile.getDistance(curOrigin, destination) / 4.0f;
	auto path = map->findShortestPath(curOrigin, destination, distToNext * GRAPH_ITERATION_LIMIT_MULTIPLIER, canEnterTile, ignoreStaticUnits, ignoreAllUnits, &curCost, curMaxCost);
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

	return result;
}


std::list<int> Battle::findLosBlockPath(int origin, int destination, BattleUnitType type, int iterationLimit)
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
		return{ destination };
	}

	if (!blockAvailable[type][origin])
	{
		LogInfo("Origin unavailable!");
		return{ };
	}
	
	if (!blockAvailable[type][destination])
	{
		LogInfo("Destination unavailable!");
		return{};
	}
	
	auto startNode =
		new LosNode(0.0f, BattleUnitTileHelper::getDistanceStatic(blockCenterPos[type][origin], blockCenterPos[type][destination]), nullptr, origin);
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
				newNodeCost, 
				BattleUnitTileHelper::getDistanceStatic(blockCenterPos[type][j], blockCenterPos[type][destination]),
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
		LogInfo("Surprisingly, no nodes to expand! Closest path %d",
			closestNodeSoFar->block);
	}

	auto result = closestNodeSoFar->getPathToNode();
	
	for (auto &p : nodesToDelete)
		delete p;

	return result;
}

}

