#include "game/state/tileview/tileobject_battlemappart.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"

namespace OpenApoc
{

void TileObjectBattleMapPart::draw(Renderer &r, TileTransform &transform,
                                   Vec2<float> screenPosition, TileViewMode mode, int)
{
	std::ignore = transform;
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	auto map_part = this->map_part.lock();
	if (!map_part)
	{
		LogError("Called with no owning scenery object");
		return;
	}
	auto &type = map_part->type;
	sp<Image> sprite;
	Vec2<float> transformedScreenPos = screenPosition;
	switch (mode)
	{
		case TileViewMode::Isometric:
		{
			int frame = map_part->getAnimationFrame();
			if (frame == -1)
			{
				sprite = type->sprite;
			}
			else
			{
				auto &curType = map_part->alternative_type ? map_part->alternative_type : map_part->type;
				sprite = curType->animation_frames[frame];
			}
			transformedScreenPos -= type->imageOffset;
			break;
		}
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

TileObject::Type TileObjectBattleMapPart::convertType(BattleMapPartType::Type type)
{
	switch (type)
	{
		case BattleMapPartType::Type::Ground:
			return TileObject::Type::Ground;
		case BattleMapPartType::Type::LeftWall:
			return TileObject::Type::LeftWall;
		case BattleMapPartType::Type::RightWall:
			return TileObject::Type::RightWall;
		case BattleMapPartType::Type::Feature:
			return TileObject::Type::Feature;
		default:
			LogError("Unknown BattleMapPartType::Type %d", (int)type);
			return TileObject::Type::Ground;
	}
}

void TileObjectBattleMapPart::setPosition(Vec3<float> newPosition)
{
	TileObject::setPosition(newPosition);
	if (type == Type::Ground || type == Type::LeftWall || type == Type::RightWall ||
	    type == Type::Feature)
	{
		owningTile->updateBattlescapeParameters();
		drawOnTile->updateBattlescapeUIDrawOrder();
	}
}

void TileObjectBattleMapPart::removeFromMap()
{
	bool requireRecalc = owningTile != nullptr;
	auto prevOwningTile = owningTile;
	auto prevDrawOnTile = drawOnTile;

	TileObject::removeFromMap();
	if (requireRecalc)
	{
		prevOwningTile->updateBattlescapeParameters();
		prevDrawOnTile->updateBattlescapeUIDrawOrder();
	}
}
TileObjectBattleMapPart::~TileObjectBattleMapPart() = default;

TileObjectBattleMapPart::TileObjectBattleMapPart(TileMap &map, sp<BattleMapPart> map_part)

    : TileObject(map, convertType(map_part->type->type), Vec3<float>{1.0f, 1.0f, 1.0f}),
      map_part(map_part)
{
}

sp<BattleMapPart> TileObjectBattleMapPart::getOwner()
{
	auto s = this->map_part.lock();
	if (!s)
	{
		LogError("Owning map part object disappeared");
	}
	return s;
}

sp<VoxelMap> TileObjectBattleMapPart::getVoxelMap(Vec3<int>)
{
	return this->getOwner()->type->voxelMapLOF;
}

Vec3<float> TileObjectBattleMapPart::getPosition() const
{
	auto s = this->map_part.lock();
	if (!s)
	{
		LogError("Called with no owning map part object");
		return {0, 0, 0};
	}
	return s->getPosition();
}

float TileObjectBattleMapPart::getZOrder() const
{
	auto z = getPosition().z;
	switch (type)
	{
		case Type::Ground:
			return z - 3.0f;
		case Type::LeftWall:
			return z - 2.0f;
		case Type::RightWall:
			return z - 1.0f;
		case Type::Feature:
		{
			auto mp = map_part.lock();
			if (!mp)
				return -1.0f;
			return z + (float)mp->type->height / 40.0f / 2.0f;
		}
		default:
			LogError("Impossible map part type %d", (int)type);
			return 0.0f;
	}
}
}
