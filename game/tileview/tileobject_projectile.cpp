#include "game/tileview/tileobject_projectile.h"
#include "game/tileview/tile.h"

namespace OpenApoc
{

void TileObjectProjectile::draw(Renderer &r, TileView &view, Vec2<float> screenPosition,
                                TileViewMode mode)
{
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	std::ignore = mode;
	auto projectile = this->projectile.lock();
	if (!projectile)
	{
		LogError("Called with no owning projectile object");
		return;
	}
	switch (projectile->type)
	{
		case Projectile::Type::Beam:
		{
			Vec2<float> headScreenCoords = screenPosition;
			Vec3<float> tailPosition =
			    (projectile->beamLength * (glm::normalize(projectile->velocity)));
			tailPosition /= VELOCITY_SCALE;
			Vec2<float> tailScreenCoords = view.tileToScreenCoords(tailPosition);
			tailScreenCoords += screenPosition;
			r.drawLine(headScreenCoords, tailScreenCoords, projectile->colour,
			           projectile->beamWidth);
			break;
		}
		default:
			LogError("Unknown projectile type");
	}
}

TileObjectProjectile::~TileObjectProjectile() {}

TileObjectProjectile::TileObjectProjectile(TileMap &map, sp<Projectile> projectile)
    : TileObject(map, TileObject::Type::Projectile, projectile->getPosition(),
                 Vec3<float>{0, 0, 0}),
      projectile(projectile)
{
}

} // namespace OpenApoc
