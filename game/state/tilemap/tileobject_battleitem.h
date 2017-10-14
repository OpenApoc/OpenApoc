#pragma once

#include "game/state/tilemap/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class BattleItem;

class TileObjectBattleItem : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int, bool, bool) override;
	~TileObjectBattleItem() override;
	sp<BattleItem> getItem();
	Vec3<float> getPosition() const override;
	float getZOrder() const override;
	Vec3<float> getCenterOffset() const override { return {0.0f, 0.0f, bounds_div_2.z}; }

  private:
	friend class TileMap;
	wp<BattleItem> item;
	TileObjectBattleItem(TileMap &map, sp<BattleItem> item);
};

} // namespace OpenApoc
