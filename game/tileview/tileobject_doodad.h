#pragma once

#include "game/city/doodad.h"
#include "game/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectDoodad : public TileObject
{
  public:
	void draw(Renderer &r, TileView &view, Vec2<float> screenPosition, TileViewMode mode) override;
	virtual ~TileObjectDoodad();

	std::weak_ptr<Doodad> doodad;

	sp<VoxelMap> getVoxelMap() override { return nullptr; };

  private:
	friend class TileMap;
	TileObjectDoodad(TileMap &map, sp<Doodad> doodad);
};

} // namespace OpenApoc
