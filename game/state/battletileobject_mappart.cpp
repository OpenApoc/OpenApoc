#include "game/state/battletileobject_mappart.h"
#include "framework/renderer.h"
#include "game/state/battletile.h"

namespace OpenApoc
{
void BattleTileObjectMapPart::draw(Renderer &r, TileTransform &transform,
                                   Vec2<float> screenPosition, TileViewMode mode)
{
	std::ignore = transform;
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	auto map_part = this->map_part.lock();
	if (!map_part)
	{
		LogError("Called with no owning scenery object");
		return;
	}
	// FIXME: If damaged use damaged tile sprites?
	auto &type = map_part->type;
	sp<Image> sprite;
	Vec2<float> transformedScreenPos = screenPosition;
	switch (mode)
	{
		case TileViewMode::Isometric:
			sprite = type->sprite;
			transformedScreenPos -= type->imageOffset;
			break;
		case TileViewMode::Strategy:
			sprite = type->strategySprite;
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		default:
			LogError("Unsupported view mode");
	}
	if (sprite)
		r.draw(sprite, transformedScreenPos);
}

const BattleTileObject::Type BattleTileObjectMapPart::convertType(BattleMapPartType::Type type)
{
	switch (type)
	{
		case BattleMapPartType::Type::Ground:
			return BattleTileObject::Type::Ground;
		case BattleMapPartType::Type::LeftWall:
			return BattleTileObject::Type::LeftWall;
		case BattleMapPartType::Type::RightWall:
			return BattleTileObject::Type::RightWall;
		case BattleMapPartType::Type::Scenery:
			return BattleTileObject::Type::Scenery;
		default:
			LogError("Unknown BattleMapPartType::Type %d", (int)type);
			return BattleTileObject::Type::Ground;
	}
}

BattleTileObjectMapPart::~BattleTileObjectMapPart() = default;

BattleTileObjectMapPart::BattleTileObjectMapPart(BattleTileMap &map, sp<BattleMapPart> map_part)
    : BattleTileObject(map, convertType(map_part->type->type), Vec3<float>{1, 1, 1}),
      map_part(map_part)
{
}

sp<BattleMapPart> BattleTileObjectMapPart::getOwner()
{
	auto s = this->map_part.lock();
	if (!s)
	{
		LogError("Owning map part object disappeared");
	}
	return s;
}

sp<VoxelMap> BattleTileObjectMapPart::getVoxelMap() { return this->getOwner()->type->voxelMap; }

Vec3<float> BattleTileObjectMapPart::getPosition() const
{
	auto s = this->map_part.lock();
	if (!s)
	{
		LogError("Called with no owning map part object");
		return {0, 0, 0};
	}
	return s->getPosition();
}
}
