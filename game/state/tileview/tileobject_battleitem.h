#pragma once

#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlestrategyiconlist.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectBattleItem : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode, int) override;
	~TileObjectBattleItem() override;
	Vec3<float> getPosition() const override;

  private:
	friend class TileMap;
	std::weak_ptr<BattleItem> item;
	TileObjectBattleItem(TileMap &map, sp<BattleItem> item);
};

} // namespace OpenApoc
