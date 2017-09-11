#pragma once

#include "game/ui/tileview/tileview.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class Image;
class GameState;

class CityTileView : public TileView
{
  public:
	CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	             TileViewMode initialMode, GameState &gameState);
	~CityTileView() override;

	void eventOccurred(Event *e) override;
	void render() override;

	bool DEBUG_SHOW_ALIEN_CREW = false;

  private:
	GameState &state;
	sp<Image> selectedTileImageBack;
	sp<Image> selectedTileImageFront;
	Vec2<int> selectedTileImageOffset;
	Colour alienDetectionColour;
	float alienDetectionThickness;
};
}
