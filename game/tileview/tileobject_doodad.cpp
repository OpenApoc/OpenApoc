
#include "game/tileview/tileobject_doodad.h"
#include "game/rules/doodaddef.h"

namespace OpenApoc
{

void TileObjectDoodad::draw(Renderer &r, TileView &view, Vec2<float> screenPosition,
                            TileViewMode mode)
{
	std::ignore = view;
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	auto doodad = this->doodad.lock();
	if (!doodad)
	{
		LogError("Called with no owning doodad object");
		return;
	}
	sp<Image> sprite;
	Vec2<float> transformedScreenPos = screenPosition;
	switch (mode)
	{
		case TileViewMode::Isometric:
			sprite = doodad->getSprite();
			transformedScreenPos -= doodad->getImageOffset();
			break;
		case TileViewMode::Strategy:
			// no doodads on strategy view?
			return;
		default:
			LogError("Unsupported view mode");
	}
	r.draw(sprite, transformedScreenPos);
}

TileObjectDoodad::~TileObjectDoodad() {}

TileObjectDoodad::TileObjectDoodad(TileMap &map, sp<Doodad> doodad)
    : TileObject(map, TileObject::Type::Doodad, doodad->getPosition(), Vec3<float>{0, 0, 0}),
      doodad(doodad)
{
}

} // namespace OpenApoc
