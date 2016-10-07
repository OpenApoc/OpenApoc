#pragma once

#include "game/state/tileview/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class Scenery;

class TileObjectScenery : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          int, bool, bool) override;
	~TileObjectScenery() override;

	wp<Scenery> scenery;

	sp<Scenery> getOwner() const;

	bool hasVoxelMap() override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex, bool) const override;
	Vec3<float> getPosition() const override;
	float getZOrder() const override;

  private:
	friend class TileMap;
	TileObjectScenery(TileMap &map, sp<Scenery> scenery);
};

} // namespace OpenApoc
