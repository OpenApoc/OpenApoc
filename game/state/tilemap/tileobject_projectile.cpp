#include "game/state/tilemap/tileobject_projectile.h"
#include "framework/renderer.h"
#include "game/state/shared/projectile.h"
#include "game/state/tilemap/tilemap.h"
#include "library/line.h"

namespace OpenApoc
{

void TileObjectProjectile::draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
                                TileViewMode mode, bool visible, int, bool, bool)
{
	// Mode isn't used as TileView::tileToScreenCoords already transforms according to the mode
	std::ignore = mode;
	auto projectile = this->projectile.lock();
	if (!projectile)
	{
		LogError("Called with no owning projectile object");
		return;
	}

	if (projectile->delay_ticks_remaining > 0)
	{
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
			drawTinted(r, *sprite_it, pos, visible);
		}
		sprite_it++;
	}
}

TileObjectProjectile::~TileObjectProjectile() = default;

TileObjectProjectile::TileObjectProjectile(TileMap &map, sp<Projectile> projectile)
    : TileObject(map, Type::Projectile, Vec3<float>{0.25f, 0.25f, 0.25f}), projectile(projectile)
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

void TileObjectProjectile::addToDrawnTiles(Tile *)
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

		// Projectiles are drawn in the tile that contains their point that is closest to camera
		if (maxCoords.z * 1000 + maxCoords.x + maxCoords.y < pos.z * 1000 + pos.x + pos.y)
		{
			maxCoords = {pos.x, pos.y, pos.z};
		}
	}
	TileObject::addToDrawnTiles(map.getTile(maxCoords));
}

sp<Projectile> TileObjectProjectile::getProjectile() const { return projectile.lock(); }

bool TileObjectProjectile::hasVoxelMap(bool los) const
{
	return los ? (projectile.lock()->voxelMapLos != nullptr)
	           : (projectile.lock()->voxelMapLof != nullptr);
}

sp<VoxelMap> TileObjectProjectile::getVoxelMap(Vec3<int> mapIndex, bool los) const
{
	if (mapIndex.x > 0 || mapIndex.y > 0 || mapIndex.z > 0)
	{
		return nullptr;
	}
	return los ? getProjectile()->voxelMapLos : getProjectile()->voxelMapLof;
}

Vec3<float> TileObjectProjectile::getVoxelCentrePosition() const { return getPosition(); }

} // namespace OpenApoc
