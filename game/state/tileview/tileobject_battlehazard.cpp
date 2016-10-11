#include "game/state/tileview/tileobject_battlehazard.h"
#include "framework/renderer.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/rules/doodad_type.h"
#include "game/state/tileview/tile.h"
#include "library/line.h"

namespace OpenApoc
{

void TileObjectBattleHazard::draw(Renderer &r, TileTransform &, Vec2<float> screenPosition,
                                TileViewMode mode, int, bool, bool)
{
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	std::ignore = mode;

	Vec2<float> transformedScreenPos = screenPosition;
	sp<Image> sprite;
	switch (mode)
	{
		case TileViewMode::Isometric:
			//sprite = item->item->type->dropped_sprite;
			//transformedScreenPos -= item->item->type->dropped_offset;
			break;
		case TileViewMode::Strategy:
		{
			//sprite = item->strategySprite;
			//transformedScreenPos -= Vec2<float>{4, 4};
			break;
		}
		default:
			LogError("Unsupported view mode");
	}
	if (sprite)
		r.draw(sprite, transformedScreenPos);
}

TileObjectBattleHazard::~TileObjectBattleHazard() = default;

TileObjectBattleHazard::TileObjectBattleHazard(TileMap &map, sp<BattleHazard> hazard)
    : TileObject(map, Type::Hazard, Vec3<float>{0.0f, 0.0f, 0.0f}),
      hazard(hazard)
{
}

sp<BattleHazard> TileObjectBattleHazard::getHazard()
{
	auto i = hazard.lock();
	if (!i)
	{
		LogError("Item disappeared");
		return nullptr;
	}
	return i;
}

Vec3<float> TileObjectBattleHazard::getPosition() const
{
	auto p = this->hazard.lock();
	if (!p)
	{
		LogError("Called with no owning hazard object");
		return {0, 0, 0};
	}
	return p->getPosition();
}

float TileObjectBattleHazard::getZOrder() const { return getPosition().z - 7.0f; }

} // namespace OpenApoc
