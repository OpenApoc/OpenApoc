#pragma once

#include "game/state/city/scenery.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectScenery : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode, bool) override;
	~TileObjectScenery() override;

	std::weak_ptr<Scenery> scenery;

	sp<Scenery> getOwner();

	sp<VoxelMap> getVoxelMap() override;
	Vec3<float> getPosition() const override;

  private:
	friend class TileMap;
	TileObjectScenery(TileMap &map, sp<Scenery> scenery);
};

} // namespace OpenApoc
