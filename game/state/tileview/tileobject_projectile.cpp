#include "game/state/tileview/tileobject_projectile.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"
#include "library/line.h"

namespace OpenApoc
{

void TileObjectProjectile::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
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

	Vec2<float> headScreenCoords = screenPosition;
	Vec3<float> tailPosition =
	    ((float)projectile->tail_length * (-glm::normalize(projectile->velocity)));
	tailPosition /= VELOCITY_SCALE;

	Vec2<float> tailScreenCoords = transform.tileToScreenCoords(tailPosition);
	tailScreenCoords += screenPosition;

	Vec3<int> projectile_line_start(lrint(headScreenCoords.x), lrint(headScreenCoords.y), 0);
	Vec3<int> projectile_line_end(lrint(tailScreenCoords.x), lrint(tailScreenCoords.y), 0);

	LineSegment<int, false> line{projectile_line_start, projectile_line_end};
	auto sprite_it = projectile->projectile_sprites.begin();

	for (auto &point : line)
	{
		if (sprite_it == projectile->projectile_sprites.end())
		{
			// We've reached the end of the sprite trail
			break;
		}
		if (*sprite_it == nullptr)
		{
			// A 'gap' in the sprite train
			continue;
		}
		r.draw(*sprite_it, Vec2<float>{point.x, point.y});
		sprite_it++;
	}
}

TileObjectProjectile::~TileObjectProjectile() = default;

TileObjectProjectile::TileObjectProjectile(TileMap &map, sp<Projectile> projectile)
    : TileObject(map, Type::Projectile, Vec3<float>{0, 0, 0}), projectile(projectile)
{
}

Vec3<float> TileObjectProjectile::getPosition() const
{
	auto p = this->projectile.lock();
	if (!p)
	{
		LogError("Called with no owning projectile object");
		return {0, 0, 0};
	}
	return p->getPosition();
}

} // namespace OpenApoc
