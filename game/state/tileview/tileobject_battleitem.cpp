#include "game/state/tileview/tileobject_battleitem.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"
#include "library/line.h"

namespace OpenApoc
{

void TileObjectBattleItem::draw(Renderer &r, TileTransform &, Vec2<float> screenPosition,
                                TileViewMode mode, int)
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
			transformedScreenPos -= item->item->type->dropped_offset;
			break;
		case TileViewMode::Strategy:
		{
			if (!item->supported)
				break;
			auto battle = item->battle.lock();
			if (!battle)
				return;
			sprite = battle->common_image_list->strategyImages[480];
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		}
		default:
			LogError("Unsupported view mode");
	}
	if (sprite)
		r.draw(sprite, transformedScreenPos);
}

TileObjectBattleItem::~TileObjectBattleItem() = default;

TileObjectBattleItem::TileObjectBattleItem(TileMap &map, sp<BattleItem> item)
    // 1.0f bounds on z axis provides for same image offsets of all battlescape sprites
    : TileObject(map, Type::Item, Vec3<float>{0.0f, 0.0f, 1.0f}),
      item(item)
{
}

sp<BattleItem> TileObjectBattleItem::getItem()
{
	auto i = item.lock();
	if (!i)
	{
		LogError("Item disappeared");
		return nullptr;
	}
	return i;
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
