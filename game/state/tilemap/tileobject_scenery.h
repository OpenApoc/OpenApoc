#pragma once

#include "game/state/tilemap/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class Scenery;

class TileObjectScenery : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int, bool, bool) override;
	~TileObjectScenery() override;

	wp<Scenery> scenery;

	sp<Scenery> getOwner() const;

	bool hasVoxelMap(bool los [[maybe_unused]]) const override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex, bool) const override;
	Vec3<float> getPosition() const override;
	float getZOrder() const override;
	void setPosition(Vec3<float> newPosition) override;
	void removeFromMap() override;
	void addToDrawnTiles(Tile *tile) override;

  private:
	friend class TileMap;
	TileObjectScenery(TileMap &map, sp<Scenery> scenery);
};

} // namespace OpenApoc
