#pragma once

#include "game/state/tilemap/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class BattleHazard;

class TileObjectBattleHazard : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int, bool, bool) override;
	~TileObjectBattleHazard() override;
	sp<BattleHazard> getHazard();
	Vec3<float> getPosition() const override;
	float getZOrder() const override;

  private:
	friend class TileMap;
	wp<BattleHazard> hazard;
	TileObjectBattleHazard(TileMap &map, sp<BattleHazard> hazard);
};

} // namespace OpenApoc
