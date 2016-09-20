#pragma once
#include "game/ui/tileview/tileview.h"


namespace OpenApoc
{
class TileObjectBattleUnit;

class BattleTileView : public TileView
{
  public:
	enum class LayerDrawingMode
	{
		UpToCurrentLevel,
		AllLevels,
		OnlyCurrentLevel
	};

  private:

	int currentZLevel;
	LayerDrawingMode layerDrawingMode;

	std::vector<sp<Image>> activeUnitSelectionArrow;
	std::vector<sp<Image>> inactiveUnitSelectionArrow;
	std::map<BattleUnit::BehaviorMode, sp<Image>> behaviorUnitSelectionUnderlay;
	sp<Image> runningIcon;
	sp<Image> bleedingIcon;
	std::list<sp<Image>> healingIcons;
	sp<Image> healingIcon;

  public:
	BattleTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize, TileViewMode initialMode);
	~BattleTileView() override;

	std::list<sp<BattleUnit>> selectedUnits;

	void setZLevel(int zLevel);
	int getZLevel();

	void setLayerDrawingMode(LayerDrawingMode mode);

	void eventOccurred(Event *e) override;
	void render() override;

	void drawUnitSelectionArrow(Renderer &r, sp<TileObjectBattleUnit> obj, bool first);
};
}