#include "game/state/tileview/tileobject_battleunit.h"
#include "framework/renderer.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "game/state/tileview/tile.h"
#include "library/voxel.h"

namespace OpenApoc
{

void TileObjectBattleUnit::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                                TileViewMode mode, int currentLevel)
{
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
			if (unit->current_hand_state == AgentType::HandState::Firing)
			{
				LogError("Implement handling firing angles!");
			}
			unit->agent->getAnimationPack()->drawUnit(
			    r, screenPosition, unit->agent->getImagePack(AgentType::BodyPart::Body),
			    unit->agent->getImagePack(AgentType::BodyPart::Legs),
			    unit->agent->getImagePack(AgentType::BodyPart::Helmet),
			    unit->agent->getImagePack(AgentType::BodyPart::LeftArm),
			    unit->agent->getImagePack(AgentType::BodyPart::RightArm),
			    unit->getDisplayedItem(), unit->facing, unit->current_body_state,
			    unit->target_body_state, unit->current_hand_state, unit->target_hand_state,
			    unit->usingLift ? AgentType::MovementState::None : unit->current_movement_state,
			    unit->getBodyAnimationFrame(), unit->getHandAnimationFrame(),
			    unit->getDistanceTravelled());
			break;
		}
		case TileViewMode::Strategy:
		{
			// Dead units don't appear on strategy screen
			if (unit->isDead())
				break;

			auto battle = unit->battle.lock();
			if (!battle)
				return;

			bool friendly = unit->owner == battle->currentPlayer;
			bool hostile = battle->currentPlayer->isRelatedTo(unit->owner) 
				== Organisation::Relation::Hostile;

			// 0 = enemy, 2 = friendly, 3 = neutral
			int side_offset = friendly ? 2 : (hostile ? 0 : 3);
			// Icon type, 0 = normal, 1 = prone, 2 = large
			int icon_type = unit->isLarge()
			                    ? ICON_LARGE
			                    : ((unit->current_body_state == AgentType::BodyState::Prone ||
			                        unit->target_body_state == AgentType::BodyState::Prone)
			                           ? ICON_PRONE
			                           : ICON_STANDART);
			// Unit facing, in game starts with north (0,-1) and goes clockwise, from 0 to 7
			int facing_offset = offset_dir_map.at(unit->facing);
			// Current level offset, 0 = current 1 = above 2 = below
			int curent_level_offset = currentLevel < 0 ? 2 : (currentLevel > 0 ? 1 : 0);

			auto common_image_list = battle->common_image_list;
			switch (icon_type)
			{
				case ICON_STANDART:
					r.draw(common_image_list->strategyImages[side_offset * 120 + curent_level_offset * 40 +
					                                  facing_offset],
					       screenPosition - Vec2<float>{4, 4});
					break;
				case ICON_PRONE:
					// Vertical
					if (facing_offset == 0 || facing_offset == 4)
					{
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 0],
						       screenPosition - Vec2<float>{4.0f, 8.0f});
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 1],
						       screenPosition - Vec2<float>{4.0f, 0.0f});
					}
					// Horizontal
					else if (facing_offset == 2 || facing_offset == 6)
					{
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 0],
						       screenPosition - Vec2<float>{8.0f, 4.0f});
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 1],
						       screenPosition - Vec2<float>{0.0f, 4.0f});
					}
					// Diagonal
					else
					{
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 0],
						       screenPosition - Vec2<float>{8.0f, 8.0f});
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 1],
						       screenPosition - Vec2<float>{0.0f, 8.0f});
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 2],
						       screenPosition - Vec2<float>{8.0f, 0.0f});
						r.draw(common_image_list
						           ->strategyImages[side_offset * 120 + curent_level_offset * 40 +
						                    offset_prone_map.at(facing_offset) + 3],
						       screenPosition - Vec2<float>{0.0f, 0.0f});
					}
					break;
				case ICON_LARGE:
					r.draw(common_image_list->strategyImages[side_offset * 120 + curent_level_offset * 40 +
					                                  offset_large + 0],
					       screenPosition - Vec2<float>{8.0f, 8.0f});
					r.draw(common_image_list->strategyImages[side_offset * 120 + curent_level_offset * 40 +
					                                  offset_large + 1],
					       screenPosition - Vec2<float>{0.0f, 8.0f});
					r.draw(common_image_list->strategyImages[side_offset * 120 + curent_level_offset * 40 +
					                                  offset_large + 2],
					       screenPosition - Vec2<float>{8.0f, 0.0f});
					r.draw(common_image_list->strategyImages[side_offset * 120 + curent_level_offset * 40 +
					                                  offset_large + 3],
					       screenPosition - Vec2<float>{0.0f, 0.0f});
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
	auto prevOwningTile = owningTile;

	TileObject::removeFromMap();
	if (requireRecalc)
	{
		prevOwningTile->updateBattlescapeUnitPresent();
	}
}

void TileObjectBattleUnit::setPosition(Vec3<float> newPosition)
{
	auto u = getUnit();

	// Set appropriate bounds for the unit
	auto size = std::max(u->agent->type->bodyType->size[u->current_body_state][u->facing],
	                     u->agent->type->bodyType->size[u->target_body_state][u->facing]);
	setBounds({size.x, size.y, size.z});
	// It would be appropriate to set bounds based on unit height, like this:
	//   setBounds({ size.x, size.y, (float)u->agent->type->bodyType->maxHeight / 40.0f });
	// However, this requires re-aligning unit sprites according to their height,
	// because vanilla has same offsets for all units regardless of their height
	// Therefore, it's much easier to just leave it this way
	if (u->isLarge())
	{
		centerOffset = {0.0f, 0.0f, bounds_div_2.z};
	}
	else // if small
	{
		if (u->current_body_state == AgentType::BodyState::Prone ||
		    u->target_body_state == AgentType::BodyState::Prone)
		{
			centerOffset = {-u->facing.x * bounds.x / 4.0f, -u->facing.y * bounds.y / 4.0f,
			                bounds_div_2.z};
		}
		else
		{
			centerOffset = {0.0f, 0.0f, bounds_div_2.z};
		}
	}

	occupiedTiles.clear();
	
	TileObject::setPosition(newPosition);
	owningTile->updateBattlescapeUnitPresent();

	auto pos = owningTile->position;

	// Vanilla allowed units to "pop into" other units without any limit
	// That is, unit can stand on height 38 while another unit is hovering in the tile above,
	// and cause no problems whatsoever, even though they're almost fully inside each other
	// (theis positions only differ by 1 pixel)
	// We could introduce an option to disallow this?
	// Right now, here goes vanilla behavior
	if (u->isLarge())
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
	else if (u->current_body_state == AgentType::BodyState::Prone)
	{
		// Prone units additionally occupy the tile behind them
		occupiedTiles.insert(pos);
		occupiedTiles.insert({pos.x - u->facing.x, pos.y = u->facing.y, pos.z});
		if (!u->atGoal)
		{
			pos = u->goalPosition;
			occupiedTiles.insert(pos);
			occupiedTiles.insert({pos.x - u->facing.x, pos.y = u->facing.y, pos.z});
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
	for (auto intersectingTile : intersectingTiles)
	{
		int x = intersectingTile->position.x;
		int y = intersectingTile->position.y;
		int z = intersectingTile->position.z;

		// Units are drawn in the topmost tile their head pops into
		// Otherwise, they can only be drawn in it if it's their owner tile
		if (maxCoords.z * 1000 + maxCoords.x + maxCoords.y < z * 1000 + x + y &&
			u->position.z + (float)u->getCurrentHeight() / 40.0f >= (float)z)
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

Vec3<float> TileObjectBattleUnit::getCentrePosition()
{
	auto u = this->getUnit();
	auto size = u->agent->type->bodyType->size[u->current_body_state][u->facing];

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
	return Vec3<float>(objPos.x + voxelCentre.x / map.voxelMapSize.x,
	                   objPos.y + voxelCentre.y / map.voxelMapSize.y,
	                   objPos.z + voxelCentre.z / map.voxelMapSize.z);
}

sp<VoxelMap> TileObjectBattleUnit::getVoxelMap(Vec3<int> mapIndex)
{
	auto u = this->getUnit();
	auto size = u->agent->type->bodyType->size[u->current_body_state][u->facing];
	if (mapIndex.x >= size.x || mapIndex.y >= size.y || mapIndex.z >= size.z)
		return nullptr;

	return u->agent->type->bodyType
	    ->voxelMaps[u->current_body_state][u->facing]
	               [mapIndex.z * size.y * size.x + mapIndex.y * size.x + mapIndex.x];
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
