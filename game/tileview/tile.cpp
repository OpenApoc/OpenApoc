#include "library/sp.h"
#include "game/tileview/tile.h"
#include "framework/framework.h"
#include "framework/trace.h"
#include "game/tileview/tileobject_projectile.h"
#include "game/city/projectile.h"
#include "game/tileview/tileobject_vehicle.h"
#include "game/tileview/tileobject_shadow.h"
#include "game/city/vehicle.h"
#include "game/tileview/tileobject_scenery.h"
#include "game/city/scenery.h"
#include "game/tileview/tileobject_doodad.h"
#include "game/city/doodad.h"

#include <unordered_map>

namespace OpenApoc
{

TileMap::TileMap(Framework &fw, Vec3<int> size, std::vector<std::set<TileObject::Type>> layerMap)
    : layerMap(layerMap), fw(fw), size(size)
{
	tiles.reserve(size.z * size.y * size.z);
	for (int z = 0; z < size.z; z++)
	{
		for (int y = 0; y < size.y; y++)
		{
			for (int x = 0; x < size.x; x++)
			{
				tiles.emplace_back(*this, Vec3<int>{x, y, z}, this->getLayerCount());
			}
		}
	}

	// Quick sanity check of the layer map:
	std::set<TileObject::Type> seenTypes;
	for (auto &typesInLayer : layerMap)
	{
		for (auto &type : typesInLayer)
		{
			if (seenTypes.find(type) != seenTypes.end())
			{
				LogError("Type %d appears in multiple layers", (int)type);
			}
			seenTypes.insert(type);
		}
	}
}

Tile *TileMap::getTile(int x, int y, int z)
{
	if (x >= size.x || y >= size.y || z >= size.z)
	{
		LogError("Requesting tile {%d,%d,%d} in map of size {%d,%d,%d}", x, y, z, size.x, size.y,
		         size.z);
		return nullptr;
	}
	return &this->tiles[z * size.x * size.y + y * size.x + x];
}

Tile *TileMap::getTile(Vec3<int> pos) { return getTile(pos.x, pos.y, pos.z); }

Tile *TileMap::getTile(Vec3<float> pos) { return getTile(pos.x, pos.y, pos.z); }

TileMap::~TileMap() {}

Tile::Tile(TileMap &map, Vec3<int> position, int layerCount)
    : map(map), position(position), drawnObjects(layerCount)
{
}

namespace
{
class PathNode
{
  public:
	PathNode(float costToGetHere, Tile *parentTile, Tile *thisTile, const Vec3<float> &goal)
	    : costToGetHere(costToGetHere), parentTile(parentTile), thisTile(thisTile)
	{
		Vec3<float> thisPosition{(float)thisTile->position.x, (float)thisTile->position.y,
		                         (float)thisTile->position.z};
		Vec3<float> vectorToGoal = (goal - thisPosition);
		this->distanceToGoal = glm::length(vectorToGoal);
	}
	float costToGetHere;
	Tile *parentTile;
	Tile *thisTile;

	float distanceToGoal;
};

class PathNodeComparer
{
  public:
	bool operator()(const PathNode &p1, const PathNode &p2)
	{
		return (p1.costToGetHere + p1.distanceToGoal) < (p2.costToGetHere + p2.distanceToGoal);
	}
};

} // anonymous namespace

static std::list<Tile *> getPathToNode(std::unordered_map<Tile *, PathNode> nodes, PathNode &end)
{
	std::list<Tile *> path;
	path.push_back(end.thisTile);
	Tile *t = end.parentTile;
	while (t)
	{
		path.push_front(t);
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

std::list<Tile *> TileMap::findShortestPath(Vec3<int> origin, Vec3<int> destination,
                                            unsigned int iterationLimit,
                                            const CanEnterTileHelper &canEnterTile)
{
	TRACE_FN;
	PathNodeComparer c;
	std::unordered_map<Tile *, PathNode> visitedTiles;
	std::set<PathNode, PathNodeComparer> fringe(c);
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
		return {goalTile};
	}

	PathNode startNode(0.0f, nullptr, startTile, goalPosition);
	fringe.emplace(startNode);
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
			return getPathToNode(visitedTiles, nodeToExpand);

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
					if (nextPosition.z < 0 || nextPosition.z >= this->size.z ||
					    nextPosition.y < 0 || nextPosition.y >= this->size.y ||
					    nextPosition.x < 0 || nextPosition.x >= this->size.x)
						continue;
					Tile *tile = this->getTile(nextPosition);
					// If Skip if we've already expanded this, as in a 3d-grid we know the first
					// expansion will be the shortest route
					if (visitedTiles.find(tile) != visitedTiles.end())
						continue;
					// FIXME: Make 'blocked' tiles cleverer (e.g. don't plan around objects that
					// will
					// move anyway?)
					if (!canEnterTile.canEnterTile(nodeToExpand.thisTile, tile))
						continue;
					// FIXME: The old code *tried* to disallow diagonal paths that would clip past
					// scenery but it didn't seem to work, no we should re-add that here
					float newNodeCost = nodeToExpand.costToGetHere;

					newNodeCost +=
					    glm::length(Vec3<float>{nextPosition} - Vec3<float>{currentPosition});

					PathNode newNode(newNodeCost, nodeToExpand.thisTile, tile, goalPosition);
					visitedTiles.emplace(tile, newNode);
					fringe.emplace(newNode);
				}
			}
		}
	}
	LogInfo("No route found after %d iterations, returning closest path {%d,%d,%d}", iterationCount,
	        closestNodeSoFar.thisTile->position.x, closestNodeSoFar.thisTile->position.y,
	        closestNodeSoFar.thisTile->position.z);
	return getPathToNode(visitedTiles, closestNodeSoFar);
}

void TileMap::addObjectToMap(sp<Projectile> projectile)
{
	if (projectile->tileObject)
	{
		LogError("Projectile already has tile object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectProjectile> obj(new TileObjectProjectile(*this, projectile));
	obj->setPosition(projectile->getPosition());
	projectile->tileObject = obj;
}

void TileMap::addObjectToMap(sp<Vehicle> vehicle)
{
	if (vehicle->tileObject)
	{
		LogError("Vehicle already has tile object");
	}
	if (vehicle->shadowObject)
	{
		LogError("Vehicle already has shadow object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectVehicle> obj(new TileObjectVehicle(*this, vehicle));
	obj->setPosition(vehicle->getPosition());
	vehicle->tileObject = obj;

	sp<TileObjectShadow> shadow(new TileObjectShadow(*this, vehicle));
	shadow->setPosition(vehicle->getPosition());
	vehicle->shadowObject = shadow;
}

void TileMap::addObjectToMap(sp<Scenery> scenery)
{
	if (scenery->tileObject)
	{
		LogError("Scenery already has tile object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectScenery> obj(new TileObjectScenery(*this, scenery));
	obj->setPosition(scenery->getPosition());
	scenery->tileObject = obj;
}

void TileMap::addObjectToMap(sp<Doodad> doodad)
{
	if (doodad->tileObject)
	{
		LogError("Doodad already has tile object");
	}
	// FIXME: std::make_shared<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectDoodad> obj(new TileObjectDoodad(*this, doodad));
	obj->setPosition(doodad->getPosition());
	doodad->tileObject = obj;
}

int TileMap::getLayer(TileObject::Type type) const
{
	for (int i = 0; i < this->layerMap.size(); i++)
	{
		if (this->layerMap[i].find(type) != this->layerMap[i].end())
		{
			return i;
		}
	}
	LogError("No layer matching object type %d", (int)type);
	return 0;
}

int TileMap::getLayerCount() const { return this->layerMap.size(); }

}; // namespace OpenApoc
