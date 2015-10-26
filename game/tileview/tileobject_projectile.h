#pragma once

#include "game/city/projectile.h"
#include "game/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectProjectile : public TileObject
{
  public:
	virtual void draw(Renderer &r, TileView &view, Vec2<float> screenPosition, TileViewMode mode);
	virtual ~TileObjectProjectile();

  private:
	friend class TileMap;
	std::weak_ptr<Projectile> projectile;
	TileObjectProjectile(TileMap &map, sp<Projectile> projectile);
};

} // namespace OpenApoc
