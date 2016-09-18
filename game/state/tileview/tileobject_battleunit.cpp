#include "game/state/tileview/tileobject_battleunit.h"
#include "game/state/battle/battleunitanimationpack.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"
#include "library/voxel.h"

namespace OpenApoc
{
	void TileObjectBattleUnit::setBounds(Vec3<float> bounds)
	{
		TileObject::setBounds(bounds);
		tileOffset = bounds_div_2 - 0.001f;
	}

	void TileObjectBattleUnit::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
		TileViewMode mode, int currentLevel)
	{
		static const std::map<Vec2<int>, int> offset_dir_map = {
			{ { 0,-1 },	0 },
			{ { 1,-1 },	1 },
			{ { 1,0 },	2 },
			{ { 1,1 },	3 },
			{ { 0,1 },	4 },
			{ { -1,1 },	5 },
			{ { -1,0 },	6 },
			{ { -1,-1 },	7 },
		};

		static const std::map<int, int> offset_prone_map = {
			{ 0, 0 },
			{ 1, 2 },
			{ 2, 6 },
			{ 3, 8 },
			{ 4, 12 },
			{ 5, 14 },
			{ 6, 18 },
			{ 7, 20 },
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
			unit->agent->getAnimationPack()->drawUnit(r, screenPosition,
				unit->agent->getImagePack(AgentType::BodyPart::Body),
				unit->agent->getImagePack(AgentType::BodyPart::Legs),
				unit->agent->getImagePack(AgentType::BodyPart::Helmet),
				unit->agent->getImagePack(AgentType::BodyPart::LeftArm),
				unit->agent->getImagePack(AgentType::BodyPart::RightArm),
				unit->agent->getItemInHands(), unit->facing,
				unit->current_body_state, unit->target_body_state,
				unit->current_hand_state, unit->target_hand_state,
				unit->movement_state, unit->getBodyAnimationFrame(), unit->getHandAnimationFrame(), unit->getDistanceTravelled());
			break;
		}
		case TileViewMode::Strategy:
		{
			// Dead units don't appear on strategy screen
			if (unit->isDead())
				break;

			// FIXME: Actually determine unit's side and relationship to player here to get unit's icon color
			// Side offset, 0 = ??, 1 = ??, 2 = ??, 3 = ?? , should be pink, red and yellow
			int side_offset = 2; // Just a random valid value for now
			// Icon type, 0 = normal, 1 = prone, 2 = large
			int icon_type = unit->agent->type->large ? ICON_LARGE : 
				(unit->current_body_state == AgentType::BodyState::Prone ? ICON_PRONE : ICON_STANDART);
			// Unit facing, in game starts with north (0,-1) and goes clockwise, from 0 to 7
			int facing_offset = offset_dir_map.at(unit->facing);
			// Current level offset, 0 = current 1 = above 2 = below	
			int curent_level_offset = currentLevel < 0 ? 2 : (currentLevel > 0 ? 1 : 0);

			switch (icon_type)
			{
				case ICON_STANDART:
					r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + facing_offset], screenPosition - Vec2<float>{4, 4});
					break;
				case ICON_PRONE:
					// Vertical 
					if (facing_offset == 0 || facing_offset == 4)
					{
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 0], screenPosition - Vec2<float>{4.0f, 8.0f});
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 1], screenPosition - Vec2<float>{4.0f, 0.0f});
					}
					// Horizontal
					else if (facing_offset == 2 || facing_offset == 6)
					{
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 0], screenPosition - Vec2<float>{8.0f, 4.0f});
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 1], screenPosition - Vec2<float>{0.0f, 4.0f});
					}
					// Diagonal
					else
					{
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 0], screenPosition - Vec2<float>{8.0f, 8.0f});
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 1], screenPosition - Vec2<float>{0.0f, 8.0f});
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 2], screenPosition - Vec2<float>{8.0f, 0.0f});
						r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + offset_prone_map.at(facing_offset) + 3], screenPosition - Vec2<float>{0.0f, 0.0f});
					}
					break;
				case ICON_LARGE:
					r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + 32], screenPosition - Vec2<float>{8.0f, 8.0f});
					r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + 33], screenPosition - Vec2<float>{0.0f, 8.0f});
					r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + 34], screenPosition - Vec2<float>{8.0f, 0.0f});
					r.draw(unit->strategy_icon_list->images[side_offset * 120 + curent_level_offset * 40 + 35], screenPosition - Vec2<float>{0.0f, 0.0f});
					break;
			}
			break;
		}
		default:
			LogError("Unsupported view mode");
		}
	}

	TileObjectBattleUnit::~TileObjectBattleUnit() = default;

	TileObjectBattleUnit::TileObjectBattleUnit(TileMap &map, sp<BattleUnit> unit)
		: TileObject(map, Type::Unit, Vec3<float>{0, 0, 0}), unit(unit)
	{
		if (unit->agent->type->large)
			setBounds({ 2.0f, 2.0f, 2.0f });
		else
			setBounds({ 1.0f, 1.0f, 1.0f });
	}

	Vec3<float> TileObjectBattleUnit::getCentrePosition()
	{
		this->getVoxelMap()->calculateCentre();
		auto voxelCentre = this->getVoxelMap()->centre;
		auto objPos = this->getPosition();
		objPos -= this->getVoxelOffset();
		return Vec3<float>(objPos.x + voxelCentre.x / map.voxelMapSize.x, objPos.y + voxelCentre.y / map.voxelMapSize.y,
			objPos.z + voxelCentre.z / map.voxelMapSize.z);
	}

	sp<VoxelMap> TileObjectBattleUnit::getVoxelMap() { auto u = this->getUnit(); return u->agent->type->voxelMaps[u->current_body_state]; }

	sp<BattleUnit> TileObjectBattleUnit::getUnit() const { return this->unit.lock(); }

	Vec3<float> TileObjectBattleUnit::getPosition() const
	{
		auto v = getUnit();
		if (!v)
		{
			LogError("Called with no owning unit object");
			return{ 0, 0, 0 };
		}
		return v->getPosition();
	}

	
} // namespace OpenApoc
