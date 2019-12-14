#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "game/state/tilemap/tileobject_battleunit.h"
#include "framework/renderer.h"
#include "game/state/battle/battleunit.h"
#include "game/state/rules/battle/battleunitanimationpack.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/tilemap/tilemap.h"
#include "library/voxel.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace OpenApoc
{

void TileObjectBattleUnit::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                                TileViewMode mode, bool visible, int currentLevel, bool friendly,
                                bool hostile)
{
	// We never draw non-visible units? Or maybe we do?
	if (!visible)
		return;

	static const int offset_prone = 8;
	static const int offset_large = 32;

	static const std::map<Vec2<int>, int> offset_dir_map = {
	    {{0, -1}, 0}, {{1, -1}, 1}, {{1, 0}, 2},  {{1, 1}, 3},
	    {{0, 1}, 4},  {{-1, 1}, 5}, {{-1, 0}, 6}, {{-1, -1}, 7},
	};

	static const std::map<int, int> offset_prone_map = {
	    {0, offset_prone + 0},  {1, offset_prone + 2},  {2, offset_prone + 6},
	    {3, offset_prone + 8},  {4, offset_prone + 12}, {5, offset_prone + 14},
	    {6, offset_prone + 18}, {7, offset_prone + 20},
	};

	static const int ICON_STANDART = 0;
	static const int ICON_PRONE = 1;
	static const int ICON_LARGE = 2;

	std::ignore = transform;
	auto unit = getUnit();
	if (!unit)
	{
		LogError("Called with no owning unit object");
		return;
	}
	switch (mode)
	{
		case TileViewMode::Isometric:
		{
			int firingAngle = 0;
			if (unit->current_hand_state == HandState::Firing ||
			    unit->target_hand_state == HandState::Aiming)
			{
				Vec3<float> targetVector = unit->targetTile - (Vec3<int>)unit->position -
				                           Vec3<int>{0, 0, unit->isLarge() ? 1 : 0};
				Vec3<float> targetVectorZeroZ = {targetVector.x, targetVector.y, 0.0f};
				// Firing angle is 0 for -10..10, +-1  for -20..-10 and 10..20, and 2 for else
				firingAngle = (int)((glm::angle(glm::normalize(targetVector),
				                                glm::normalize(targetVectorZeroZ)) *
				                     360.0f / 2.0f / M_PI) /
				                    10.0f);
				if (targetVector.z < 0)
				{
					firingAngle = -firingAngle;
				}
				firingAngle = clamp(firingAngle, -2, 2);
			}
			unit->agent->getAnimationPack()->drawUnit(
			    r, screenPosition, unit->agent->getImagePack(BodyPart::Body),
			    unit->agent->getImagePack(BodyPart::Legs),
			    unit->agent->getImagePack(BodyPart::Helmet),
			    unit->agent->getImagePack(BodyPart::LeftArm),
			    unit->agent->getImagePack(BodyPart::RightArm), unit->displayedItem, unit->facing,
			    unit->current_body_state, unit->target_body_state, unit->current_hand_state,
			    unit->target_hand_state,
			    unit->usingLift ? MovementState::None : unit->current_movement_state,
			    unit->getBodyAnimationFrame(), unit->getHandAnimationFrame(),
			    unit->getDistanceTravelled(), firingAngle, visible, unit->isCloaked());
			// Unit on fire
			if (unit->fireDebuffTicksRemaining > 0)
			{
				Vec2<float> transformedScreenPos = screenPosition;
				sp<Image> sprite;

				int age = unit->fireDebuffTicksRemaining;
				int maxLifetime = 5 * TICKS_PER_TURN;
				int frame = std::min(unit->burningDoodad->frames.size() - 1,
				                     unit->burningDoodad->frames.size() * age / maxLifetime);
				sprite = unit->burningDoodad->frames[frame].image;

				transformedScreenPos -= unit->burningDoodad->imageOffset;
				drawTinted(r, sprite, transformedScreenPos, visible);
			}
			break;
		}
		case TileViewMode::Strategy:
		{
			// Dead or non-visible units don't appear on strategy screen
			if (unit->isDead() || !visible)
				break;

			// 0 = friendly, 1 = enemy, 2 = neutral
			int side_offset = friendly ? 0 : (hostile ? 1 : 2);
			// Icon type, 0 = normal, 1 = prone, 2 = large
			int icon_type = unit->isLarge() ? ICON_LARGE
			                                : ((unit->current_body_state == BodyState::Prone ||
			                                    unit->target_body_state == BodyState::Prone)
			                                       ? ICON_PRONE
			                                       : ICON_STANDART);
			// Unit facing, in game starts with north (0,-1) and goes clockwise, from 0 to 7
			int facing_offset = offset_dir_map.at(unit->facing);
			// Current level offset, 0 = current 1 = above 2 = below
			int curent_level_offset = currentLevel < 0 ? 2 : (currentLevel > 0 ? 1 : 0);

			switch (icon_type)
			{
				case ICON_STANDART:
					drawTinted(r,
					           unit->strategyImages->at(side_offset * 120 +
					                                    curent_level_offset * 40 + facing_offset),
					           screenPosition - Vec2<float>{4, 4}, visible);
					break;
				case ICON_PRONE:
					// Vertical
					if (facing_offset == 0 || facing_offset == 4)
					{
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 0),
						           screenPosition - Vec2<float>{4.0f, 8.0f}, visible);
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 1),
						           screenPosition - Vec2<float>{4.0f, 0.0f}, visible);
					}
					// Horizontal
					else if (facing_offset == 2 || facing_offset == 6)
					{
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 0),
						           screenPosition - Vec2<float>{8.0f, 4.0f}, visible);
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 1),
						           screenPosition - Vec2<float>{0.0f, 4.0f}, visible);
					}
					// Diagonal
					else
					{
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 0),
						           screenPosition - Vec2<float>{8.0f, 8.0f}, visible);
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 1),
						           screenPosition - Vec2<float>{0.0f, 8.0f}, visible);
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 2),
						           screenPosition - Vec2<float>{8.0f, 0.0f}, visible);
						drawTinted(r,
						           unit->strategyImages->at(side_offset * 120 +
						                                    curent_level_offset * 40 +
						                                    offset_prone_map.at(facing_offset) + 3),
						           screenPosition - Vec2<float>{0.0f, 0.0f}, visible);
					}
					break;
				case ICON_LARGE:
					drawTinted(r,
					           unit->strategyImages->at(
					               side_offset * 120 + curent_level_offset * 40 + offset_large + 0),
					           screenPosition - Vec2<float>{8.0f, 8.0f}, visible);
					drawTinted(r,
					           unit->strategyImages->at(
					               side_offset * 120 + curent_level_offset * 40 + offset_large + 1),
					           screenPosition - Vec2<float>{0.0f, 8.0f}, visible);
					drawTinted(r,
					           unit->strategyImages->at(
					               side_offset * 120 + curent_level_offset * 40 + offset_large + 2),
					           screenPosition - Vec2<float>{8.0f, 0.0f}, visible);
					drawTinted(r,
					           unit->strategyImages->at(
					               side_offset * 120 + curent_level_offset * 40 + offset_large + 3),
					           screenPosition - Vec2<float>{0.0f, 0.0f}, visible);
					break;
			}
			break;
		}
		default:
			LogError("Unsupported view mode");
	}
}

void TileObjectBattleUnit::removeFromMap()
{
	bool requireRecalc = owningTile != nullptr;
	std::set<Tile *> prevIntersectingTiles;
	for (auto &t : intersectingTiles)
	{
		prevIntersectingTiles.insert(t);
	}

	TileObject::removeFromMap();
	if (requireRecalc)
	{
		for (auto &t : intersectingTiles)
		{
			prevIntersectingTiles.erase(t);
		}
		for (auto &t : prevIntersectingTiles)
		{
			t->updateBattlescapeUnitPresent();
		}
	}
}

void TileObjectBattleUnit::setPosition(Vec3<float> newPosition)
{
	auto u = getUnit();

	// Set appropriate bounds for the unit
	auto size = std::max(u->agent->type->bodyType->size[u->current_body_state][u->facing],
	                     u->agent->type->bodyType->size[u->target_body_state][u->facing]);
	auto maxHeight = std::max(u->agent->type->bodyType->height[u->current_body_state],
	                          u->agent->type->bodyType->height[u->target_body_state]);
	setBounds({size.x, size.y, (float)maxHeight / 40.0f});

	if (u->isLarge())
	{
		centerOffset = {0.0f, 0.0f, 1.0f};
	}
	else // if small
	{
		if (u->current_body_state == BodyState::Prone || u->target_body_state == BodyState::Prone)
		{
			centerOffset = {-u->facing.x * bounds.x / 4.0f, -u->facing.y * bounds.y / 4.0f, 0.5f};
		}
		else
		{
			centerOffset = {0.0f, 0.0f, 0.5f};
		}
	}

	occupiedTiles.clear();

	TileObject::setPosition(newPosition);
	for (auto &t : intersectingTiles)
	{
		t->updateBattlescapeUnitPresent();
	}

	auto pos = owningTile->position;

	// Vanilla allowed units to "pop into" other units without any limit
	// That is, unit can stand on height 38 while another unit is hovering in the tile above,
	// and cause no problems whatsoever, even though they're almost fully inside each other
	// (their positions only differ by 1 pixel)
	// We could introduce an option to disallow this?
	// Right now, here goes vanilla behavior
	if (u->current_movement_state == MovementState::Brainsuck)
	{
		// Sucking headcrab occupies nothing
	}
	else if (u->isLarge())
	{
		// Large units occupy 2x2x2 box, where position is the point
		// with max x, max y and min z
		// Also the same but on destination
		occupiedTiles.insert(pos);
		occupiedTiles.insert({pos.x - 1, pos.y, pos.z});
		occupiedTiles.insert({pos.x, pos.y - 1, pos.z});
		occupiedTiles.insert({pos.x - 1, pos.y - 1, pos.z});
		occupiedTiles.insert({pos.x, pos.y, pos.z + 1});
		occupiedTiles.insert({pos.x - 1, pos.y, pos.z + 1});
		occupiedTiles.insert({pos.x, pos.y - 1, pos.z + 1});
		occupiedTiles.insert({pos.x - 1, pos.y - 1, pos.z + 1});
		if (!u->atGoal)
		{
			pos = u->goalPosition;
			occupiedTiles.insert({pos.x - 1, pos.y, pos.z});
			occupiedTiles.insert({pos.x, pos.y - 1, pos.z});
			occupiedTiles.insert({pos.x - 1, pos.y - 1, pos.z});
			occupiedTiles.insert({pos.x, pos.y, pos.z + 1});
			occupiedTiles.insert({pos.x - 1, pos.y, pos.z + 1});
			occupiedTiles.insert({pos.x, pos.y - 1, pos.z + 1});
			occupiedTiles.insert({pos.x - 1, pos.y - 1, pos.z + 1});
		}
	}
	else if (u->current_body_state == BodyState::Prone)
	{
		// Prone units additionally occupy the tile behind them
		occupiedTiles.insert(pos);
		occupiedTiles.insert({pos.x - u->facing.x, pos.y - u->facing.y, pos.z});
		if (!u->atGoal)
		{
			pos = u->goalPosition;
			occupiedTiles.insert(pos);
			occupiedTiles.insert({pos.x - u->facing.x, pos.y - u->facing.y, pos.z});
		}
	}
	else
	{
		// Small units occupy just their tile
		occupiedTiles.insert(pos);
		if (!u->atGoal)
		{
			pos = u->goalPosition;
			occupiedTiles.insert(pos);
		}
	}
}

void TileObjectBattleUnit::addToDrawnTiles(Tile *tile)
{
	auto u = getUnit();
	if (!u)
	{
		return;
	}
	Vec3<int> maxCoords = {-1, -1, -1};
	auto currentHeight = u->getCurrentHeight();
	while (currentHeight > 40)
	{
		currentHeight -= 40;
	}
	for (auto &intersectingTile : intersectingTiles)
	{
		int x = intersectingTile->position.x;
		int y = intersectingTile->position.y;
		int z = intersectingTile->position.z;

		// Units are drawn in the topmost tile their head pops into
		// Otherwise, they can only be drawn in it if it's their owner tile
		if (maxCoords.z * 1000 + maxCoords.x + maxCoords.y < z * 1000 + x + y &&
		    u->position.z + (float)currentHeight / 40.0f >= (float)z + 0.25f)
		{
			tile = intersectingTile;
			maxCoords = {x, y, z};
		}
	}

	TileObject::addToDrawnTiles(tile);
}

TileObjectBattleUnit::~TileObjectBattleUnit() = default;

TileObjectBattleUnit::TileObjectBattleUnit(TileMap &map, sp<BattleUnit> unit)
    : TileObject(map, Type::Unit, Vec3<float>{0, 0, 0}), unit(unit)
{
}

Vec3<float> TileObjectBattleUnit::getVoxelCentrePosition() const
{
	// We could do all the following, but since we always aim at voxelmap's center,
	// regardless of which bits are filled or not (only accounting for height)
	// it is enough to just offset center according to voxelmap's height
	// Leaving the code here though as it may be useful later
	/*
	auto u = this->getUnit();
	auto size = u->agent->type->bodyType->size.at(u->current_body_state).at(u->facing);

	Vec3<int> voxelCentre = {0, 0, 0};
	for (int x = 0; x < size.x; x++)
	{
	    for (int y = 0; y < size.y; y++)
	    {
	        for (int z = 0; z < size.z; z++)
	        {
	            voxelCentre += getVoxelMap({x, y, z})->centre;
	        }
	    }
	}
	voxelCentre /= size.x * size.y * size.z;

	auto objPos = this->getCenter();
	objPos -= this->getVoxelOffset();
	return Vec3<float>(objPos.x + (float)voxelCentre.x / map.voxelMapSize.x,
	                   objPos.y + (float)voxelCentre.y / map.voxelMapSize.y,
	                   objPos.z + (float)voxelCentre.z / map.voxelMapSize.z);
	*/

	// Simple version:
	auto objPos = this->getCenter();
	return Vec3<float>(objPos.x, objPos.y,
	                   objPos.z - getVoxelOffset().z +
	                       (float)getUnit()->getCurrentHeight() / 2.0f / 40.0f);
}

sp<VoxelMap> TileObjectBattleUnit::getVoxelMap(Vec3<int> mapIndex, bool) const
{
	auto u = this->getUnit();
	auto size = u->agent->type->bodyType->size.at(u->current_body_state).at(u->facing);
	if (mapIndex.x >= size.x || mapIndex.y >= size.y || mapIndex.z >= size.z || mapIndex.x < 0 ||
	    mapIndex.y < 0 || mapIndex.z < 0)
		return nullptr;

	return u->agent->type->bodyType->voxelMaps.at(u->current_body_state)
	    .at(u->facing)
	    .at(mapIndex.z * size.y * size.x + mapIndex.y * size.x + mapIndex.x);
}

sp<BattleUnit> TileObjectBattleUnit::getUnit() const { return this->unit.lock(); }

Vec3<float> TileObjectBattleUnit::getPosition() const
{
	auto u = getUnit();
	if (!u)
	{
		LogError("Called with no owning unit object");
		return {0, 0, 0};
	}
	return u->getPosition();
}

} // namespace OpenApoc
