#include "game/state/tilemap/tile.h"
#include "framework/image.h"
#include "game/state/battle/battledoor.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunit.h"
#include "game/state/city/scenery.h"
#include "game/state/city/vehicle.h"
#include "game/state/rules/battle/battleunitimagepack.h"
#include "game/state/rules/city/scenerytiletype.h"
#include "game/state/shared/doodad.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/collision.h"
#include "game/state/tilemap/tilemap.h"
#include "game/state/tilemap/tileobject_battlehazard.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/state/tilemap/tileobject_doodad.h"
#include "game/state/tilemap/tileobject_projectile.h"
#include "game/state/tilemap/tileobject_scenery.h"
#include "game/state/tilemap/tileobject_shadow.h"
#include "game/state/tilemap/tileobject_vehicle.h"
#include "library/sp.h"
#include <algorithm>
#include <random>
#include <unordered_map>

namespace OpenApoc
{

Tile::Tile(TileMap &map, Vec3<int> position, int layerCount)
    : map(map), position(position), drawnObjects(layerCount)
{
}

// Position for items and units to be located on
Vec3<float> Tile::getRestingPosition(bool large, bool overlay)
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
		float maxHeight = overlay ? overlayHeight : height;
		if (overlay)
		{
			maxHeight = std::max(
			    maxHeight, map.getTile(position.x - 1, position.y, position.z)->overlayHeight);
			maxHeight = std::max(
			    maxHeight, map.getTile(position.x, position.y - 1, position.z)->overlayHeight);
			maxHeight = std::max(
			    maxHeight, map.getTile(position.x - 1, position.y - 1, position.z)->overlayHeight);
		}
		else
		{
			maxHeight =
			    std::max(maxHeight, map.getTile(position.x - 1, position.y, position.z)->height);
			maxHeight =
			    std::max(maxHeight, map.getTile(position.x, position.y - 1, position.z)->height);
			maxHeight = std::max(maxHeight,
			                     map.getTile(position.x - 1, position.y - 1, position.z)->height);
		}

		return Vec3<float>{position.x, position.y, position.z + maxHeight};
	}
	return Vec3<float>{position.x + 0.5f, position.y + 0.5f,
	                   position.z + (overlay ? overlayHeight : height)};
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
	if (map.ceaseUpdates)
	{
		return;
	}
	bool backFound = false;
	bool targetFound = false;

	unsigned int object_count = (unsigned)drawnObjects[0].size();
	unsigned int obj_id;
	for (obj_id = 0; obj_id < object_count; obj_id++)
	{
		auto &obj = drawnObjects[0][obj_id];
		if (!backFound && obj->getType() != TileObject::Type::Ground)
		{
			drawBattlescapeSelectionBackAt = obj_id;
			backFound = true;
		}
		if (!targetFound && (int)obj->getType() > 3)
		{
			drawTargetLocationIconAt = obj_id;
			targetFound = true;
		}
	}
	if (!backFound)
	{
		drawBattlescapeSelectionBackAt = obj_id;
	}
	if (!targetFound)
	{
		drawTargetLocationIconAt = obj_id;
	}
}

void Tile::updateBattlescapeUnitPresent()
{
	firstUnitPresent = nullptr;
	for (auto &o : intersectingObjects)
	{
		if (o->getType() == TileObject::Type::Unit)
		{
			auto u = std::static_pointer_cast<TileObjectBattleUnit>(o);
			if (!firstUnitPresent)
			{
				firstUnitPresent = u;
			}
			auto pos = o->getPosition();
			auto x = pos.x - position.x;
			auto y = pos.y - position.y;
			doorOpeningUnitPresent =
			    doorOpeningUnitPresent | (x > 0.45f && x < 0.55f && y > 0.45f && y < 0.55f) ||
			    u->getUnit()->isLarge();
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

void Tile::updateCityscapeParameters()
{
	if (map.ceaseUpdates)
	{
		return;
	}
	height = 0.0f;
	overlayHeight = 0.0f;
	presentScenery = nullptr;
	for (auto &o : ownedObjects)
	{
		if (o->getType() == TileObject::Type::Scenery)
		{
			auto mp = std::static_pointer_cast<TileObjectScenery>(o)->getOwner();
			if (!mp->isAlive())
			{
				continue;
			}
			presentScenery = mp;
			height = (float)mp->type->height / 16.1f;
			overlayHeight = (float)mp->type->overlayHeight / 16.1f;
			break;
		}
	}
}

void Tile::updateBattlescapeParameters()
{
	if (map.ceaseUpdates)
	{
		return;
	}
	bool providedGroundUpwards = solidGround && height >= 0.9625f;
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
	closedDoorLeft = false;
	closedDoorRight = false;
	for (auto &o : ownedObjects)
	{
		if (o->getType() == TileObject::Type::Ground || o->getType() == TileObject::Type::Feature)
		{
			auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
			if (!mp->isAlive())
			{
				continue;
			}
			height = std::max(height, (float)mp->type->height);
			if (mp->type->floor ||
			    (o->getType() == TileObject::Type::Feature && !mp->type->gravlift))
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
				walkSfx = mp->type->walkSounds;
				objectDropSfx = mp->type->objectDropSound;
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
			closedDoorLeft = mp->door && !mp->door->open;
		}
		if (o->getType() == TileObject::Type::RightWall)
		{
			auto mp = std::static_pointer_cast<TileObjectBattleMapPart>(o)->getOwner();
			if (!mp->isAlive())
			{
				continue;
			}
			movementCostRight = mp->type->movement_cost;
			closedDoorRight = mp->door && !mp->door->open;
		}
	}
	canStand = solidGround || hasLift;
	if (!canStand && position.z > 0)
	{
		// check if tile below is full height
		auto t = map.getTile(position.x, position.y, position.z - 1);
		// Floating point precision is lacking sometimes so even though we have to compare with
		// 0.975, we do this
		canStand = t->solidGround && t->height >= 0.9625f;
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
	// Propagate update upwards if we provided ground and ceased to do so
	if (!(solidGround && height >= 0.9625f) && providedGroundUpwards && position.z + 1 < map.size.z)
	{
		auto t = map.getTile(position.x, position.y, position.z + 1);
		t->updateBattlescapeParameters();
	}
}

bool Tile::updateVisionBlockage(int value)
{
	if (visionBlockValue == value)
	{
		return false;
	}
	visionBlockValue = value;
	return true;
}

sp<TileObjectBattleUnit> Tile::getUnitIfPresent() const { return firstUnitPresent; }

sp<TileObjectBattleUnit> Tile::getUnitIfPresent(bool onlyConscious, bool mustOccupy,
                                                bool mustBeStatic,
                                                sp<TileObjectBattleUnit> exceptThis, bool onlyLarge,
                                                bool checkLargeSpace) const
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

	for (auto &o : intersectingObjects)
	{
		if (o->getType() == TileObject::Type::Unit)
		{
			auto unitTileObject = std::static_pointer_cast<TileObjectBattleUnit>(o);
			auto unit = unitTileObject->getUnit();
			if ((onlyConscious && !unit->isConscious()) || (exceptThis == unitTileObject) ||
			    (mustOccupy && unitTileObject->occupiedTiles.find(position) ==
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

std::list<sp<BattleUnit>> Tile::getUnits(bool onlyConscious, bool mustOccupy, bool mustBeStatic,
                                         sp<TileObjectBattleUnit> exceptThis, bool onlyLarge,
                                         bool checkLargeSpace) const
{
	std::list<sp<BattleUnit>> result;
	if (checkLargeSpace)
	{
		for (int x = -1; x <= 0; x++)
		{
			for (int y = -1; y <= 0; y++)
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
					auto uts = map.getTile(position.x + x, position.y + y, position.z + z)
					               ->getUnits(onlyConscious, mustOccupy, mustBeStatic, exceptThis,
					                          onlyLarge, false);
					for (auto &u : uts)
					{
						result.push_back(u);
					}
				}
			}
		}
	}

	for (auto &o : intersectingObjects)
	{
		if (o->getType() == TileObject::Type::Unit)
		{
			auto unitTileObject = std::static_pointer_cast<TileObjectBattleUnit>(o);
			auto unit = unitTileObject->getUnit();
			if ((onlyConscious && !unit->isConscious()) || (exceptThis == unitTileObject) ||
			    (mustOccupy && unitTileObject->occupiedTiles.find(position) ==
			                       unitTileObject->occupiedTiles.end()) ||
			    (mustBeStatic && !unit->isStatic()) || (onlyLarge && !unit->isLarge()))
			{
				continue;
			}
			result.push_back(unitTileObject->getUnit());
		}
	}
	return result;
}

std::list<sp<BattleItem>> Tile::getItems()
{
	std::list<sp<BattleItem>> result;
	for (auto &o : ownedObjects)
	{
		if (o->getType() == TileObject::Type::Item)
		{
			auto item = std::static_pointer_cast<TileObjectBattleItem>(o)->getItem();
			if (!item->falling)
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
			for (auto &o : t->ownedObjects)
			{
				if (o->getType() == TileObject::Type::Item)
				{
					auto item = std::static_pointer_cast<TileObjectBattleItem>(o)->getItem();
					if (!item->falling)
					{
						result.push_back(item);
					}
				}
			}
		}
	}
	return result;
}

}; // namespace OpenApoc
