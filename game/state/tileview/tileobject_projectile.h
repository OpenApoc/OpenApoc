#pragma once

#include "game/state/city/projectile.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectProjectile : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode) override;
	virtual ~TileObjectProjectile();
	Vec3<float> getPosition() const override;

  private:
	friend class TileMap;
	std::weak_ptr<Projectile> projectile;
	TileObjectProjectile(TileMap &map, sp<Projectile> projectile);
};

} // namespace OpenApoc
