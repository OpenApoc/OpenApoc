#pragma once

#include "game/state/tilemap/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class Doodad;

class TileObjectDoodad : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int, bool, bool) override;
	~TileObjectDoodad() override;

	wp<Doodad> doodad;
	sp<Doodad> getOwner() const;

	Vec3<float> getPosition() const override;
	float getZOrder() const override;

	bool hasVoxelMap(bool los) const override;
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex, bool los) const override;
	Vec3<float> getVoxelOffset() const override { return {0.5f, 0.5f, 0.5f}; };
	Vec3<float> getVoxelCentrePosition() const override;

  private:
	friend class TileMap;
	TileObjectDoodad(TileMap &map, sp<Doodad> doodad);
};

} // namespace OpenApoc
