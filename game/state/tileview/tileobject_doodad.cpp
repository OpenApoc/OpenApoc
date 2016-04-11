#include "game/state/tileview/tileobject_doodad.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"

namespace OpenApoc
{

void TileObjectDoodad::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                            TileViewMode mode)
{
	std::ignore = transform;
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
    : TileObject(map, Type::Doodad, Vec3<float>{0, 0, 0}), doodad(doodad)
{
}

Vec3<float> TileObjectDoodad::getPosition() const
{
	auto d = this->doodad.lock();
	if (!d)
	{
		LogError("Called with no owning doodad object");
		return {0, 0, 0};
	}
	return d->getPosition();
}

} // namespace OpenApoc
