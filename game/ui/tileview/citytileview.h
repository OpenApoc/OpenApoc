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
	std::vector<sp<Image>> selectionBracketsFriendly;
	std::vector<sp<Image>> selectionBracketsHostile;
	sp<Image> alertImage;

  public:
	CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	             TileViewMode initialMode, GameState &gameState);
	~CityTileView() override;

	void eventOccurred(Event *e) override;
	void render() override;

	bool DEBUG_SHOW_ALIEN_CREW = false;

  protected:
	wp<Vehicle> selectedVehicle;

  private:
	GameState &state;
	sp<Image> selectedTileImageBack;
	sp<Image> selectedTileImageFront;
	Vec2<int> selectedTileImageOffset;
	Colour alienDetectionColour;
	float alienDetectionThickness;
};
}
