#include "game/state/tileview/tileobject_projectile.h"
#include "framework/renderer.h"
#include "game/state/tileview/tile.h"
#include "library/line.h"

namespace OpenApoc
{

void TileObjectProjectile::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                                TileViewMode mode, int)
{
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	std::ignore = mode;
	auto projectile = this->projectile.lock();
	if (!projectile)
	{
		LogError("Called with no owning projectile object");
		return;
	}

	auto sprite_it = projectile->projectile_sprites.begin();
	for (auto &point : projectile->spritePositions)
	{
		auto pos = screenPosition + transform.tileToScreenCoords(point - projectile->position);

		if (sprite_it == projectile->projectile_sprites.end())
		{
			// Some beams have tails longer than sprite list,
			// Some missiles too, but they will look buggy if we repeat them
			if (projectile->type == Projectile::Type::Missile)
				break;
			else
				sprite_it = projectile->projectile_sprites.begin();
		}
		if (*sprite_it != nullptr)
		{
			r.draw(*sprite_it, pos);
		}
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

void TileObjectProjectile::addToDrawnTiles(Tile *tile)
{
	auto p = this->projectile.lock();
	if (!p)
	{
		LogError("Called with no owning projectile object");
		return;
	}
	Vec3<float> maxCoords = {-1, -1, -1};
	for (auto &pos : p->spritePositions)
	{

		// Projectiles are drawn in the tile that containts their point that is closest to camera
		if (maxCoords.z * 1000 + maxCoords.x + maxCoords.y < pos.z * 1000 + pos.x + pos.y)
		{
			maxCoords = {pos.x, pos.y, pos.z};
		}
	}
	TileObject::addToDrawnTiles(map.getTile(maxCoords));
}

} // namespace OpenApoc
