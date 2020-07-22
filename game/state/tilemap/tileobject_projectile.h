#pragma once

#include "game/state/tilemap/tileobject.h"
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

	sp<Projectile> getProjectile() const;

	bool hasVoxelMap(bool los) const override;
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex, bool los) const override;
	Vec3<float> getVoxelOffset() const override { return {0.5f, 0.5f, 0.5f}; };
	Vec3<float> getVoxelCentrePosition() const override;

  private:
	friend class TileMap;
	wp<Projectile> projectile;
	TileObjectProjectile(TileMap &map, sp<Projectile> projectile);
};

} // namespace OpenApoc
