#pragma once
#include "game/ui/tileview/tileview.h"

namespace OpenApoc
{
class CityTileView : public TileView
{
  public:
	CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	             TileViewMode initialMode);
	~CityTileView() override;

	void eventOccurred(Event *e) override;
	void render() override;

  private:
	sp<Image> selectedTileImageBack;
	sp<Image> selectedTileImageFront;
	Vec2<int> selectedTileImageOffset;
};
}