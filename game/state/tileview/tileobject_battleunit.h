#pragma once

#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/battle/battleunit.h"
#include "game/state/tileview/tileobject.h"

namespace OpenApoc
{

class TileObjectBattleUnit : public TileObject
{
  public:
	void draw(Renderer &r, TileTransform &transform, Vec2<float> screenPosition, TileViewMode mode,
	          int currentLevel) override;
	~TileObjectBattleUnit() override;

	sp<BattleUnit> getUnit() const;

	// For aiming at the object
	Vec3<float> getCentrePosition();
	Vec3<float> getCenterOffset() const override { return centerOffset; }
	Vec3<float> centerOffset = {0.0f, 0.0f, 0.0f};

	// For purposes of pathfinding, in addition to intersecting tiles we store this
	// Intersecting tiles means unit can be selected when clicked on tile
	// However, if not "occupied", then the tile can be moved into
	std::set<Vec3<int>> occupiedTiles;

	void setPosition(Vec3<float> newPosition) override;
	void removeFromMap() override;
	void addToDrawnTiles(Tile *tile) override;

	bool hasVoxelMap() override { return true; }
	sp<VoxelMap> getVoxelMap(Vec3<int> mapIndex) override;
	Vec3<float> getPosition() const override;

  private:
	friend class TileMap;
	std::weak_ptr<BattleUnit> unit;
	std::list<sp<Image>>::iterator animationFrame;

	TileObjectBattleUnit(TileMap &map, sp<BattleUnit> unit);
};

} // namespace OpenApoc
