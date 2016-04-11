#pragma once

#include "game/state/city/doodad.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectDoodad : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode) override;
	virtual ~TileObjectDoodad();

	std::weak_ptr<Doodad> doodad;

	sp<VoxelMap> getVoxelMap() override { return nullptr; }

	Vec3<float> getPosition() const override;

  private:
	friend class TileMap;
	TileObjectDoodad(TileMap &map, sp<Doodad> doodad);
};

} // namespace OpenApoc
