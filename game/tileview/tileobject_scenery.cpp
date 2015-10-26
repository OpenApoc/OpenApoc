#include "game/tileview/tileobject_scenery.h"
#include "game/rules/scenerytiledef.h"

namespace OpenApoc
{

void TileObjectScenery::draw(Renderer &r, TileView &view, Vec2<float> screenPosition,
                             TileViewMode mode)
{
	std::ignore = view;
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	auto scenery = this->scenery.lock();
	if (!scenery)
	{
		LogError("Called with no owning scenery object");
		return;
	}
	sp<Image> sprite;
	Vec2<float> transformedScreenPos = screenPosition;
	switch (mode)
	{
		case TileViewMode::Isometric:
			sprite = scenery->tileDef.getSprite();
			transformedScreenPos -= scenery->tileDef.getImageOffset();
			break;
		case TileViewMode::Strategy:
			sprite = scenery->tileDef.getStrategySprite();
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		default:
			LogError("Unsupported view mode");
	}
	r.draw(sprite, transformedScreenPos);
}

TileObjectScenery::~TileObjectScenery() {}

TileObjectScenery::TileObjectScenery(TileMap &map, sp<Scenery> scenery)
    : TileObject(map, TileObject::Type::Scenery, scenery->getPosition(), Vec3<float>{1, 1, 1}),
      scenery(scenery)
{
}

} // namespace OpenApoc
