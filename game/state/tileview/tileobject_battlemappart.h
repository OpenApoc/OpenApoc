#pragma once
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battlemappart_type.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{
class TileObjectBattleMapPart : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition,
	          TileViewMode mode, int) override;
	~TileObjectBattleMapPart() override;

	wp<BattleMapPart> map_part;

	sp<BattleMapPart> getOwner();

	sp<VoxelMap> getVoxelMap() override;
	Vec3<float> getPosition() const override;
	Vec3<float> getCenterOffset() const override { return{ 0.0f, 0.0f, -bounds_div_2.z }; }
	// In battlescape, some objects's position mark their middle point
	// Some, however, are marked by their middle by x,y and by their bottom's z
	// This makes sense as truncated x, y, z are determining which tile the object is located inside
	// For scenery and units, this must be their leg point, not their center point.
	Vec3<float> getVoxelOffset() const override { return{ bounds_div_2.x, bounds_div_2.y, 0.0f }; }

	static TileObject::Type convertType(BattleMapPartType::Type type);

  private:
	friend class TileMap;
	TileObjectBattleMapPart(TileMap &map, sp<BattleMapPart> map_part);
};
}
