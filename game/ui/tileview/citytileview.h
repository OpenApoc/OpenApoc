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
	sp<Image> cargoImage;
	sp<Image> targetTacticalThisLevel;
	sp<Image> selectionImageFriendlySmall;
	sp<Image> selectionImageFriendlyLarge;
	sp<Image> selectionImageHostileSmall;
	sp<Image> selectionImageHostileLarge;

	int selectionFrameTicksAccumulated = 0;
	int portalImageTicksAccumulated = 0;

  public:
	CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	             TileViewMode initialMode, Vec3<float> screenCenterTile, GameState &gameState);
	~CityTileView() override;

	void eventOccurred(Event *e) override;
	void render() override;

	bool DEBUG_SHOW_VEHICLE_PATH = false;
	bool DEBUG_SHOW_ROAD_PATHFINDING = false;
	bool DEBUG_SHOW_ALIEN_CREW = false;
	bool DEBUG_SHOW_ROADS = false;
	bool DEBUG_SHOW_TUBE = false;
	int DEBUG_SHOW_MISC_TYPE = 0;
	bool DEBUG_SHOW_SLOPES = false;
	bool DEBUG_ONLY_TYPE = false;
	int DEBUG_DIRECTION = -1;

  private:
	GameState &state;
	sp<Image> selectedTileImageBack;
	sp<Image> selectedTileImageFront;
	Vec2<int> selectedTileImageOffset;
	Colour alienDetectionColour;
	float alienDetectionThickness;
};
}
