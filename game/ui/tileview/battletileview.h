#pragma once
#include "game/ui/tileview/tileview.h"


namespace OpenApoc
{
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

  public:
	BattleTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize, TileViewMode initialMode);
	~BattleTileView() override;

	void setZLevel(int zLevel);
	int getZLevel();

	void setLayerDrawingMode(LayerDrawingMode mode);

	void eventOccurred(Event *e) override;
	void render() override;
};
}