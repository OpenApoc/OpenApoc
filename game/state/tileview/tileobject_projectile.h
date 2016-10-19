#pragma once

#include "game/state/tileview/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{
class Projectile;

class TileObjectProjectile : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int, bool, bool) override;
	~TileObjectProjectile() override;
	Vec3<float> getPosition() const override;
	void addToDrawnTiles(Tile *tile) override;

  private:
	friend class TileMap;
	wp<Projectile> projectile;
	TileObjectProjectile(TileMap &map, sp<Projectile> projectile);
};

} // namespace OpenApoc
