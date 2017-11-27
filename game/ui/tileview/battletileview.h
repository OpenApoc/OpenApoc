#pragma once

#include "game/state/battle/battleunit.h"
#include "game/ui/tileview/tileview.h"
#include "library/sp.h"
#include "library/vec.h"
#include <list>
#include <vector>

namespace OpenApoc
{

class TileObjectBattleUnit;
class Battle;
class Form;
class Image;

class BattleTileView : public TileView
{
  protected:
	// Formula: FPS / DESIRED_ANIMATIONS_PER_SECOND

	static const int TARGET_ICONS_ANIMATION_DELAY = 60 / 4;
	static const int HEALING_ICON_ANIMATION_DELAY = 60 / 4;
	static const int PSI_ICON_ANIMATION_DELAY = 60 / 4;
	static const int LOWMORALE_ICON_ANIMATION_DELAY = 60 / 2;

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

  protected:
	sp<Form> hiddenForm;

  private:
	LayerDrawingMode layerDrawingMode;
	GameState &state;
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
	std::vector<sp<Image>> healingIcons;
	std::vector<sp<Image>> lowMoraleIcons;
	std::map<PsiStatus, std::vector<sp<Image>>> psiIcons;
	std::vector<sp<Image>> targetLocationIcons;
	Vec2<float> targetLocationOffset;
	std::vector<sp<Image>> tuIndicators;
	// Must have same amount it items as in targetLocationIcons
	std::vector<sp<Image>> waypointIcons;
	std::vector<sp<Image>> waypointDarkIcons;
	sp<Image> targetTacticalThisLevel;
	sp<Image> targetTacticalOtherLevel;
	sp<Image> selectionImageFriendlySmall;
	sp<Image> selectionImageFriendlyLarge;
	int iconAnimationTicksAccumulated = 0;
	int healingIconTicksAccumulated = 0;
	int lowMoraleIconTicksAccumulated = 0;
	int psiIconTicksAccumulated = 0;
	int focusAnimationTicksAccumulated = 0;
	int selectionFrameTicksAccumulated = 0;

	bool colorForward = true;
	int colorCurrent = 0;
	sp<Palette> palette;
	std::vector<sp<Palette>> modPalette;

  public:
	BattleTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
	               TileViewMode initialMode, Vec3<float> screenCenterTile, GameState &gameState);
	~BattleTileView() override;

	// In turn-based, preview path cost when hovering over same tile for more than set amount of
	// time
	StateRef<BattleUnit> lastSelectedUnit;
	Vec3<int> lastSelectedUnitPosition;
	Vec2<int> lastSelectedUnitFacing;
	int ticksUntilFireSound = 0;

	int hiddenBarTicksAccumulated = 0;
	void updateHiddenBar();

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

	sp<Image> attackCostOutOfRange;
	sp<Image> attackCostNoArc;
	int attackCostTicksAccumulated = 0;
	// -4 = no weapon
	// -3 = no arc
	// -2 = out of range
	// -1 = no attack cost stored
	// 0+ = attack cost
	int calculatedAttackCost = -1;
	void resetAttackCost();
	// updateAttackCost is in battleView as it requires data about selection mode

	bool hideDisplay = false;

	bool revealWholeMap = false;

	void setZLevel(int zLevel);
	int getZLevel();

	void setScreenCenterTile(Vec2<float> center) override;
	void setScreenCenterTile(Vec3<float> center) override;
	void setScreenCenterTile(Vec2<int> center) override
	{
		this->setScreenCenterTile(Vec2<float>{center.x, center.y});
	}
	void setScreenCenterTile(Vec3<int> center) override
	{
		this->setScreenCenterTile(Vec3<float>{center.x, center.y, center.z});
	}

	void setLayerDrawingMode(LayerDrawingMode mode);

	void setSelectedTilePosition(Vec3<int> newPosition) override;

	void eventOccurred(Event *e) override;
	void render() override;
	void update() override;
};
}
