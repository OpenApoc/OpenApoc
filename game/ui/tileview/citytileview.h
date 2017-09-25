#pragma once

#include "game/ui/tileview/tileview.h"
#include "library/sp.h"
#include "library/vec.h"

namespace OpenApoc
{

class Image;
class GameState;
class Vehicle;

class CityTileView : public TileView
{
  private:
	std::vector<std::vector<sp<Image>>> selectionBrackets;
	sp<Image> alertImage;
	sp<Image> targetTacticalThisLevel;
	sp<Image> selectionImageFriendlySmall;
	sp<Image> selectionImageFriendlyLarge;
	sp<Image> selectionImageHostileSmall;
	sp<Image> selectionImageHostileLarge;

	int selectionFrameTicksAccumulated = 0;

  public:
	CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	             TileViewMode initialMode, Vec3<float> screenCenterTile, GameState &gameState);
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
