#pragma once
#include "framework/framework.h"
#include "game/state/battle/battleunit.h"
#include "game/ui/tileview/tileview.h"

namespace OpenApoc
{

class TileObjectBattleUnit;
class Battle;

class BattleTileView : public TileView
{
	// Formula: FPS / DESIRED_ANIMATIONS_PER_SECOND
	static const int TARGET_ICONS_ANIMATION_DELAY = 60 / 4;

	// Total amount of different focus icon states
	static const int FOCUS_ICONS_ANIMATION_FRAMES = 4;

	// Forrmula: FPS / FOCUS_ICONS_ANIMATION_FRAMES(both ways) / DESIRED_ANIMATIONS_PER_SECOND
	static const int FOCUS_ICONS_ANIMATION_DELAY = 60 / (2 * FOCUS_ICONS_ANIMATION_FRAMES - 2) / 2;

  public:
	enum class LayerDrawingMode
	{
		UpToCurrentLevel,
		AllLevels,
		OnlyCurrentLevel
	};

  private:
	LayerDrawingMode layerDrawingMode;
	Battle &battle;

	sp<Image> selectedTileEmptyImageBack;
	sp<Image> selectedTileEmptyImageFront;
	sp<Image> selectedTileFilledImageBack;
	sp<Image> selectedTileFilledImageFront;
	sp<Image> selectedTileFireImageBack;
	sp<Image> selectedTileFireImageFront;
	sp<Image> selectedTileBackgroundImageBack;
	sp<Image> selectedTileBackgroundImageFront;
	Vec2<int> selectedTileImageOffset;

	std::vector<sp<Image>> activeUnitSelectionArrow;
	std::vector<sp<Image>> inactiveUnitSelectionArrow;
	std::vector<sp<Image>> arrowHealthBars;
	std::map<BattleUnit::BehaviorMode, sp<Image>> behaviorUnitSelectionUnderlay;
	sp<Image> runningIcon;
	sp<Image> bleedingIcon;
	std::list<sp<Image>> healingIcons;
	sp<Image> healingIcon;
	std::vector<sp<Image>> targetLocationIcons;
	Vec2<float> targetLocationOffset;
	std::vector<sp<Image>> tuIndicators;
	// Must have same amount it items as in targetLocationIcons
	std::vector<sp<Image>> waypointIcons;
	std::vector<sp<Image>> waypointDarkIcons;
	int iconAnimationTicksAccumulated = 0;
	int focusAnimationTicksAccumulated = 0;

  public:
	BattleTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	               TileViewMode initialMode, Vec3<float> screenCenterTile, Battle &current_battle);
	~BattleTileView() override;

	// In turn-based, preview path cost when hovering over same tile for more than set amount of
	// time
	StateRef<BattleUnit> lastSelectedUnit;
	Vec3<int> lastSelectedUnitPosition;
	sp<Image> pathPreviewTooFar;
	sp<Image> pathPreviewUnreachable;
	std::list<Vec3<int>> pathPreview;
	int pathPreviewTicksAccumulated = 0;
	// -3 = unreachable
	// -2 = too far
	// -1 = no previewed path stored
	// 0+ = path cost
	int previewedPathCost = -1;
	void resetPathPreview();
	void updatePathPreview();

	void setZLevel(int zLevel);
	int getZLevel();

	void setScreenCenterTile(Vec2<float> center) override;
	void setScreenCenterTile(Vec3<float> center) override;

	void setLayerDrawingMode(LayerDrawingMode mode);

	void setSelectedTilePosition(Vec3<int> newPosition) override;

	void eventOccurred(Event *e) override;
	void render() override;
};
}
