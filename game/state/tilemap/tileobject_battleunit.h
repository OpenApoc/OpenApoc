#pragma once

#include "game/state/tilemap/tileobject.h"
#include "library/sp.h"
#include "library/vec.h"
#include <set>

namespace OpenApoc
{

class BattleUnit;
class Image;

class TileObjectBattleUnit : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          bool visible, int currentLevel, bool friendly, bool hostile) override;
	~TileObjectBattleUnit() override;

	sp<BattleUnit> getUnit() const;

	Vec3<float> getVoxelOffset() const override
	{
		return {bounds_div_2.x, bounds_div_2.y, centerOffset.z};
	}
	// For aiming at the object
	Vec3<float> getVoxelCentrePosition() const override;
	Vec3<float> getCenterOffset() const override { return centerOffset; }
	Vec3<float> centerOffset = {0.0f, 0.0f, 0.0f};

	// For purposes of pathfinding, in addition to intersecting tiles we store this
	// Intersecting tiles means unit can be selected when clicked on tile
	// However, if not "occupied", then the tile can be moved into
	std::set<Vec3<int>> occupiedTiles;

	void setPosition(Vec3<float> newPosition) override;
	void removeFromMap() override;
	void addToDrawnTiles(Tile *tile) override;

	bool hasVoxelMap(bool los [[maybe_unused]]) const override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex, bool los [[maybe_unused]]) const override;
	Vec3<float> getPosition() const override;

  private:
	friend class TileMap;
	std::weak_ptr<BattleUnit> unit;
	std::list<sp<Image>>::iterator animationFrame;

	TileObjectBattleUnit(TileMap &map, sp<BattleUnit> unit);
};

} // namespace OpenApoc
