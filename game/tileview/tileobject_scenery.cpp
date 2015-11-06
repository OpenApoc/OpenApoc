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
	auto &tileDef = scenery->damaged ? *scenery->tileDef.getDamagedTile() : scenery->tileDef;
	sp<Image> sprite;
	sp<Image> overlaySprite;
	Vec2<float> transformedScreenPos = screenPosition;
	switch (mode)
	{
		case TileViewMode::Isometric:
			sprite = tileDef.getSprite();
			overlaySprite = tileDef.getOverlaySprite();
			transformedScreenPos -= tileDef.getImageOffset();
			break;
		case TileViewMode::Strategy:
			sprite = tileDef.getStrategySprite();
			// All strategy sprites so far are 8x8 so offset by 4 to draw from the center
			// FIXME: Not true for large sprites (2x2 UFOs?)
			transformedScreenPos -= Vec2<float>{4, 4};
			break;
		default:
			LogError("Unsupported view mode");
	}
	if (sprite)
		r.draw(sprite, transformedScreenPos);
	// FIXME: Should be drawn at 'later' Z than scenery (IE on top of any vehicles on tile?)
	if (overlaySprite)
		r.draw(overlaySprite, transformedScreenPos);
}

TileObjectScenery::~TileObjectScenery() {}

TileObjectScenery::TileObjectScenery(TileMap &map, sp<Scenery> scenery)
    : TileObject(map, TileObject::Type::Scenery, scenery->getPosition(), Vec3<float>{1, 1, 1}),
      scenery(scenery)
{
}

sp<Scenery> TileObjectScenery::getOwner()
{
	auto s = this->scenery.lock();
	if (!s)
	{
		LogError("Owning scenery object disappeared");
	}
	return s;
}

sp<VoxelMap> TileObjectScenery::getVoxelMap() { return this->getOwner()->tileDef.getVoxelMap(); }

} // namespace OpenApoc
