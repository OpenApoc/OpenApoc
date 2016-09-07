#include "game/state/battle.h"
#include "game/state/battletile.h"
#include "game/state/battletileobject_mappart.h"
#include "game/state/tileview/voxel.h"
#include <random>

namespace OpenApoc
{
BattleTile::BattleTile(BattleTileMap &map, Vec3<int> position, int layerCount)
    : map(map), position(position), drawnObjects(layerCount)
{
}

BattleTileMap::BattleTileMap(Vec3<int> size, std::vector<std::set<BattleTileObject::Type>> layerMap)
    : layerMap(layerMap), size(size)
{
	tiles.reserve(size.x * size.y * size.z);
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
	std::set<BattleTileObject::Type> seenTypes;
	for (auto &typesInLayer : layerMap)
	{
		for (auto &type : typesInLayer)
		{
			if (seenTypes.find(type) != seenTypes.end())
			{
				LogError("Type %d appears in multiple layers", static_cast<int>(type));
			}
			seenTypes.insert(type);
		}
	}
	// In order for selectionBracket to be drawn properly, first layer must contain all mapparts's
	// and the unit's types
	if (layerMap[0].find(BattleTileObject::Type::Ground) == layerMap[0].end() ||
	    layerMap[0].find(BattleTileObject::Type::LeftWall) == layerMap[0].end() ||
	    layerMap[0].find(BattleTileObject::Type::RightWall) == layerMap[0].end() ||
	    layerMap[0].find(BattleTileObject::Type::Scenery) == layerMap[0].end() ||
	    layerMap[0].find(BattleTileObject::Type::Unit) == layerMap[0].end())
		LogError("Layer 0 for battlescape is not filled properly");
}

BattleTileMap::~BattleTileMap() = default;

void BattleTileMap::addObjectToMap(sp<BattleMapPart> map_part)
{
	if (map_part->tileObject)
	{
		LogError("Map part already has tile object");
	}
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<BattleTileObjectMapPart> obj(new BattleTileObjectMapPart(*this, map_part));
	obj->setPosition(map_part->getPosition());
	map_part->tileObject = obj;
}

int BattleTileMap::getLayer(BattleTileObject::Type type) const
{
	for (unsigned i = 0; i < this->layerMap.size(); i++)
	{
		if (this->layerMap[i].find(type) != this->layerMap[i].end())
		{
			return i;
		}
	}
	LogError("No layer matching object type %d", static_cast<int>(type));
	return 0;
}

int BattleTileMap::getLayerCount() const { return this->layerMap.size(); }

bool BattleTileMap::tileIsValid(Vec3<int> tile) const
{
	if (tile.z < 0 || tile.z >= this->size.z || tile.y < 0 || tile.y >= this->size.y ||
	    tile.x < 0 || tile.x >= this->size.x)
		return false;
	return true;
}

sp<Image> BattleTileMap::dumpVoxelView(const Rect<int> viewRect,
                                       const TileTransform &transform) const
{
	auto img = mksp<RGBImage>(viewRect.size());
	std::map<sp<BattleTileObject>, Colour> objectColours;
	std::default_random_engine colourRNG;
	// MSVC doesn't like uint8_t being the type for uniform_int_distribution?
	std::uniform_int_distribution<int> colourDist(0, 255);

	RGBImageLock lock(img);
	int h = viewRect.p1.y - viewRect.p0.y;
	int w = viewRect.p1.x - viewRect.p0.x;
	Vec2<float> offset = {viewRect.p0.x, viewRect.p0.y};

	LogWarning("ViewRect {%d,%d},{%d,%d}", viewRect.p0.x, viewRect.p0.y, viewRect.p1.x,
	           viewRect.p1.y);

	LogWarning("Dumping voxels {%d,%d} voxels w/offset {%f,%f}", w, h, offset.x, offset.y);

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			auto topPos = transform.screenToTileCoords(Vec2<float>{x, y} + offset, 9.99f);
			auto bottomPos = transform.screenToTileCoords(Vec2<float>{x, y} + offset, 0.0f);

			auto collision = this->findCollision(topPos, bottomPos);
			if (collision)
			{
				if (objectColours.find(collision.obj) == objectColours.end())
				{
					Colour c = {static_cast<uint8_t>(colourDist(colourRNG)),
					            static_cast<uint8_t>(colourDist(colourRNG)),
					            static_cast<uint8_t>(colourDist(colourRNG)), 255};
					objectColours[collision.obj] = c;
				}
				lock.set({x, y}, objectColours[collision.obj]);
			}
		}
	}

	return img;
}
}