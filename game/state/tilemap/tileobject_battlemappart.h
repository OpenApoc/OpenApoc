#pragma once

#include "game/state/rules/battle/battlemapparttype.h"
#include "game/state/tilemap/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{
class BattleMapPart;

class TileObjectBattleMapPart : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int, bool, bool) override;
	~TileObjectBattleMapPart() override;

	// For faster rendering, sp is better than wp
	sp<BattleMapPart> map_part;

	sp<BattleMapPart> getOwner() const;

	bool hasVoxelMap(bool los [[maybe_unused]]) const override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex, bool los) const override;
	Vec3<float> getPosition() const override;
	float getZOrder() const override;
	Vec3<float> getCenterOffset() const override { return {0.0f, 0.0f, bounds_div_2.z}; }

	void setPosition(Vec3<float> newPosition) override;
	void removeFromMap() override;
	void addToDrawnTiles(Tile *tile) override;

	static TileObject::Type convertType(BattleMapPartType::Type type);

  private:
	friend class TileMap;
	TileObjectBattleMapPart(TileMap &map, sp<BattleMapPart> map_part);
};
} // namespace OpenApoc
