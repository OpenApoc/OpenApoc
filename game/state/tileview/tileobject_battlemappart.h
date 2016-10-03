#pragma once
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{
class TileObjectBattleMapPart : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          int) override;
	~TileObjectBattleMapPart() override;

	// For faster rendering, sp is better than wp
	sp<BattleMapPart> map_part;

	sp<BattleMapPart> getOwner() const;

	bool hasVoxelMap() override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex) const override;
	Vec3<float> getPosition() const override;
	float getZOrder() const override;
	Vec3<float> getCenterOffset() const override { return {0.0f, 0.0f, bounds_div_2.z}; }

	void setPosition(Vec3<float> newPosition) override;
	void removeFromMap() override;

	static TileObject::Type convertType(BattleMapPartType::Type type);

  private:
	friend class TileMap;
	TileObjectBattleMapPart(TileMap &map, sp<BattleMapPart> map_part);
};
}
