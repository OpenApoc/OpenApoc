#include "framework/trace.h"
#include "game/state/tileview/tile.h"
#include <algorithm>

// Comments about this at the end of the file
#define NEW_PATHING_ALGORITHM

#ifdef NEW_PATHING_ALGORITHM

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

} // anonymous namespace

std::list<Vec3<int>> TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destination,
                                               unsigned int iterationLimit,
                                               const CanEnterTileHelper &canEnterTile,
											   bool demandGiveWay, float *cost, 
											   float maxCost)
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
	Vec3<float> goalPosition;
	unsigned int iterationCount = 0;

	LogInfo("Trying to route from {%d,%d,%d} to {%d,%d,%d}", origin.x, origin.y, origin.z,
	        destination.x, destination.y, destination.z);

	if (origin.x < 0 || origin.x >= this->size.x || origin.y < 0 || origin.y >= this->size.y ||
	    origin.z < 0 || origin.z >= this->size.z)
	{
		LogError("Bad origin {%d,%d,%d}", origin.x, origin.y, origin.z);
		return {};
	}
	if (destination.x < 0 || destination.x >= this->size.x || destination.y < 0 ||
	    destination.y >= this->size.y || destination.z < 0 || destination.z >= this->size.z)
	{
		LogError("Bad destination {%d,%d,%d}", destination.x, destination.y, destination.z);
		return {};
	}

	goalPosition = {destination.x, destination.y, destination.z};
	Tile *goalTile = this->getTile(destination);

	if (!goalTile)
	{
		LogError("Failed to get destination tile at {%d,%d,%d}", destination.x, destination.y,
		         destination.z);
		return {};
	}
	Tile *startTile = this->getTile(origin);
	if (!startTile)
	{
		LogError("Failed to get origin tile at {%d,%d,%d}", origin.x, origin.y, origin.z);
		return {};
	}

	if (origin == destination)
	{
		LogInfo("Destination == origin {%d,%d,%d}", destination.x, destination.y, destination.z);
		return {goalTile->position};
	}

	auto startNode =
	    new PathNode(0.0f, canEnterTile.getDistance(origin, goalPosition), nullptr, startTile);
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

		Vec3<int> currentPosition = nodeToExpand->thisTile->position;
		if (currentPosition == destination)
		{
			closestNodeSoFar = nodeToExpand;
			break;
		}

		if (nodeToExpand->distanceToGoal < closestNodeSoFar->distanceToGoal)
		{
			closestNodeSoFar = nodeToExpand;
		}
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
					if (!canEnterTile.canEnterTile(nodeToExpand->thisTile, tile, cost,
					                               demandGiveWay))
						continue;
					float newNodeCost = nodeToExpand->costToGetHere;

					if (maxCost > 0.0f && newNodeCost > maxCost)
						continue;

					newNodeCost += cost / canEnterTile.pathOverheadAlloawnce();

					// make pathfinder biased towards vehicle's altitude preference
					newNodeCost += canEnterTile.adjustCost(nextPosition, z);

					auto newNode = new PathNode(
					    newNodeCost , canEnterTile.getDistance(nextPosition, goalPosition),
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
		LogWarning("No route from {%d,%d,%d} to {%d,%d,%d} found after %d iterations, returning "
		           "closest path {%d,%d,%d}",
		           origin.x, origin.y, origin.z, destination.x, destination.y, destination.z,
		           iterationCount, closestNodeSoFar->thisTile->position.x,
		           closestNodeSoFar->thisTile->position.y, closestNodeSoFar->thisTile->position.z);
	}
	else if (closestNodeSoFar->distanceToGoal > 0)
	{
		if (maxCost > 0.0f)
		{
			LogInfo("Could not find path within maxPath, returning closest path {%d,%d,%d}", closestNodeSoFar->thisTile->position.x,
				closestNodeSoFar->thisTile->position.y, closestNodeSoFar->thisTile->position.z);
		}
		else
		{
			LogError("Surprisingly, no nodes to expand! Closest path {%d,%d,%d}", closestNodeSoFar->thisTile->position.x,
				closestNodeSoFar->thisTile->position.y, closestNodeSoFar->thisTile->position.z);
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
}

// endif NEW_PATHING_ALGORITHM
#else // OLD_PATHING_ALGORITHM

#include <unordered_map>

namespace OpenApoc
{
namespace
{
class PathNode
{
  public:
	PathNode(float costToGetHere, float distanceToGoal, Tile *parentTile, Tile *thisTile)
	    : costToGetHere(costToGetHere), parentTile(parentTile), thisTile(thisTile),
	      distanceToGoal(distanceToGoal)
	{
	}

	PathNode(float costToGetHere, float distanceToGoal, Tile *parentTile, Tile *thisTile,
	         const Vec3<float> &goal)
	    : costToGetHere(costToGetHere), parentTile(parentTile), thisTile(thisTile),
	      distanceToGoal(distanceToGoal)
	{
	}
	float costToGetHere;
	Tile *parentTile;
	Tile *thisTile;

	float distanceToGoal;
};

// class PathNodeComparer
//{
//  public:
//	bool operator()(const PathNode &p1, const PathNode &p2)
//	{
//		return (p1.costToGetHere + p1.distanceToGoal) < (p2.costToGetHere + p2.distanceToGoal);
//	}
//};

} // anonymous namespace

static std::list<Vec3<int>> getPathToNode(std::unordered_map<Tile *, PathNode> nodes, PathNode &end)
{
	std::list<Vec3<int>> path;
	path.push_back(end.thisTile->position);
	Tile *t = end.parentTile;
	while (t)
	{
		path.push_front(t->position);
		auto nextNodeIt = nodes.find(t);
		if (nextNodeIt == nodes.end())
		{
			LogError("Trying to expand unvisited node?");
			return {};
		}
		auto &nextNode = nextNodeIt->second;
		if (nextNode.thisTile != t)
		{
			LogError("Unexpected parentTile pointer");
			return {};
		}
		t = nextNode.parentTile;
	}

	return path;
}

std::list<Vec3<int>> TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destination,
                                               unsigned int iterationLimit,
                                               const CanEnterTileHelper &canEnterTile, float,
                                               bool demandGiveWay)
{
	TRACE_FN;
	std::unordered_map<Tile *, PathNode> visitedTiles;
	std::list<PathNode> fringe;
	Vec3<float> goalPosition;
	unsigned int iterationCount = 0;

	LogInfo("Trying to route from {%d,%d,%d} to {%d,%d,%d}", origin.x, origin.y, origin.z,
	        destination.x, destination.y, destination.z);

	if (origin.x < 0 || origin.x >= this->size.x || origin.y < 0 || origin.y >= this->size.y ||
	    origin.z < 0 || origin.z >= this->size.z)
	{
		LogError("Bad origin {%d,%d,%d}", origin.x, origin.y, origin.z);
		return {};
	}
	if (destination.x < 0 || destination.x >= this->size.x || destination.y < 0 ||
	    destination.y >= this->size.y || destination.z < 0 || destination.z >= this->size.z)
	{
		LogError("Bad destination {%d,%d,%d}", destination.x, destination.y, destination.z);
		return {};
	}

	goalPosition = {destination.x, destination.y, destination.z};
	Tile *goalTile = this->getTile(destination);

	if (!goalTile)
	{
		LogError("Failed to get destination tile at {%d,%d,%d}", destination.x, destination.y,
		         destination.z);
		return {};
	}
	Tile *startTile = this->getTile(origin);
	if (!startTile)
	{
		LogError("Failed to get origin tile at {%d,%d,%d}", origin.x, origin.y, origin.z);
		return {};
	}

	if (origin == destination)
	{
		LogInfo("Destination == origin {%d,%d,%d}", destination.x, destination.y, destination.z);
		return {goalTile->position};
	}

	PathNode startNode(0.0f, canEnterTile.getDistance(origin, goalPosition), nullptr, startTile,
	                   goalPosition);
	fringe.emplace_back(startNode);
	visitedTiles.emplace(startTile, startNode);

	auto closestNodeSoFar = *fringe.begin();

	while (iterationCount++ < iterationLimit)
	{
		auto first = fringe.begin();
		if (first == fringe.end())
		{
			LogInfo("No more tiles to expand after %d iterations", iterationCount);
			return {};
		}
		auto nodeToExpand = *first;
		fringe.erase(first);

		// Make it so we always try to move at least one tile
		if (closestNodeSoFar.parentTile == nullptr)
			closestNodeSoFar = nodeToExpand;

		Vec3<int> currentPosition = nodeToExpand.thisTile->position;
		if (currentPosition == destination)
		{
			return getPathToNode(visitedTiles, nodeToExpand);
		}

		if (nodeToExpand.distanceToGoal < closestNodeSoFar.distanceToGoal)
		{
			closestNodeSoFar = nodeToExpand;
		}
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
					// If Skip if we've already expanded this, as in a 3d-grid we know the first
					// expansion will be the shortest route
					if (visitedTiles.find(tile) != visitedTiles.end())
						continue;
					// FIXME: Make 'blocked' tiles cleverer (e.g. don't plan around objects that
					// will move anyway?)
					float cost = 0.0f;
					if (!canEnterTile.canEnterTile(nodeToExpand.thisTile, tile, cost,
					                               demandGiveWay))
						continue;
					// FIXME: The old code *tried* to disallow diagonal paths that would clip past
					// scenery but it didn't seem to work, no we should re-add that here
					float newNodeCost = nodeToExpand.costToGetHere;

					newNodeCost += cost;

					// make pathfinder biased towards vehicle's altitude preference
					newNodeCost += canEnterTile.adjustCost(nextPosition, z);

					PathNode newNode(newNodeCost,
					                 canEnterTile.applyPathOverheadAllowance(
					                     canEnterTile.getDistance(nextPosition, goalPosition)),
					                 nodeToExpand.thisTile, tile, goalPosition);
					// Allow entering goal multiple times, so that we can find a faster route
					if (nextPosition != destination)
					{
						visitedTiles.emplace(tile, newNode);
					}
					// Put node at appropriate place in the list
					auto it = fringe.begin();
					while (it != fringe.end() &&
					       (it->costToGetHere + it->distanceToGoal) <
					           (newNode.costToGetHere + newNode.distanceToGoal))
						it++;
					fringe.emplace(it, newNode);
				}
			}
		}
	}
	LogWarning("No route found after %d iterations, returning closest path {%d,%d,%d}",
	           iterationCount, closestNodeSoFar.thisTile->position.x,
	           closestNodeSoFar.thisTile->position.y, closestNodeSoFar.thisTile->position.z);
	return getPathToNode(visitedTiles, closestNodeSoFar);
}
}

#endif //  OLD_PATHING_ALGORITHM

// Alexey Andronov (Istrebitel)
//
// I have rewritten pathfinding algorithm using pointers instead of list of visited points
// This makes fixing problems with old algorithm easier
//
// One such problem was that we would add node to list of visited				    /---\
// at the moment we	first pathfind into it, and instead we should					|+  |
// do it when we pathfind from it. The way it worked, pathfinding                  2\-*-/
// would produce unoptimal paths, as demonstrated on the right:					  143**
//
//   (legend: "+" is goal, "|/\-" are walls)
// We find path into 1, and next we try 2, because 2 is closest to goal
// From 2 we try several tiles including 3. We add 3 to visited tiles.
// When eventually we try 4, we cannot use path from 4 to 3, because 3 is in visited tiles,
// even though "1-4-3" is cheaper than "1-2-3".
//
// In new algorithm, we add node to visited when we pathfind from it, therefore we can add
// tile into fringe multiple times, but this allows us to get better paths
//
// In case I have broken something horribly, I'm including old pathfinding algorithm here
// However, I'm certain new one should work properly.

// Ideas for improved pathfinding for battlescape
//
// I think I know how to improve battlescape pathfinding, and this is something close to what
// vanilla does with "connecting" LOS blocks.
// Basically, the game has every sector divided into blocks. And these usually correspond
// to landscape - meaning, usually a block is either passable inside, or full inside.
//
// This means we can create a graph which connects blocks with adjacent blocks
//
// We can then mark blocks that are not passable, defined as "cannot go from one wall to
// an opposite one in a reasonable amount of time".
//
// We can then check each block-to-block connection, and see if it exists,
// and if it requires a waypoint.
// A connection exists if you can go (canEnterTile) between adjacent tiles in adjacent blocks
// Waypoint is required if this is true only for several tiles, not all of them
// Waypoint is itself a block that encompasses all passable adjacent blocks
// (it can be just 1 tile, it can be a group of tiles)
//
// This will provide us with a waypoint graph.
// When trying to pathfind and unable to do so within reasonable ammount of attempts,
// we can check with the graph. We can first pathfind through the graph, and get a list of
// LOS blocks we must pass from start to goal. We ignore impassable blocks and connections.
// Then we can take all waypoints we encountered and path through them
// (path to closest tile within first waypoint, then closest tile withing second waypoint, etc.
// until we reach final waypoint, then path to goal)