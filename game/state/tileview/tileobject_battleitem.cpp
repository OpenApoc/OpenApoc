#include "game/state/tileview/tileobject_battleitem.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"
#include "library/line.h"

namespace OpenApoc
{

void TileObjectBattleItem::draw(Renderer &r, TileTransform &, Vec2<float> screenPosition,
                                TileViewMode mode)
{
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	std::ignore = mode;
	auto item = this->item.lock();
	if (!item)
	{
		LogError("Called with no owning item object");
		return;
	}
	Vec2<float> transformedScreenPos = screenPosition;
	sp<Image> sprite;
	switch (mode)
	{
		case TileViewMode::Isometric:
			sprite = item->item->type->dropped_sprite;
			transformedScreenPos -=
			    item->item->type->image_offset - Vec2<float>{0.0f, -20.0f}; // FIXME: Proper
			break;
		case TileViewMode::Strategy:
			sprite = item->item->type->strategy_sprite;
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

TileObjectBattleItem::~TileObjectBattleItem() = default;

TileObjectBattleItem::TileObjectBattleItem(TileMap &map, sp<BattleItem> item)
    : TileObject(map, Type::Item, Vec3<float>{0.5f, 0.5f, 0.5f}), item(item)
{
}

Vec3<float> TileObjectBattleItem::getPosition() const
{
	auto p = this->item.lock();
	if (!p)
	{
		LogError("Called with no owning item object");
		return {0, 0, 0};
	}
	return p->getPosition();
}

} // namespace OpenApoc
