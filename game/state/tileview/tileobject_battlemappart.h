#pragma once
#include "game/state/battlemappart.h"
#include "game/state/battlemappart_type.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{
class TileObjectBattleMapPart : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode) override;
	~TileObjectBattleMapPart() override;

	std::weak_ptr<BattleMapPart> map_part;

	sp<BattleMapPart> getOwner();

	sp<VoxelMap> getVoxelMap() override;
	Vec3<float> getPosition() const override;

	static TileObject::Type convertType(BattleMapPartType::Type type);

  private:
	friend class TileMap;
	TileObjectBattleMapPart(TileMap &map, sp<BattleMapPart> map_part);
};
}
