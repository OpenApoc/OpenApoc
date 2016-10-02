#include "game/state/tileview/tile.h"
#include "framework/image.h"
#include "framework/trace.h"
#include "game/state/city/doodad.h"
#include "game/state/city/projectile.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/tileview/collision.h"
#include "game/state/tileview/tileobject_battleitem.h"
#include "game/state/tileview/tileobject_battlemappart.h"
#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/tileview/tileobject_doodad.h"
#include "game/state/tileview/tileobject_projectile.h"
#include "game/state/tileview/tileobject_scenery.h"
#include "game/state/tileview/tileobject_shadow.h"
#include "game/state/tileview/tileobject_vehicle.h"
#include "library/sp.h"
#include <algorithm>
#include <random>
#include <unordered_map>

namespace OpenApoc
{

TileMap::TileMap(Vec3<int> size, Vec3<float> velocityScale, Vec3<int> voxelMapSize,
                 std::vector<std::set<TileObject::Type>> layerMap)
    : layerMap(layerMap), size(size), voxelMapSize(voxelMapSize), velocityScale(velocityScale)
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
	std::set<TileObject::Type> seenTypes;
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
}

TileMap::~TileMap() = default;

Tile::Tile(TileMap &map, Vec3<int> position, int layerCount)
    : map(map), position(position), drawnObjects(layerCount)
{
}

// Position for items and units to be located on
Vec3<float> Tile::getRestingPosition(bool large)
{
	if (large)
	{
		if (position.x < 1 || position.y < 1)
		{
			LogError(
			    "Trying to get resting position for a large unit when it can't fit! %d, %d, %d",
			    position.x, position.y, position.z);
			return Vec3<float>{position.x + 0.5, position.y + 0.5, position.z};
		}
		float maxHeight = height;
		maxHeight =
		    std::max(maxHeight, map.getTile(position.x - 1, position.y, position.z)->height);
		maxHeight =
		    std::max(maxHeight, map.getTile(position.x, position.y - 1, position.z)->height);
		maxHeight =
		    std::max(maxHeight, map.getTile(position.x - 1, position.y - 1, position.z)->height);

		return Vec3<float>{position.x, position.y, position.z + maxHeight};
	}
	return Vec3<float>{position.x + 0.5, position.y + 0.5, position.z + height};
}

sp<BattleMapPart> Tile::getItemSupportingObject() { return supportProviderForItems; }

bool Tile::getSolidGround(bool large)
{
	if (large)
	{
		if (position.x < 1 || position.y < 1)
		{
			LogError("Trying to get solid ground for a large unit when it can't fit! %d, %d, %d",
			         position.x, position.y, position.z);
			return false;
		}
		if (solidGround || map.getTile(position.x - 1, position.y, position.z)->solidGround ||
		    map.getTile(position.x, position.y - 1, position.z)->solidGround ||
		    map.getTile(position.x - 1, position.y - 1, position.z)->solidGround)
		{
			return true;
		}
	}
	else
	{
		if (solidGround)
		{
			return true;
		}
	}
	return false;
}

bool Tile::getCanStand(bool large)
{
	if (large)
	{
		if (position.x < 1 || position.y < 1)
		{
			LogError(
			    "Trying to get standing ability for a large unit when it can't fit! %d, %d, %d",
			    position.x, position.y, position.z);
			return false;
		}
		if (canStand || map.getTile(position.x - 1, position.y, position.z)->canStand ||
		    map.getTile(position.x, position.y - 1, position.z)->canStand ||
		    map.getTile(position.x - 1, position.y - 1, position.z)->canStand)
		{
			return true;
		}
	}
	else
	{
		if (canStand)
		{
			return true;
		}
	}
	return false;
}

bool Tile::getHasExit(bool large)
{
	if (large)
	{
		if (position.x < 1 || position.y < 1)
		{
			LogError("Trying to get exit for a large unit when it can't fit! %d, %d, %d",
			         position.x, position.y, position.z);
			return false;
		}
		if (hasExit || map.getTile(position.x - 1, position.y, position.z)->hasExit ||
		    map.getTile(position.x, position.y - 1, position.z)->hasExit ||
		    map.getTile(position.x - 1, position.y - 1, position.z)->hasExit)
		{
			return true;
		}
	}
	else
	{
		if (hasExit)
		{
			return true;
		}
	}
	return false;
}

bool Tile::getPassable(bool large, int height)
{
	if (movementCostIn == 255)
		return false;

	if (large)
	{
		if (movementCostLeft == 255)
			return false;
		if (movementCostRight == 255)
			return false;

		if (position.x < 1 || position.y < 1 || position.z >= map.size.z - 1)
		{
			return false;
		}

		auto tX = map.getTile(position.x - 1, position.y, position.z);
		if (tX->movementCostIn == 255)
			return false;
		if (tX->movementCostRight == 255)
			return false;

		auto tY = map.getTile(position.x, position.y - 1, position.z);
		if (tY->movementCostIn == 255)
			return false;
		if (tY->movementCostLeft == 255)
			return false;

		if (map.getTile(position.x - 1, position.y - 1, position.z)->movementCostIn == 255)
			return false;

		auto tZ = map.getTile(position.x, position.y, position.z + 1);
		if (tZ->movementCostIn == 255)
			return false;
		if (tZ->movementCostLeft == 255)
			return false;
		if (tZ->movementCostRight == 255)
			return false;

		auto tXZ = map.getTile(position.x - 1, position.y, position.z + 1);
		if (tXZ->movementCostIn == 255)
			return false;
		if (tXZ->movementCostRight == 255)
			return false;

		auto tYZ = map.getTile(position.x, position.y - 1, position.z + 1);
		if (tYZ->movementCostIn == 255)
			return false;
		if (tYZ->movementCostLeft == 255)
			return false;

		if (map.getTile(position.x - 1, position.y - 1, position.z + 1)->movementCostIn == 255)
			return false;
	}

	return height == 0 || getHeadFits(large, height);
}

bool Tile::getHeadFits(bool large, int height)
{
	if (position.z + (large ? 2 : 1) >= map.size.z)
		return true;
	if (large)
	{
		// Check four tiles above our "to"'s head
		if (map.getTile(Vec3<int>{position.x, position.y, position.z + 2})->solidGround ||
		    map.getTile(Vec3<int>{position.x - 1, position.y, position.z + 2})->solidGround ||
		    map.getTile(Vec3<int>{position.x, position.y - 1, position.z + 2})->solidGround ||
		    map.getTile(Vec3<int>{position.x - 1, position.y - 1, position.z + 2})->solidGround)
		{
			auto toX1 = map.getTile(Vec3<int>{position.x - 1, position.y, position.z});
			auto toY1 = map.getTile(Vec3<int>{position.x, position.y - 1, position.z});
			auto toXY1 = map.getTile(Vec3<int>{position.x - 1, position.y - 1, position.z});

			// If we have solid ground upon arriving - check if we fit
			float maxHeight = this->height;
			maxHeight = std::max(maxHeight, toX1->height);
			maxHeight = std::max(maxHeight, toY1->height);
			maxHeight = std::max(maxHeight, toXY1->height);
			if (height + maxHeight * 40 - 1 > 80)
			{
				return false;
			}
		}
	}
	else
	{
		if (map.getTile(Vec3<int>{position.x, position.y, position.z + 1})->solidGround &&
		    height + this->height * 40 - 1 > 40)
		{
			return false;
		}
	}
	return true;
}

void Tile::updateBattlescapeUIDrawOrder()
{
	drawBattlescapeSelectionBackAt = -1;
	drawTargetLocationIconAt = -1;

	auto object_count = drawnObjects[0].size();
	size_t obj_id;
	for (obj_id = 0; obj_id < object_count; obj_id++)
	{
		auto &obj = drawnObjects[0][obj_id];
		if (drawBattlescapeSelectionBackAt == -1 && obj->getType() != TileObject::Type::Ground)
		{
			drawBattlescapeSelectionBackAt = obj_id;
		}
		if (drawTargetLocationIconAt == -1 && (int)obj->getType() > 3)
		{
			drawTargetLocationIconAt = obj_id;
		}
	}
	if (drawBattlescapeSelectionBackAt == -1)
	{
		drawBattlescapeSelectionBackAt = obj_id;
	}
	if (drawTargetLocationIconAt == -1)
	{
		drawTargetLocationIconAt = obj_id;
	}
}

void Tile::updateBattlescapeUnitPresent()
{
	firstUnitPresent = nullptr;
	for (auto o : intersectingObjects)
	{
		if (o->getType() == TileObject::Type::Unit)
		{
			if (!firstUnitPresent)
			{
				firstUnitPresent = std::static_pointer_cast<TileObjectBattleUnit>(o);
			}
			auto pos = o->getPosition();
			auto x = pos.x - position.x;
			auto y = pos.y - position.y;
			doorOpeningUnitPresent =
			    doorOpeningUnitPresent || (x > 0.45f && x < 0.55f && y > 0.45f && y < 0.55f);
			if (firstUnitPresent && doorOpeningUnitPresent)
			{
				break;
			}
		}
	}
	if (!firstUnitPresent)
	{
		doorOpeningUnitPresent = false;
	}
}

void Tile::updateBattlescapeParameters()
{
	height = 0.0f;
	movementCostIn = -1; // -1 means empty, and will be set to 4 afterwards
	movementCostOver = 255;
	movementCostLeft = 0;
	movementCostRight = 0;
	solidGround = false;
	canStand = false;
	hasLift = false;
	hasExit = false;
	walkSfx = nullptr;
	objectDropSfx = nullptr;
	supportProviderForItems = nullptr;
	bool groundEncountered = false;
	closedDoorLeft = false;
	closedDoorRight = false;
	for (auto o : ownedObjects)
	{
		if (o->getType() == TileObject::Type::Ground || o->getType() == TileObject::Type::Feature)
		{
			auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
			if (!mp->isAlive())
			{
				continue;
			}
			height = std::max(height, (float)mp->type->height);
			if ((mp->type->floor || o->getType() == TileObject::Type::Feature) &&
			    !mp->type->gravlift)
			{
				if (!supportProviderForItems ||
				    supportProviderForItems->type->height < mp->type->height)
				{
					supportProviderForItems = mp;
				}
			}
			solidGround = solidGround || (mp->type->floor && !mp->type->gravlift) ||
			              (o->getType() == TileObject::Type::Feature && !mp->type->gravlift);
			hasLift = hasLift || mp->type->gravlift;
			hasExit = hasExit || mp->type->exit;
			movementCostIn = std::max(movementCostIn, mp->type->movement_cost);
			if (mp->type->sfxIndex != -1)
			{
				auto b = mp->battle;
				if (b)
				{
					walkSfx = b->common_sample_list->walkSounds[mp->type->sfxIndex];
					objectDropSfx = b->common_sample_list->objectDropSounds[mp->type->sfxIndex];
				}
			}
		}
		if (o->getType() == TileObject::Type::LeftWall)
		{
			auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
			if (!mp->isAlive())
			{
				continue;
			}
			movementCostLeft = mp->type->movement_cost;
			closedDoorLeft = mp->isDoor() && !mp->getDoor()->open;
		}
		if (o->getType() == TileObject::Type::RightWall)
		{
			auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
			if (!mp->isAlive())
			{
				continue;
			}
			movementCostRight = mp->type->movement_cost;
			closedDoorRight = mp->isDoor() && !mp->getDoor()->open;
		}
	}
	canStand = solidGround || hasLift;
	if (!canStand && position.z > 0)
	{
		// check if tile below is full height
		auto t = map.getTile(position.x, position.y, position.z - 1);
		// Floating point precision is lacking somtimes so even though we have to compare with
		// 0.975, we do this
		canStand = t->solidGround && (t->height >= 0.9625f);
		if (canStand)
		{
			movementCostIn = std::max(movementCostIn, t->movementCostOver);
		}
	}
	if (movementCostIn == -1)
	{
		movementCostIn = 4;
	}
	// Height of 39 means whole tile
	if (height == 39 && solidGround)
	{
		movementCostOver = movementCostIn;
		movementCostIn = 255;
		// Provide solid ground upwards
		if (position.z + 1 < map.size.z)
		{
			auto t = map.getTile(position.x, position.y, position.z + 1);
			if (!t->canStand)
			{
				t->canStand = true;
				t->movementCostIn = std::max(movementCostOver, t->movementCostIn);
			}
		}
	}
	height = height / (float)TILE_Z_BATTLE;
}

sp<TileObjectBattleUnit> Tile::getUnitIfPresent() { return firstUnitPresent; }

sp<TileObjectBattleUnit> Tile::getUnitIfPresent(bool onlyConscious, bool mustOccupy,
                                                bool mustBeStatic,
                                                sp<TileObjectBattleUnit> exceptThis, bool onlyLarge,
                                                bool checkLargeSpace)
{
	if (checkLargeSpace)
	{
		for (int x = -1; x >= 0; x++)
		{
			for (int y = -1; y >= 0; y++)
			{
				for (int z = 0; z <= 1; z++)
				{
					if (x == 0 && y == 0 && z == 0)
					{
						continue;
					}
					if (position.x + x < 0 || position.y + y < 0 || position.z + z >= map.size.z)
					{
						continue;
					}
					auto u = map.getTile(position.x + x, position.y + y, position.z + z)
					             ->getUnitIfPresent(onlyConscious, mustOccupy, mustBeStatic,
					                                exceptThis, onlyLarge, false);
					if (u)
					{
						return u;
					}
				}
			}
		}
	}

	for (auto o : intersectingObjects)
	{
		if (o->getType() == TileObject::Type::Unit)
		{
			auto unitTileObject = std::static_pointer_cast<TileObjectBattleUnit>(o);
			auto unit = unitTileObject->getUnit();
			if ((onlyConscious && !unit->isConscious()) || (exceptThis == unitTileObject) ||
			    (mustOccupy &&
			     unitTileObject->occupiedTiles.find(position) ==
			         unitTileObject->occupiedTiles.end()) ||
			    (mustBeStatic && !unit->isStatic()) || (onlyLarge && !unit->isLarge()))
			{
				continue;
			}
			return unitTileObject;
		}
	}
	return nullptr;
}

std::list<sp<BattleItem>> Tile::getItems()
{
	std::list<sp<BattleItem>> result;
	for (auto o : ownedObjects)
	{
		if (o->getType() == TileObject::Type::Item)
		{
			auto item = std::static_pointer_cast<TileObjectBattleItem>(o)->getItem();
			if (item->supported)
			{
				result.push_back(item);
			}
		}
	}
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			if (x == 0 && y == 0)
			{
				continue;
			}
			auto pos = Vec3<int>{position.x + x, position.y + y, position.z};
			if (pos.x < 0 || pos.x >= map.size.x || pos.y < 0 || pos.y >= map.size.y)
				continue;
			auto t = map.getTile(pos);
			for (auto o : t->ownedObjects)
			{
				if (o->getType() == TileObject::Type::Item)
				{
					auto item = std::static_pointer_cast<TileObjectBattleItem>(o)->getItem();
					if (item->supported)
					{
						result.push_back(item);
					}
				}
			}
		}
	}
	return result;
}

void TileMap::addObjectToMap(sp<Projectile> projectile)
{
	if (projectile->tileObject)
	{
		LogError("Projectile already has tile object");
	}
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
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
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
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
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
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
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectDoodad> obj(new TileObjectDoodad(*this, doodad));
	obj->setPosition(doodad->getPosition());
	doodad->tileObject = obj;
}

void TileMap::addObjectToMap(sp<BattleMapPart> map_part)
{
	if (map_part->tileObject)
	{
		LogError("Map part already has tile object");
	}
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectBattleMapPart> obj(new TileObjectBattleMapPart(*this, map_part));
	obj->setPosition(map_part->getPosition());
	map_part->tileObject = obj;
}

void TileMap::addObjectToMap(sp<BattleItem> item)
{
	if (item->tileObject)
	{
		LogError("Item already has tile object");
	}
	if (item->shadowObject)
	{
		LogError("Item already has shadow object");
	}
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectBattleItem> obj(new TileObjectBattleItem(*this, item));
	obj->setPosition(item->getPosition());
	item->tileObject = obj;

	sp<TileObjectShadow> shadow(new TileObjectShadow(*this, item));
	shadow->setPosition(item->getPosition());
	item->shadowObject = shadow;
}

void TileMap::addObjectToMap(sp<BattleUnit> unit)
{
	if (unit->tileObject)
	{
		LogError("Unit already has tile object");
	}
	if (unit->shadowObject)
	{
		LogError("Unit already has shadow object");
	}
	// FIXME: mksp<> doesn't work for private (but accessible due to friend)
	// constructors?
	sp<TileObjectBattleUnit> obj(new TileObjectBattleUnit(*this, unit));
	obj->setPosition(unit->getPosition());
	unit->tileObject = obj;

	if (!unit->agent->type->shadow_pack)
		return;

	sp<TileObjectShadow> shadow(new TileObjectShadow(*this, unit));
	shadow->setPosition(unit->getPosition());
	unit->shadowObject = shadow;
}

int TileMap::getLayer(TileObject::Type type) const
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

int TileMap::getLayerCount() const { return this->layerMap.size(); }

bool TileMap::tileIsValid(Vec3<int> tile) const
{
	if (tile.z < 0 || tile.z >= this->size.z || tile.y < 0 || tile.y >= this->size.y ||
	    tile.x < 0 || tile.x >= this->size.x)
		return false;
	return true;
}

sp<Image> TileMap::dumpVoxelView(const Rect<int> viewRect, const TileTransform &transform,
                                 float maxZ, bool fast) const
{
	auto img = mksp<RGBImage>(viewRect.size());
	std::map<sp<TileObject>, Colour> objectColours;
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

	int inc = fast ? 2 : 1;

	for (int y = 0; y < h; y += inc)
	{
		for (int x = 0; x < w; x += inc)
		{
			auto topPos = transform.screenToTileCoords(Vec2<float>{x, y} + offset, maxZ - 0.01f);
			auto bottomPos = transform.screenToTileCoords(Vec2<float>{x, y} + offset, 0.0f);

			auto collision = this->findCollision(topPos, bottomPos, {}, false, true);
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
				if (fast)
				{
					lock.set({x + 1, y}, objectColours[collision.obj]);
					lock.set({x, y + 1}, objectColours[collision.obj]);
					lock.set({x + 1, y + 1}, objectColours[collision.obj]);
				}
			}
		}
	}

	return img;
}

}; // namespace OpenApoc
