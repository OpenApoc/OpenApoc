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

	sp<Palette> day_palette;
	sp<Palette> twilight_palette;
	sp<Palette> night_palette;

	int colorForward = 1;
	int colorCurrent = 0;

	std::vector<sp<Palette>> mod_day_palette;
	std::vector<sp<Palette>> mod_twilight_palette;
	std::vector<sp<Palette>> mod_night_palette;
	std::vector<sp<Palette>> mod_interpolated_palette;
	// Ingame minute when interpolated palette was calculated
	std::vector<int> interpolated_palette_minute;

  protected:
	static const int COUNTER_MAX = 50;
	int counter = 0;

  public:
	CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	             TileViewMode initialMode, Vec3<float> screenCenterTile, GameState &gameState);
	~CityTileView() override;

	void eventOccurred(Event *e) override;
	void render() override;
	void update() override;

	bool DEBUG_SHOW_VEHICLE_PATH = false;
	bool DEBUG_SHOW_ROAD_PATHFINDING = false;
	bool DEBUG_SHOW_ALIEN_CREW = false;
	bool DEBUG_SHOW_ROADS = false;
	bool DEBUG_SHOW_TUBE = false;
	int DEBUG_SHOW_MISC_TYPE = 0;
	bool DEBUG_SHOW_SLOPES = false;
	bool DEBUG_ONLY_TYPE = false;
	int DEBUG_DIRECTION = -1;
	int DEBUG_LAYER = -1;
	bool DEBUG_SHOW_ALIEN = false;

  private:
	GameState &state;
	sp<Image> selectedTileImageBack;
	sp<Image> selectedTileImageFront;
	Vec2<int> selectedTileImageOffset;
	Colour alienDetectionColour;
};
} // namespace OpenApoc
