#include "game/state/tilemap/tileobject_battlemappart.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/tilemap/tilemap.h"

namespace OpenApoc
{

void TileObjectBattleMapPart::draw(Renderer &r, TileTransform &transform,
                                   Vec2<float> screenPosition, TileViewMode mode, bool visible, int,
                                   bool, bool)
{
	std::ignore = transform;
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
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
				auto &curType =
				    map_part->alternative_type ? map_part->alternative_type : map_part->type;
				sprite = curType->animation_frames[frame];
			}
			transformedScreenPos -= type->imageOffset;
			break;
		}
		case TileViewMode::Strategy:
			sprite = type->strategySprite;
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			if (sprite)
			{
				transformedScreenPos -= sprite->size / (unsigned)2;
			}
			break;
		default:
			LogError("Unsupported view mode");
	}
	if (sprite)
	{
		drawTinted(r, sprite, transformedScreenPos, visible);
	}
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

	owningTile->updateBattlescapeParameters();
	drawOnTile->updateBattlescapeUIDrawOrder();
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
TileObjectBattleMapPart::~TileObjectBattleMapPart() { map_part = nullptr; }

TileObjectBattleMapPart::TileObjectBattleMapPart(TileMap &map, sp<BattleMapPart> map_part)

    : TileObject(map, convertType(map_part->type->type), Vec3<float>{1.0f, 1.0f, 1.0f}),
      map_part(map_part)
{
}

sp<BattleMapPart> TileObjectBattleMapPart::getOwner() const { return map_part; }

sp<VoxelMap> TileObjectBattleMapPart::getVoxelMap(Vec3<int>, bool los) const
{
	if (los)
	{
		if (getOwner()->falling)
		{
			// Falling map parts do not break LOS
			return nullptr;
		}
		else
		{
			return getOwner()->type->voxelMapLOS;
		}
	}
	else
	{
		return getOwner()->type->voxelMapLOF;
	}
}

Vec3<float> TileObjectBattleMapPart::getPosition() const { return map_part->getPosition(); }

float TileObjectBattleMapPart::getZOrder() const
{
	auto z = getPosition().z;
	switch (type)
	{
		case Type::Ground:
			return z - 14.0f;
		case Type::LeftWall:
			return z - 14.0f;
		case Type::RightWall:
			return z - 14.0f;
		case Type::Feature:
		{
			return z + (float)map_part->type->height / 40.0f / 2.0f - 14.0f;
		}
		default:
			LogError("Impossible map part type %d", (int)type);
			return 0.0f;
	}
}

void TileObjectBattleMapPart::addToDrawnTiles(Tile *tile)
{
	Vec3<int> maxCoords = {-1, -1, -1};
	for (auto &intersectingTile : intersectingTiles)
	{
		int x = intersectingTile->position.x;
		int y = intersectingTile->position.y;
		int z = intersectingTile->position.z;

		// Map parts are drawn in the topmost tile their head pops into
		// Otherwise, they can only be drawn in it if it's their owner tile
		if (maxCoords.z * 1000 + maxCoords.x + maxCoords.y < z * 1000 + x + y &&
		    map_part->position.z + (float)map_part->type->height / 39.1f >= (float)z)
		{
			tile = intersectingTile;
			maxCoords = {x, y, z};
		}
	}
	TileObject::addToDrawnTiles(tile);
}
} // namespace OpenApoc
