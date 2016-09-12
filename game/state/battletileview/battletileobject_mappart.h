#pragma once
#include "game/state/battlemappart.h"
#include "game/state/battlemappart_type.h"
#include "game/state/battletileview/battletileobject.h"

namespace OpenApoc
{
class BattleTileObjectMapPart : public BattleTileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode) override;
	~BattleTileObjectMapPart() override;

	std::weak_ptr<BattleMapPart> map_part;

	sp<BattleMapPart> getOwner();

	sp<VoxelMap> getVoxelMap() override;
	Vec3<float> getPosition() const override;

	static BattleTileObject::Type convertType(BattleMapPartType::Type type);

  private:
	friend class BattleTileMap;
	BattleTileObjectMapPart(BattleTileMap &map, sp<BattleMapPart> map_part);
};
}
