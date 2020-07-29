#include "game/ui/tileview/battletileview.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/ui.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/palette.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"
#include "game/state/battle/battlehazard.h"
#include "game/state/battle/battleitem.h"
#include "game/state/battle/battlemappart.h"
#include "game/state/battle/battleunitmission.h"
#include "game/state/gamestate.h"
#include "game/state/rules/battle/battlecommonimagelist.h"
#include "game/state/rules/battle/battlecommonsamplelist.h"
#include "game/state/shared/organisation.h"
#include "game/state/tilemap/tileobject_battlehazard.h"
#include "game/state/tilemap/tileobject_battleitem.h"
#include "game/state/tilemap/tileobject_battlemappart.h"
#include "game/state/tilemap/tileobject_battleunit.h"
#include "game/state/tilemap/tileobject_shadow.h"
#include "library/strings_format.h"
#include <glm/glm.hpp>

namespace OpenApoc
{
void BattleTileView::updateHiddenBar()
{
	hiddenBarTicksAccumulated = 0;

	int width = 321;
	int height = 33;
	Colour bar = {142, 142, 142, 255};
	Colour black = {0, 0, 0, 0};

	int totalTU = 0;
	int remainingTU = 0;

	for (auto &u : battle.units)
	{
		if (u.second->owner != battle.currentActiveOrganisation || !u.second->isConscious() ||
		    u.second->getAIType() == AIType::None || !u.second->canMove())
		{
			continue;
		}
		int reserve = u.second->reserveShotCost +
		              u.second->getBodyStateChangeCost(BodyState::Standing, BodyState::Kneeling);

		totalTU += u.second->initialTU - reserve;
		remainingTU += std::max(0, u.second->agent->modified_stats.time_units - reserve);
	}
	int maxWidth = totalTU ? clamp(width - remainingTU * width / totalTU, 0, width) : 0;

	auto progressBar = mksp<RGBImage>(Vec2<int>{width, height});
	{
		RGBImageLock l(progressBar);
		// Content
		for (int x = 1; x < maxWidth; x++)
		{
			for (int y = 1; y < height - 1; y++)
			{
				l.set({x, y}, bar);
			}
		}
		// Borders
		for (int y = 1; y < height - 1; y++)
		{
			l.set({0, y}, bar);
			l.set({width - 1, y}, bar);
		}
		for (int x = 0; x < width; x++)
		{
			l.set({x, 0}, bar);
			l.set({x, height - 1}, bar);
		}
		// Remainder
		for (int x = maxWidth + 1; x < width - 1; x++)
		{
			for (int y = 1; y < height - 1; y++)
			{
				l.set({x, y}, black);
			}
		}
	}

	hiddenForm->findControlTyped<Graphic>("GRAPHIC_HIDDEN")->setImage(progressBar);
}

BattleTileView::BattleTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
                               TileViewMode initialMode, Vec3<float> screenCenterTile,
                               GameState &gameState)
    : TileView(map, isoTileSize, stratTileSize, initialMode),
      hiddenForm(ui().getForm("battle/hidden")), state(gameState),
      battle(*gameState.current_battle),
      palette(fw().data->loadPalette("xcom3/tacdata/tactical.pal"))
{
	pal = palette;

	for (int j = 0; j <= 15; j++)
	{
		colorCurrent = j;
		auto newPal = mksp<Palette>();

		for (int i = 0; i < 255 - 4; i++)
		{
			newPal->setColour(i, palette->getColour(i));
		}
		// Lift color, pulsates from (0r 3/8g 5/8b) to (0r 8/8g 4/8b)
		newPal->setColour(255 - 4, Colour(0, (colorCurrent * 16 * 5 + 255 * 3) / 8,
		                                  (colorCurrent * 16 * -1 + 255 * 5) / 8));
		// Yellow color, for owned indicators, pulsates from (3/8r 3/8g 0b) to (8/8r 8/8g 0b)
		newPal->setColour(255 - 3, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8,
		                                  (colorCurrent * 16 * 5 + 255 * 3) / 8, 0));
		// Red color, for enemy indicators, pulsates from (3/8r 0g 0b) to (8/8r 0g 0b)
		newPal->setColour(255 - 2, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0, 0));
		// Pink color, for neutral indicators, pulsates from (3/8r 0g 3/8b) to (8/8r 0g 8/8b)
		newPal->setColour(255 - 1, Colour((colorCurrent * 16 * 5 + 255 * 3) / 8, 0,
		                                  (colorCurrent * 16 * 5 + 255 * 3) / 8));
		// Blue color, for misc. indicators, pulsates from (0r 3/8g 3/8b) to (0r 8/8g 8/8b)
		newPal->setColour(255 - 0, Colour(0, (colorCurrent * 16 * 5 + 255 * 3) / 8,
		                                  (colorCurrent * 16 * 5 + 255 * 3) / 8));

		modPalette.push_back(newPal);
	}

	layerDrawingMode = LayerDrawingMode::UpToCurrentLevel;
	targetTacticalThisLevel =

	    fw().data->loadImage(format("PCKSTRAT:xcom3/tacdata/stratico.pck:xcom3/tacdata/"
	                                "stratico.tab:%d",
	                                482));
	targetTacticalOtherLevel =
	    fw().data->loadImage(format("PCKSTRAT:xcom3/tacdata/stratico.pck:xcom3/tacdata/"
	                                "stratico.tab:%d",
	                                483));
	selectedTileEmptyImageBack =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                182));
	selectedTileEmptyImageFront =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                183));
	selectedTileFilledImageBack =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                184));
	selectedTileFilledImageFront =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                185));
	selectedTileFireImageBack =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                186));
	selectedTileFireImageFront =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                187));
	selectedTileBackgroundImageBack =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                188));
	selectedTileBackgroundImageFront =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                189));
	selectedTileImageOffset = {23, 42};
	pal = fw().data->loadPalette("xcom3/tacdata/tactical.pal");

	selectionImageFriendlySmall = fw().data->loadImage("battle/map-selection-small.png");
	selectionImageFriendlyLarge = fw().data->loadImage("battle/map-selection-large.png");

	activeUnitSelectionArrow.push_back(fw().data->loadImage("battle/battle-active-squadless.png"));
	inactiveUnitSelectionArrow.push_back(
	    fw().data->loadImage("battle/battle-inactive-squadless.png"));
	for (int i = 0; i < 6; i++)
	{
		activeUnitSelectionArrow.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
		                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
		                                167 + i)));
		inactiveUnitSelectionArrow.push_back(
		    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
		                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
		                                173 + i)));
	}
	// Alexey Andronov (Istrebitel)
	// FIXME: For some reason, when drawing unit selection images, a wild pixel appears
	// in the bottom left (visible above soldier's shoulder) when moving
	// Using PNG instead solves the problem, but we should rather find a way to properly use
	// vanilla game resources?
	//	activeUnitSelectionArrow.front() = fw().data->loadImage("battle/167.png");

	behaviorUnitSelectionUnderlay[BattleUnit::BehaviorMode::Evasive] =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                190));
	behaviorUnitSelectionUnderlay[BattleUnit::BehaviorMode::Normal] =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                191));
	behaviorUnitSelectionUnderlay[BattleUnit::BehaviorMode::Aggressive] =
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                192));

	runningIcon = fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                          "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                          193));

	bleedingIcon = fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                           "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                           194));

	healingIcons.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   195)));
	healingIcons.push_back(fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                                   "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                                   196)));

	lowMoraleIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                197)));
	lowMoraleIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                198)));

	psiIcons[PsiStatus::Probe].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                199)));
	psiIcons[PsiStatus::Probe].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                200)));
	psiIcons[PsiStatus::Panic].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                201)));
	psiIcons[PsiStatus::Panic].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                202)));
	psiIcons[PsiStatus::Stun].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                203)));
	psiIcons[PsiStatus::Stun].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                204)));
	psiIcons[PsiStatus::Control].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                205)));
	psiIcons[PsiStatus::Control].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                206)));
	psiIcons[PsiStatus::NotEngaged].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                207)));
	psiIcons[PsiStatus::NotEngaged].push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                208)));

	targetLocationIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                68)));
	targetLocationIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                69)));
	targetLocationIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                70)));
	targetLocationIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                69)));
	targetLocationIcons.push_back(
	    fw().data->loadImage(format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
	                                "icons.tab:%d:xcom3/tacdata/tactical.pal",
	                                68)));
	targetLocationOffset = {23, 42};

	waypointIcons.push_back(fw().data->loadImage("battle/battle-waypoint-1.png"));
	waypointIcons.push_back(fw().data->loadImage("battle/battle-waypoint-2.png"));
	waypointIcons.push_back(fw().data->loadImage("battle/battle-waypoint-3.png"));
	waypointIcons.push_back(fw().data->loadImage("battle/battle-waypoint-2.png"));
	waypointIcons.push_back(fw().data->loadImage("battle/battle-waypoint-1.png"));

	waypointDarkIcons.push_back(fw().data->loadImage("battle/battle-waypoint-1-dark.png"));
	waypointDarkIcons.push_back(fw().data->loadImage("battle/battle-waypoint-2-dark.png"));
	waypointDarkIcons.push_back(fw().data->loadImage("battle/battle-waypoint-3-dark.png"));
	waypointDarkIcons.push_back(fw().data->loadImage("battle/battle-waypoint-2-dark.png"));
	waypointDarkIcons.push_back(fw().data->loadImage("battle/battle-waypoint-1-dark.png"));

	auto font = ui().getFont("smallset");

	for (int i = 0; i <= 255; i++)
	{
		tuIndicators.push_back(font->getString(format("%d", i)));
	}
	tuSeparator = font->getString("/");
	pathPreviewTooFar = font->getString(tr("Too Far"));
	pathPreviewUnreachable = font->getString(tr("Blocked"));
	attackCostOutOfRange = font->getString(tr("Out of range"));
	attackCostNoArc = font->getString(tr("No arc of throw"));

	for (int i = 0; i < 16; i++)
	{
		auto healthBar = mksp<RGBImage>(Vec2<int>{15, 2});
		{
			RGBImageLock l(healthBar);
			for (int y = 0; y < 2; y++)
			{
				for (int x = 0; x < (int)healthBar->size.x; x++)
				{
					if (x >= i)
						l.set({x, y}, {0, 0, 0, 255});
					else
						l.set({x, y}, {0, 0, 0, 0});
				}
			}
		}
		arrowHealthBars.push_back(healthBar);
	}

	// FIXME: Load from save last screen location?
	setScreenCenterTile(screenCenterTile);
};

BattleTileView::~BattleTileView() = default;

void BattleTileView::eventOccurred(Event *e)
{
	if (e->type() == EVENT_KEY_DOWN)
	{
		if (debugHotkeyMode)
		{
			switch (e->keyboard().KeyCode)
			{
				case SDLK_F6:
				{
					LogWarning("Writing voxel view to tileviewvoxels.png");
					auto imageOffset = -this->getScreenOffset();
					auto img = std::dynamic_pointer_cast<RGBImage>(this->map.dumpVoxelView(
					    {imageOffset, imageOffset + dpySize}, *this, battle.battleViewZLevel));
					fw().data->writeImage("tileviewvoxels.png", img);
					return;
				}
				case SDLK_F7:
				{
					LogWarning("Writing voxel view (fast) to tileviewvoxels.png");
					auto imageOffset = -this->getScreenOffset();
					auto img = std::dynamic_pointer_cast<RGBImage>(
					    this->map.dumpVoxelView({imageOffset, imageOffset + dpySize}, *this,
					                            battle.battleViewZLevel, true));
					fw().data->writeImage("tileviewvoxels.png", img);
					return;
				}
				case SDLK_F8:
				{
					LogWarning("Writing voxel view to tileviewvoxels.png");
					auto imageOffset = -this->getScreenOffset();
					auto img = std::dynamic_pointer_cast<RGBImage>(
					    this->map.dumpVoxelView({imageOffset, imageOffset + dpySize}, *this,
					                            battle.battleViewZLevel, false, true));
					fw().data->writeImage("tileviewvoxels.png", img);
					return;
				}
				case SDLK_F9:
				{
					LogWarning("Writing voxel view (fast) to tileviewvoxels.png");
					auto imageOffset = -this->getScreenOffset();
					auto img = std::dynamic_pointer_cast<RGBImage>(
					    this->map.dumpVoxelView({imageOffset, imageOffset + dpySize}, *this,
					                            battle.battleViewZLevel, true, true));
					fw().data->writeImage("tileviewvoxels.png", img);
					return;
				}
				default:
					break;
			}
		}
		else
		{
			switch (e->keyboard().KeyCode)
			{
				default:
					break;
			}
		}
	}
	TileView::eventOccurred(e);
}

void BattleTileView::render()
{
	Renderer &r = *fw().renderer;
	r.clear();
	r.setPalette(this->pal);

	if (hideDisplay)
	{
		hiddenBarTicksAccumulated++;
		if (hiddenBarTicksAccumulated > 10)
		{
			updateHiddenBar();
		}
		hiddenForm->render();
		return;
	}

	// Rotate Icons
	{
		healingIconTicksAccumulated++;
		healingIconTicksAccumulated %= 2 * HEALING_ICON_ANIMATION_DELAY;
		lowMoraleIconTicksAccumulated++;
		lowMoraleIconTicksAccumulated %= 2 * LOWMORALE_ICON_ANIMATION_DELAY;
		psiIconTicksAccumulated++;
		psiIconTicksAccumulated %= 2 * PSI_ICON_ANIMATION_DELAY;
		selectionFrameTicksAccumulated++;
		selectionFrameTicksAccumulated %= 2 * SELECTION_FRAME_ANIMATION_DELAY;
		iconAnimationTicksAccumulated++;
		iconAnimationTicksAccumulated %= targetLocationIcons.size() * TARGET_ICONS_ANIMATION_DELAY;
		focusAnimationTicksAccumulated++;
		focusAnimationTicksAccumulated %=
		    (2 * FOCUS_ICONS_ANIMATION_FRAMES - 2) * FOCUS_ICONS_ANIMATION_DELAY;
	}

	// screenOffset.x/screenOffset.y is the 'amount added to the tile coords' - so we want
	// the inverse to tell which tiles are at the screen bounds
	auto topLeft = offsetScreenToTileCoords(Vec2<int>{-isoTileSize.x, -isoTileSize.y}, 0);
	auto topRight = offsetScreenToTileCoords(Vec2<int>{dpySize.x, -isoTileSize.y}, 0);
	auto bottomLeft = offsetScreenToTileCoords(Vec2<int>{-isoTileSize.x, dpySize.y}, map.size.z);
	auto bottomRight = offsetScreenToTileCoords(Vec2<int>{dpySize.x, dpySize.y}, map.size.z);

	int minX = std::max(0, topLeft.x);
	int maxX = std::min(map.size.x, bottomRight.x);

	int minY = std::max(0, topRight.y);
	int maxY = std::min(map.size.y, bottomLeft.y);

	int zFrom = 0;
	int zTo = maxZDraw;

	switch (layerDrawingMode)
	{
		case LayerDrawingMode::UpToCurrentLevel:
			zFrom = 0;
			zTo = battle.battleViewZLevel;
			break;
		case LayerDrawingMode::AllLevels:
			zFrom = 0;
			zTo = maxZDraw;
			break;
		case LayerDrawingMode::OnlyCurrentLevel:
			zFrom = battle.battleViewZLevel - 1;
			zTo = battle.battleViewZLevel;
			break;
	}

	// This is also the place where we emit a burning sound, because we find out which fire is in
	// the viewport and is closest to the screen center
	bool fireEncountered = false;
	float closestFireDistance = FLT_MAX;
	Vec3<float> closestFirePosition;

	// FIXME: A different algorithm is required in order to properly display everything
	// --- Read the note at the bottom of this file ---
	// FIXME: Draw double selection bracket for big units?
	switch (this->viewMode)
	{
		case TileViewMode::Isometric:
		{
			// List of units that require drawing of an overhead icon (bool = is first)
			std::list<std::pair<sp<BattleUnit>, bool>> unitsToDrawSelectionArrows;

			// List of units that require drawing of focus arrows (bool = islarge)
			std::list<std::pair<sp<TileObject>, bool>> unitsToDrawFocusArrows;

			// List of target icons to draw
			// std::map<Vec3<int>, std::list<Vec3<float>>> targetIconLocations;
			std::set<Vec3<int>> targetIconLocations;

			// List of waypointLocations to draw
			// std::map<Vec3<int>, std::list<Vec3<float>>> waypointLocations;
			std::set<Vec3<int>> waypointLocations;
			bool drawWaypoints = config().getBool("OpenApoc.NewFeature.DisplayUnitPaths");
			bool darkenWaypoints = false;
			std::list<StateRef<BattleUnit>> allUnits;
			if (revealWholeMap)
			{
				for (auto &entry : battle.units)
				{
					if (entry.second->isDead() || entry.second->retreated)
					{
						continue;
					}
					auto &u = entry.second;
					for (auto &m : u->missions)
					{
						if (m->type == BattleUnitMission::Type::ReachGoal)
						{
							targetIconLocations.insert(m->targetLocation);
							break;
						}
						if (m->type == BattleUnitMission::Type::GotoLocation &&
						    !m->currentPlannedPath.empty())
						{
							targetIconLocations.insert(m->targetLocation);
							if (drawWaypoints)
							{
								for (auto &w : m->currentPlannedPath)
								{
									if (w != m->targetLocation)
									{
										waypointLocations.insert(w);
									}
								}
							}
							break;
						}
					}
				}
			}
			else
			{
				for (auto &u : battle.battleViewSelectedUnits)
				{
					for (auto &m : u->missions)
					{
						if (m->type == BattleUnitMission::Type::ReachGoal)
						{
							targetIconLocations.insert(m->targetLocation);
							break;
						}
						if (m->type == BattleUnitMission::Type::GotoLocation &&
						    !m->currentPlannedPath.empty())
						{
							targetIconLocations.insert(m->targetLocation);
							if (drawWaypoints)
							{
								for (auto &w : m->currentPlannedPath)
								{
									if (w != m->targetLocation)
									{
										waypointLocations.insert(w);
									}
								}
							}
							break;
						}
					}
				}
			}
			if (previewedPathCost != PreviewedPathCostSpecial::NONE && drawWaypoints &&
			    waypointLocations.empty())
			{
				darkenWaypoints = true;
				for (auto &w : pathPreview)
				{
					waypointLocations.insert(w);
				}
			}
			auto &waypointImageSource = darkenWaypoints ? waypointDarkIcons : waypointIcons;

			static const Vec2<float> offsetFaceIcon = {-7.0f, -1.0f};

			for (int z = zFrom; z < zTo; z++)
			{
				int currentLevel = z - battle.battleViewZLevel + 1;

				// Find out when to draw selection bracket parts (if ever)
				Tile *selTileOnCurLevel = nullptr;
				Vec3<int> selTilePosOnCurLevel;
				sp<Image> selectionImageBack;
				sp<Image> selectionImageFront;
				bool drawPathPreview = false;
				bool drawAttackCost = false;
				if (selectedTilePosition.z >= z && selectedTilePosition.x >= minX &&
				    selectedTilePosition.x < maxX && selectedTilePosition.y >= minY &&
				    selectedTilePosition.y < maxY)
				{
					selTilePosOnCurLevel = {selectedTilePosition.x, selectedTilePosition.y, z};
					selTileOnCurLevel = map.getTile(selTilePosOnCurLevel.x, selTilePosOnCurLevel.y,
					                                selTilePosOnCurLevel.z);

					// Find what kind of selection bracket to draw (yellow or green)
					// Yellow if this tile intersects with a unit
					if (selectedTilePosition.z == z)
					{
						drawPathPreview = previewedPathCost != PreviewedPathCostSpecial::NONE;
						drawAttackCost = calculatedAttackCost != CalculatedAttackCostSpecial::NONE;
						auto u = selTileOnCurLevel->getUnitIfPresent(true);
						auto unit = u ? u->getUnit() : nullptr;
						if (unit &&
						    (unit->owner == battle.currentPlayer ||
						     battle.visibleUnits[battle.currentPlayer].find({&state, unit->id}) !=
						         battle.visibleUnits[battle.currentPlayer].end()))
						{
							if (battle.currentPlayer->isRelatedTo(unit->owner) ==
							    Organisation::Relation::Hostile)
							{
								selectionImageBack = selectedTileFireImageBack;
								selectionImageFront = selectedTileFireImageFront;
							}
							else
							{
								selectionImageBack = selectedTileFilledImageBack;
								selectionImageFront = selectedTileFilledImageFront;
							}
						}
						else
						{
							selectionImageBack = selectedTileEmptyImageBack;
							selectionImageFront = selectedTileEmptyImageFront;
						}
					}
					else
					{
						selectionImageBack = selectedTileBackgroundImageBack;
						selectionImageFront = selectedTileBackgroundImageFront;
					}
				}

				// Actually draw stuff
				for (unsigned int layer = 0; layer < map.getLayerCount(); layer++)
				{
					for (int y = minY; y < maxY; y++)
					{
						for (int x = minX; x < maxX; x++)
						{
							auto tile = map.getTile(x, y, z);
							bool visible = battle.getVisible(battle.currentPlayer, x, y, z);
							auto object_count = tile->drawnObjects[layer].size();
							size_t obj_id = 0;
							do
							{
								if (tile == selTileOnCurLevel && layer == 0 &&
								    selTileOnCurLevel->drawBattlescapeSelectionBackAt == obj_id)
								{
									r.draw(selectionImageBack,
									       tileToOffsetScreenCoords(selTilePosOnCurLevel) -
									           selectedTileImageOffset);
								}
								if (tile->drawTargetLocationIconAt == obj_id)
								{
									if (targetIconLocations.find({x, y, z}) !=
									    targetIconLocations.end())
									{
										r.draw(targetLocationIcons[iconAnimationTicksAccumulated /
										                           TARGET_ICONS_ANIMATION_DELAY],
										       tileToOffsetScreenCoords(Vec3<float>{
										           x, y, tile->getRestingPosition().z}) -
										           targetLocationOffset);
									}
									if (waypointLocations.find({x, y, z}) !=
									    waypointLocations.end())
									{
										r.draw(waypointImageSource[iconAnimationTicksAccumulated /
										                           TARGET_ICONS_ANIMATION_DELAY],
										       tileToOffsetScreenCoords(Vec3<float>{
										           x, y, tile->getRestingPosition().z}) -
										           targetLocationOffset);
									}
								}
								if (obj_id >= object_count)
								{
									break;
								}
								auto &obj = tile->drawnObjects[layer][obj_id];
								bool friendly = false;
								bool hostile = false;
								bool unitLowMorale = false;
								bool unitPsiAttacker = false;
								PsiStatus unitPsiAttackedStatus = PsiStatus::NotEngaged;
								Vec2<float> unitFaceIconPos;
								bool objectVisible = visible;
								switch (obj->getType())
								{
									case TileObject::Type::Shadow:
									{
										auto s = std::static_pointer_cast<TileObjectShadow>(obj);
										auto u = s->ownerBattleUnit.lock();
										if (u)
										{
											objectVisible =
											    !u->isConscious() ||
											    u->owner == battle.currentPlayer ||
											    battle.visibleUnits.at(battle.currentPlayer)
											            .find({&state, u->id}) !=
											        battle.visibleUnits.at(battle.currentPlayer)
											            .end();
										}
										break;
									}
									case TileObject::Type::Unit:
									{
										auto u = std::static_pointer_cast<TileObjectBattleUnit>(obj)
										             ->getUnit();
										objectVisible =
										    !u->isConscious() || u->owner == battle.currentPlayer ||
										    battle.visibleUnits.at(battle.currentPlayer)
										            .find({&state, u->id}) !=
										        battle.visibleUnits.at(battle.currentPlayer).end();
										friendly = u->owner == battle.currentPlayer;
										hostile = battle.currentPlayer->isRelatedTo(u->owner) ==
										          Organisation::Relation::Hostile;
										if (objectVisible)
										{
											if (u->moraleState != MoraleState::Normal)
											{
												unitLowMorale = true;
											}
											else if (u->psiStatus != PsiStatus::NotEngaged)
											{
												unitPsiAttacker = true;
											}
											if (!u->psiAttackers.empty())
											{
												unitPsiAttackedStatus =
												    u->psiAttackers.begin()->second;
											}
											if (unitLowMorale || unitPsiAttacker ||
											    unitPsiAttackedStatus != PsiStatus::NotEngaged)
											{
												unitFaceIconPos =
												    tileToOffsetScreenCoords(
												        u->getPosition() +
												        Vec3<float>{0.0f, 0.0f,
												                    (u->getCurrentHeight() - 4.0f) *
												                        1.5f / 40.0f}) +
												    offsetFaceIcon;
											}
										}
										if (!battle.battleViewSelectedUnits.empty())
										{
											auto selectedPos =
											    std::find(battle.battleViewSelectedUnits.begin(),
											              battle.battleViewSelectedUnits.end(), u);

											if (selectedPos ==
											    battle.battleViewSelectedUnits.begin())
											{
												unitsToDrawSelectionArrows.push_back({u, true});
											}
											else if (selectedPos !=
											         battle.battleViewSelectedUnits.end())
											{
												unitsToDrawSelectionArrows.push_back({u, false});
											}
											// If visible and focused by selected - draw focus
											// arrows
											if (objectVisible)
											{
												bool focusedBySelectedUnits = false;
												for (auto &su : battle.battleViewSelectedUnits)
												{
													if (std::find(u->focusedByUnits.begin(),
													              u->focusedByUnits.end(),
													              su) != u->focusedByUnits.end())
													{
														focusedBySelectedUnits = true;
														break;
													}
												}
												if (focusedBySelectedUnits)
												{
													unitsToDrawFocusArrows.push_back(
													    {obj, u->isLarge()});
												}
											}
										}
										break;
									}
									case TileObject::Type::Hazard:
									{
										if (visible && ticksUntilFireSound == 0)
										{
											auto h =
											    std::static_pointer_cast<TileObjectBattleHazard>(
											        obj)
											        ->getHazard();
											if (h->hazardType->fire)
											{
												auto distance =
												    glm::length(centerPos - h->position);
												if (distance < closestFireDistance)
												{
													fireEncountered = true;
													closestFireDistance = distance;
													closestFirePosition = h->position;
												}
											}
										}
									}
									default:
										break;
								}
								Vec2<float> pos = tileToOffsetScreenCoords(obj->getCenter());
								obj->draw(r, *this, pos, this->viewMode,
								          revealWholeMap || objectVisible, currentLevel, friendly,
								          hostile);
								int faceShift = 0;
								if (unitPsiAttacker)
								{
									r.draw(
									    psiIcons[PsiStatus::NotEngaged][psiIconTicksAccumulated /
									                                    PSI_ICON_ANIMATION_DELAY],
									    unitFaceIconPos);
									faceShift = 1;
								}
								if (unitPsiAttackedStatus != PsiStatus::NotEngaged)
								{
									r.draw(
									    psiIcons[unitPsiAttackedStatus][psiIconTicksAccumulated /
									                                    PSI_ICON_ANIMATION_DELAY],
									    unitFaceIconPos + Vec2<float>{0, faceShift * 16.0f});
									faceShift = -1;
								}
								if (unitLowMorale)
								{
									r.draw(lowMoraleIcons[lowMoraleIconTicksAccumulated /
									                      LOWMORALE_ICON_ANIMATION_DELAY],
									       unitFaceIconPos + Vec2<float>{0, faceShift * 16.0f});
								}
								// Loop ends when "break" is reached above
								obj_id++;
							} while (true);
							// When done with all objects, draw the front selection image
							if (tile == selTileOnCurLevel && layer == 0)
							{
								static const Vec2<int> offset{2, -53};
								const Vec2<int> tileScreenCoords =
								    tileToOffsetScreenCoords(selTilePosOnCurLevel);

								r.draw(selectionImageFront,
								       tileScreenCoords - selectedTileImageOffset);
								// drawing attack cost takes precedence over path cost
								if (drawAttackCost)
								{
									sp<Image> img;
									switch (static_cast<CalculatedAttackCostSpecial>(
									    calculatedAttackCost))
									{
										case CalculatedAttackCostSpecial::NO_WEAPON:
											img = nullptr;
											break;
										case CalculatedAttackCostSpecial::NO_ARC:
											img = attackCostNoArc;
											break;
										case CalculatedAttackCostSpecial::OUT_OF_RANGE:
											img = attackCostOutOfRange;
											break;
										default:
										{
											img = nullptr;         // don't draw using img
											if (!lastSelectedUnit) // shouldn't happen
												LogError("Displaying attack cost without selected "
												         "unit?!");
											auto imgCost = tuIndicators[calculatedAttackCost];
											auto imgStock =
											    tuIndicators[lastSelectedUnit->agent->modified_stats
											                     .time_units];
											Vec2<int> offset2{(imgCost->size.x + imgStock->size.x +
											                   tuSeparator->size.x) /
											                      2,
											                  tuSeparator->size.y / 2};
											r.draw(imgCost, tileScreenCoords + offset - offset2);
											offset2.x -= imgCost->size.x;
											r.draw(tuSeparator,
											       tileScreenCoords + offset - offset2);
											offset2.x -= tuSeparator->size.x;
											r.draw(imgStock, tileScreenCoords + offset - offset2);
											break;
										}
									}
									if (img)
									{
										r.draw(img,
										       tileScreenCoords + offset -
										           Vec2<int>{img->size.x / 2, img->size.y / 2});
									}
								}
								else if (drawPathPreview)
								{
									sp<Image> img;
									switch (
									    static_cast<PreviewedPathCostSpecial>(previewedPathCost))
									{
										case PreviewedPathCostSpecial::UNREACHABLE:
											img = pathPreviewUnreachable;
											break;
										case PreviewedPathCostSpecial::TOO_FAR:
											img = pathPreviewTooFar;
											break;
										default:
											img = tuIndicators[previewedPathCost];
											break;
									}
									if (img)
									{
										r.draw(img,
										       tileScreenCoords + offset -
										           Vec2<int>{img->size.x / 2, img->size.y / 2});
									}
								}
							}
							if (tile->pathfindingDebugFlag)
								r.draw(waypointIcons[0],
								       tileToOffsetScreenCoords(Vec3<int>{x, y, z}) -
								           selectedTileImageOffset);
						}
					}
				}
			}

			// Draw next level, units whose "legs" are below "zTo", projectiles and items moving
			for (int z = zTo; z < maxZDraw && z < zTo + 1; z++)
			{
				int currentLevel = z - battle.battleViewZLevel + 1;

				unsigned int layer1 = map.getLayer(TileObject::Type::Unit);
				unsigned int layer2 = map.getLayer(TileObject::Type::Shadow);
				unsigned int minLayer = std::min(layer1, layer2);
				unsigned int maxLayer = std::max(layer1, layer2);

				for (unsigned int layer = minLayer; layer <= maxLayer; layer++)
				{
					for (int y = minY; y < maxY; y++)
					{
						for (int x = minX; x < maxX; x++)
						{
							auto tile = map.getTile(x, y, z);
							bool visible = battle.getVisible(battle.currentPlayer, x, y, z);
							auto object_count = tile->drawnObjects[layer].size();
							size_t obj_id = 0;
							do
							{
								if (obj_id >= object_count)
								{
									break;
								}
								auto &obj = tile->drawnObjects[layer][obj_id];
								bool objectVisible = visible;
								bool friendly = false;
								bool hostile = false;
								bool draw = false;
								bool unitLowMorale = false;
								bool unitPsiAttacker = false;
								PsiStatus unitPsiAttackedStatus = PsiStatus::NotEngaged;
								Vec2<float> unitFaceIconPos;
								switch (obj->getType())
								{
									case TileObject::Type::Unit:
									{
										auto u = std::static_pointer_cast<TileObjectBattleUnit>(obj)
										             ->getUnit();
										if (u->position.z < zTo)
										{
											objectVisible =
											    !u->isConscious() ||
											    u->owner == battle.currentPlayer ||
											    battle.visibleUnits.at(battle.currentPlayer)
											            .find({&state, u->id}) !=
											        battle.visibleUnits.at(battle.currentPlayer)
											            .end();
											friendly = u->owner == battle.currentPlayer;
											hostile = battle.currentPlayer->isRelatedTo(u->owner) ==
											          Organisation::Relation::Hostile;
											draw = true;
											if (objectVisible)
											{
												if (u->moraleState != MoraleState::Normal)
												{
													unitLowMorale = true;
												}
												else if (u->psiStatus != PsiStatus::NotEngaged)
												{
													unitPsiAttacker = true;
												}
												if (!u->psiAttackers.empty())
												{
													unitPsiAttackedStatus =
													    u->psiAttackers.begin()->second;
												}
												if (unitLowMorale || unitPsiAttacker ||
												    unitPsiAttackedStatus != PsiStatus::NotEngaged)
												{
													unitFaceIconPos =
													    tileToOffsetScreenCoords(
													        u->getPosition() +
													        Vec3<float>{
													            0.0f, 0.0f,
													            (u->getCurrentHeight() - 4.0f) *
													                1.5f / 40.0f}) +
													    offsetFaceIcon;
												}
											}
											if (!battle.battleViewSelectedUnits.empty())
											{
												auto selectedPos = std::find(
												    battle.battleViewSelectedUnits.begin(),
												    battle.battleViewSelectedUnits.end(), u);

												if (selectedPos ==
												    battle.battleViewSelectedUnits.begin())
												{
													unitsToDrawSelectionArrows.push_back({u, true});
												}
												else if (selectedPos !=
												         battle.battleViewSelectedUnits.end())
												{
													unitsToDrawSelectionArrows.push_back(
													    {u, false});
												}
												// If visible and focused - draw focus arrows
												if (objectVisible)
												{
													bool focusedBySelectedUnits = false;
													for (auto &su : battle.battleViewSelectedUnits)
													{
														if (std::find(u->focusedByUnits.begin(),
														              u->focusedByUnits.end(),
														              su) !=
														    u->focusedByUnits.end())
														{
															focusedBySelectedUnits = true;
															break;
														}
													}
													if (focusedBySelectedUnits)
													{
														unitsToDrawFocusArrows.push_back(
														    {obj, u->isLarge()});
													}
												}
											}
										}
										break;
									}
									case TileObject::Type::Item:
									{
										draw = std::static_pointer_cast<TileObjectBattleItem>(obj)
										           ->getItem()
										           ->falling;
										break;
									}
									case TileObject::Type::Projectile:
									{
										draw = true;
										break;
									}
									case TileObject::Type::Ground:
									case TileObject::Type::LeftWall:
									case TileObject::Type::RightWall:
									case TileObject::Type::Feature:
									{
										draw =
										    std::static_pointer_cast<TileObjectBattleMapPart>(obj)
										        ->getOwner()
										        ->falling;
										break;
									}
									default:
										break;
								}
								if (draw)
								{
									Vec2<float> pos = tileToOffsetScreenCoords(obj->getCenter());
									obj->draw(r, *this, pos, this->viewMode,
									          revealWholeMap || objectVisible, currentLevel,
									          friendly, hostile);
									int faceShift = 0;
									if (unitPsiAttacker)
									{
										r.draw(psiIcons[PsiStatus::NotEngaged]
										               [psiIconTicksAccumulated /
										                PSI_ICON_ANIMATION_DELAY],
										       unitFaceIconPos);
										faceShift = 1;
									}
									if (unitPsiAttackedStatus != PsiStatus::NotEngaged)
									{
										r.draw(psiIcons[unitPsiAttackedStatus]
										               [psiIconTicksAccumulated /
										                PSI_ICON_ANIMATION_DELAY],
										       unitFaceIconPos + Vec2<float>{0, faceShift * 16.0f});
										faceShift = -1;
									}
									if (unitLowMorale)
									{
										r.draw(lowMoraleIcons[lowMoraleIconTicksAccumulated /
										                      LOWMORALE_ICON_ANIMATION_DELAY],
										       unitFaceIconPos + Vec2<float>{0, faceShift * 16.0f});
									}
								}
								// Loop ends when "break" is reached above
								obj_id++;
							} while (true);
						}
					}
				}
			}

			// Draw selected unit arrows
			for (auto &obj : unitsToDrawSelectionArrows)
			{
				static const Vec2<float> offset = {-13.0f, -14.0f};
				static const Vec2<float> offsetRunning = {0.0f, 0.0f};
				static const Vec2<float> offsetBehavior = {0.0f, 0.0f};
				static const Vec2<float> offsetBleed = {0.0f, 0.0f};
				static const Vec2<float> offsetHealing = {6.0f, 14.0f};
				static const Vec2<float> offsetTU = {13.0f, -5.0f};
				static const Vec2<float> offsetHealth = {6.0f, 2.0f};

				// Health from 0 to 15, where 15 = 100%, 14 = less than 99.9% and 0 = 0%+
				int health = obj.first->agent->modified_stats.health * 15 /
				             obj.first->agent->current_stats.health;

				if (health < 0)
					continue;

				Vec2<float> pos =
				    tileToOffsetScreenCoords(
				        obj.first->getPosition() +
				        Vec3<float>{0.0f, 0.0f,
				                    (obj.first->getCurrentHeight() - 4.0f) * 1.5f / 40.0f}) +
				    offset;

				// Selection arrow
				r.draw(obj.second ? activeUnitSelectionArrow[obj.first->squadNumber + 1]
				                  : inactiveUnitSelectionArrow[obj.first->squadNumber + 1],
				       pos);
				r.draw(arrowHealthBars[health], pos + offsetHealth);
				// Behavior
				r.draw(behaviorUnitSelectionUnderlay[obj.first->behavior_mode],
				       pos + offsetBehavior);

				if (battle.mode == Battle::Mode::TurnBased)
				{
					auto &img = tuIndicators[obj.first->agent->modified_stats.time_units];
					r.draw(img, pos + offsetTU - Vec2<float>{img->size.x / 2, img->size.y / 2});
				}

				if (obj.first->movement_mode == MovementMode::Running)
				{
					r.draw(runningIcon, pos + offsetRunning);
				}
				if (obj.first->isFatallyWounded())
				{
					if (obj.first->isHealing)
					{
						r.draw(healingIcons[healingIconTicksAccumulated /
						                    HEALING_ICON_ANIMATION_DELAY],
						       pos + offsetHealing);
					}
					else
					{
						r.draw(bleedingIcon, pos + offsetBleed);
					}
				}
			}

			if (revealWholeMap)
			{
				static const Vec2<float> offset = {-13.0f, -19.0f};
				static const Vec2<float> offsetTU = {13.0f, -5.0f};

				for (auto &u : battle.units)
				{
					if (!u.second->isConscious())
					{
						continue;
					}
					Vec2<float> pos = tileToOffsetScreenCoords(
					    u.second->getPosition() +
					    Vec3<float>{0.0f, 0.0f,
					                (u.second->getCurrentHeight() - 4.0f) * 1.5f / 40.0f});

					if (battle.mode == Battle::Mode::TurnBased)
					{
						auto &img = tuIndicators[u.second->agent->modified_stats.time_units];
						r.draw(img, pos + offset + offsetTU -
						                Vec2<float>{img->size.x / 2, img->size.y / 2});
					}

					for (auto &t : u.second->visibleEnemies)
					{
						auto tarPos = tileToOffsetScreenCoords(t->getPosition());
						auto tarVec = tarPos - pos;
						tarVec *= 0.75f;
						r.drawLine(pos, pos + tarVec, this->strategyViewBoxColour,
						           this->strategyViewBoxThickness);
					}
				}
			}

			// Draw unit focus arrows
			if (!unitsToDrawFocusArrows.empty())
			{
				static const Vec2<float> offset1 = {-20.0f, -29.0f};
				static const Vec2<float> offset2 = {22.0f, -29.0f};
				static const Vec2<float> offset3 = {-20.0f, 27.0f};
				static const Vec2<float> offset4 = {22.0f, 27.0f};
				static const Vec2<float> offsetd14 = {-1.0f, -1.0f};
				static const Vec2<float> offsetd23 = {1.0f, -1.0f};

				float offset = (float)focusAnimationTicksAccumulated / FOCUS_ICONS_ANIMATION_DELAY;
				// Offset goes like this: 0 1 2 3 4 3 2 1  (example for 5 frames)
				// Therefore, if value is >=frames, we do 2*frames -2 -offset
				// For example, 2*5 - 2 - 5 = 3, that's how we get 3 that's after 4
				Vec2<float> imgOffset = {
				    (float)battle.common_image_list->focusArrows[0]->size.x / 2.0f,
				    (float)battle.common_image_list->focusArrows[0]->size.y / 2.0f};
				if (offset >= FOCUS_ICONS_ANIMATION_FRAMES)
					offset = 2 * FOCUS_ICONS_ANIMATION_FRAMES - 2 - offset;

				for (auto &obj : unitsToDrawFocusArrows)
				{
					float largeOffset = obj.second ? 2.0f : 1.0f;
					Vec2<float> pos = tileToOffsetScreenCoords(obj.first->getCenter());

					r.draw(battle.common_image_list->focusArrows[0],
					       pos - imgOffset + largeOffset * offset1 + offset * offsetd14);
					r.draw(battle.common_image_list->focusArrows[1],
					       pos - imgOffset + largeOffset * offset2 + offset * offsetd23);
					r.draw(battle.common_image_list->focusArrows[2],
					       pos - imgOffset + largeOffset * offset3 - offset * offsetd23);
					r.draw(battle.common_image_list->focusArrows[3],
					       pos - imgOffset + largeOffset * offset4 - offset * offsetd14);
				}
			}
		}
		break;
		case TileViewMode::Strategy:
		{
			// Params are: visible, level, friendly, hostile, selected (0 = not, 1 = small, 2 =
			// large)
			std::list<std::tuple<sp<TileObject>, bool, int, bool, bool, int>> unitsToDraw;
			// Lines to draw between unit and destination, as well as what kind of target to draw
			std::list<std::tuple<Vec3<int>, Vec3<int>, bool>> targetLocationsToDraw;
			// Params are: visible, level
			std::list<std::tuple<sp<TileObject>, bool, int>> itemsToDraw;

			// Gather units below current level
			for (int z = 0; z < zFrom; z++)
			{
				for (unsigned int layer = 0; layer < map.getLayerCount(); layer++)
				{
					for (int y = minY; y < maxY; y++)
					{
						for (int x = minX; x < maxX; x++)
						{
							auto tile = map.getTile(x, y, z);
							auto object_count = tile->drawnObjects[layer].size();

							for (size_t obj_id = 0; obj_id < object_count; obj_id++)
							{
								auto &obj = tile->drawnObjects[layer][obj_id];
								switch (obj->getType())
								{
									case TileObject::Type::Unit:
									{
										auto u = std::static_pointer_cast<TileObjectBattleUnit>(obj)
										             ->getUnit();
										bool objectVisible =
										    !u->isConscious() || u->owner == battle.currentPlayer ||
										    battle.visibleUnits.at(battle.currentPlayer)
										            .find({&state, u->id}) !=
										        battle.visibleUnits.at(battle.currentPlayer).end();
										bool friendly = u->owner == battle.currentPlayer;
										bool hostile =
										    battle.currentPlayer->isRelatedTo(u->owner) ==
										    Organisation::Relation::Hostile;
										bool selected = false;

										if (friendly)
										{
											if (std::find(battle.battleViewSelectedUnits.begin(),
											              battle.battleViewSelectedUnits.end(),
											              u) !=
											    battle.battleViewSelectedUnits.end())
											{
												selected = true;
											}
											for (auto &m : u->missions)
											{
												if ((m->type ==
												     BattleUnitMission::Type::ReachGoal) ||
												    (m->type ==
												         BattleUnitMission::Type::GotoLocation &&
												     !m->currentPlannedPath.empty()))
												{
													targetLocationsToDraw.emplace_back(
													    m->targetLocation, (Vec3<int>)u->position,
													    (obj->getOwningTile()->position.z -
													     (battle.battleViewZLevel - 1)) == 0);
													break;
												}
											}
										}
										unitsToDraw.emplace_back(
										    obj, revealWholeMap || objectVisible,
										    obj->getOwningTile()->position.z -
										        (battle.battleViewZLevel - 1),
										    friendly, hostile,
										    selected ? (u->isLarge() ? 2 : 1) : 0);

										break;
									}
									default:
										break;
								}
							}
						}
					}
				}
			}

			// Draw everything but units and items
			// Gather units and items on current level
			for (int z = zFrom; z < zTo; z++)
			{
				// currentZLevel is an upper exclusive boundary, that's why we need to sub 1 here
				int currentLevel = z - (battle.battleViewZLevel - 1);

				for (unsigned int layer = 0; layer < map.getLayerCount(); layer++)
				{
					for (int y = minY; y < maxY; y++)
					{
						for (int x = minX; x < maxX; x++)
						{
							auto tile = map.getTile(x, y, z);
							bool visible = battle.getVisible(battle.currentPlayer, x, y, z);
							auto object_count = tile->drawnObjects[layer].size();

							for (size_t obj_id = 0; obj_id < object_count; obj_id++)
							{
								auto &obj = tile->drawnObjects[layer][obj_id];
								bool objectVisible = visible;
								switch (obj->getType())
								{
									case TileObject::Type::Unit:
									{
										auto u = std::static_pointer_cast<TileObjectBattleUnit>(obj)
										             ->getUnit();
										objectVisible =
										    !u->isConscious() || u->owner == battle.currentPlayer ||
										    battle.visibleUnits.at(battle.currentPlayer)
										            .find({&state, u->id}) !=
										        battle.visibleUnits.at(battle.currentPlayer).end();
										bool friendly = u->owner == battle.currentPlayer;
										bool hostile =
										    battle.currentPlayer->isRelatedTo(u->owner) ==
										    Organisation::Relation::Hostile;
										bool selected = false;

										if (friendly)
										{
											if (std::find(battle.battleViewSelectedUnits.begin(),
											              battle.battleViewSelectedUnits.end(),
											              u) !=
											    battle.battleViewSelectedUnits.end())
											{
												selected = true;
											}
											for (auto &m : u->missions)
											{
												if ((m->type ==
												     BattleUnitMission::Type::ReachGoal) ||
												    (m->type ==
												         BattleUnitMission::Type::GotoLocation &&
												     !m->currentPlannedPath.empty()))
												{
													targetLocationsToDraw.emplace_back(
													    m->targetLocation, (Vec3<int>)u->position,
													    (obj->getOwningTile()->position.z -
													     (battle.battleViewZLevel - 1)) == 0);
													break;
												}
											}
										}
										unitsToDraw.emplace_back(
										    obj, revealWholeMap || objectVisible,
										    obj->getOwningTile()->position.z -
										        (battle.battleViewZLevel - 1),
										    friendly, hostile,
										    selected ? (u->isLarge() ? 2 : 1) : 0);

										continue;
									}
									case TileObject::Type::Item:
									{
										if (currentLevel == 0)
										{
											itemsToDraw.emplace_back(
											    obj, revealWholeMap || visible,
											    obj->getOwningTile()->position.z -
											        (battle.battleViewZLevel - 1));
										}
										continue;
									}
									case TileObject::Type::Hazard:
									{
										if (visible && ticksUntilFireSound == 0)
										{
											auto h =
											    std::static_pointer_cast<TileObjectBattleHazard>(
											        obj)
											        ->getHazard();
											if (h->hazardType->fire)
											{
												auto distance =
												    glm::length(centerPos - h->position);
												if (distance < closestFireDistance)
												{
													fireEncountered = true;
													closestFireDistance = distance;
													closestFirePosition = h->position;
												}
											}
										}
									}
									default:
										break;
								}
								Vec2<float> pos = tileToOffsetScreenCoords(obj->getCenter());
								obj->draw(r, *this, pos, this->viewMode,
								          revealWholeMap || objectVisible, currentLevel);
							}
						}
					}
				}
			}

			// Gather units above current level
			for (int z = zTo; z < maxZDraw; z++)
			{
				for (unsigned int layer = 0; layer < map.getLayerCount(); layer++)
				{
					for (int y = minY; y < maxY; y++)
					{
						for (int x = minX; x < maxX; x++)
						{
							auto tile = map.getTile(x, y, z);
							auto object_count = tile->drawnObjects[layer].size();

							for (size_t obj_id = 0; obj_id < object_count; obj_id++)
							{
								auto &obj = tile->drawnObjects[layer][obj_id];
								switch (obj->getType())
								{
									case TileObject::Type::Unit:
									{
										auto u = std::static_pointer_cast<TileObjectBattleUnit>(obj)
										             ->getUnit();
										bool objectVisible =
										    !u->isConscious() || u->owner == battle.currentPlayer ||
										    battle.visibleUnits.at(battle.currentPlayer)
										            .find({&state, u->id}) !=
										        battle.visibleUnits.at(battle.currentPlayer).end();
										bool friendly = u->owner == battle.currentPlayer;
										bool hostile =
										    battle.currentPlayer->isRelatedTo(u->owner) ==
										    Organisation::Relation::Hostile;
										bool selected = false;

										if (friendly)
										{
											if (std::find(battle.battleViewSelectedUnits.begin(),
											              battle.battleViewSelectedUnits.end(),
											              u) !=
											    battle.battleViewSelectedUnits.end())
											{
												selected = true;
											}
											for (auto &m : u->missions)
											{
												if ((m->type ==
												     BattleUnitMission::Type::ReachGoal) ||
												    (m->type ==
												         BattleUnitMission::Type::GotoLocation &&
												     !m->currentPlannedPath.empty()))
												{
													targetLocationsToDraw.emplace_back(
													    m->targetLocation, (Vec3<int>)u->position,
													    (obj->getOwningTile()->position.z -
													     (battle.battleViewZLevel - 1)) == 0);
													break;
												}
											}
										}
										unitsToDraw.emplace_back(
										    obj, revealWholeMap || objectVisible,
										    obj->getOwningTile()->position.z -
										        (battle.battleViewZLevel - 1),
										    friendly, hostile,
										    selected ? (u->isLarge() ? 2 : 1) : 0);
										break;
									}
									default:
										break;
								}
							}
						}
					}
				}
			}

			// Draw stuff
			for (auto &obj : targetLocationsToDraw)
			{
				static const auto offsetLine = Vec2<int>{3, 3};
				static const auto offsetStrat = Vec2<int>{-1, -1};
				static const auto lineColor = Colour(255, 255, 0, 255);
				// Draw line from unit to target tile
				r.drawLine(tileToOffsetScreenCoords(std::get<0>(obj)) + offsetLine,
				           tileToOffsetScreenCoords(std::get<1>(obj)) + offsetLine, lineColor);
				// Draw location image at target tile
				r.draw(std::get<2>(obj) ? targetTacticalThisLevel : targetTacticalOtherLevel,
				       tileToOffsetScreenCoords(std::get<0>(obj)) + offsetStrat);
			}
			for (auto &obj : unitsToDraw)
			{
				auto unit = std::get<0>(obj);
				Vec2<float> pos = tileToOffsetScreenCoords(unit->getCenter());
				unit->draw(r, *this, pos, this->viewMode, std::get<1>(obj), std::get<2>(obj),
				           std::get<3>(obj), std::get<4>(obj));
				// Draw unit selection brackets
				auto selected = std::get<5>(obj);
				if (selected && (selectionFrameTicksAccumulated / SELECTION_FRAME_ANIMATION_DELAY))
				{
					auto drawn =
					    selected == 1 ? selectionImageFriendlySmall : selectionImageFriendlyLarge;
					r.draw(drawn, pos - Vec2<float>(drawn->size / (unsigned)2));
				}
			}
			for (auto &obj : itemsToDraw)
			{
				Vec2<float> pos = tileToOffsetScreenCoords(std::get<0>(obj)->getCenter());
				std::get<0>(obj)->draw(r, *this, pos, this->viewMode, std::get<1>(obj),
				                       std::get<2>(obj));
			}

			renderStrategyOverlay(r);
		}
		break;
		default:
			LogError("Unexpected tile view mode \"%d\"", (int)this->viewMode);
			break;
	}

	if (fireEncountered)
	{
		ticksUntilFireSound = 60 * state.battle_common_sample_list->burn->sampleCount /
		                      state.battle_common_sample_list->burn->format.frequency /
		                      state.battle_common_sample_list->burn->format.channels;
		fw().soundBackend->playSample(state.battle_common_sample_list->burn, closestFirePosition);
	}

	if (this->debugHotkeyMode)
	{
		auto font = ui().getFont("smallset");
		auto cursorPositionString = font->getString(format("Cursor at %s", selectedTilePosition));
		r.draw(cursorPositionString, {0, 0});
	}
}

void BattleTileView::update()
{
	TileView::update();

	// Pulsate palette colors
	colorCurrent += (colorForward ? 1 : -1);
	if (colorCurrent <= 0 || colorCurrent >= 15)
	{
		colorCurrent = clamp(colorCurrent, 0, 15);
		colorForward = !colorForward;
	}
	pal = modPalette[colorCurrent];
}

void BattleTileView::resetAttackCost()
{
	if (attackCostTicksAccumulated > 0)
	{
		attackCostTicksAccumulated = 0;
		calculatedAttackCost = static_cast<int>(CalculatedAttackCostSpecial::NONE);
	}
}

void BattleTileView::setZLevel(int zLevel)
{
	battle.battleViewZLevel = clamp(zLevel, 1, maxZDraw);
	setScreenCenterTile(Vec3<float>{centerPos.x, centerPos.y, battle.battleViewZLevel - 1});
}

int BattleTileView::getZLevel() { return battle.battleViewZLevel; }

void BattleTileView::setLayerDrawingMode(LayerDrawingMode mode) { layerDrawingMode = mode; }

void BattleTileView::setScreenCenterTile(Vec3<float> center)
{
	TileView::setScreenCenterTile(center);
	fw().soundBackend->setListenerPosition({center.x, center.y, center.z});
}

void BattleTileView::setScreenCenterTile(Vec2<float> center)
{
	this->setScreenCenterTile(Vec3<float>{center.x, center.y, battle.battleViewZLevel - 1});
}

void BattleTileView::setSelectedTilePosition(Vec3<int> newPosition)
{
	auto oldPosition = selectedTilePosition;
	TileView::setSelectedTilePosition(newPosition);
	if (oldPosition != selectedTilePosition)
	{
		resetPathPreview();
		resetAttackCost();
	}
}

void BattleTileView::resetPathPreview()
{
	if (pathPreviewTicksAccumulated > 0)
	{
		pathPreviewTicksAccumulated = 0;
		previewedPathCost = static_cast<int>(PreviewedPathCostSpecial::NONE);
		pathPreview.clear();
	}
}
} // namespace OpenApoc

// Alexey Andronov (Istrebitel)
// A different algorithm is required in order to properly display big units.
/*
1) Rendering must go in diagonal lines. Illustration (on XY plane):

CURRENT		TARGET

147			136
258			258
369			479

2) Objects must be located in the bottom-most, right-most tile they intersect
(already implemented)

3) Object can either occupy 1, 2 or 3 tiles on the X axis (only X matters)

- Tiny objects (items, projectiles) occupy 1 tile always
- Small typical objects (walls, sceneries, small units) occupy 1 tile when static,
2 when moving on X axis
- Large objects (large units) occupy 2 tiles when static, 3 when moving on x axis

How to determine this value is TBD.

4) When rendering we must check 1 tile ahead for 2-tile object
and 1 tile ahead and further on x axis for 3-tile object.

If present we must draw 1 tile ahead for 2-tile object
or 2 tiles ahead and one tile further on x-axis for 3 tile object
then resume normal draw order without drawing already drawn tiles

Illustration:

SMALL MOVING	LARGE STATIC	LARGE MOVING		LEGEND

xxxxx > xxxxx6.		x		= tile w/o  object drawn
xxxx > xxxx48	xxxx > xxxx48	x+++  > x+++59		+		= tile with object drawn
xxx  > xxx37	x++  > x++37	x++O  > x++28.		digit	= draw order
x+O  > x+16	x+O  > x+16		x+OO  > x+13.		o		= object yet to draw
x?   > x25		x?   > x25		x?	  > x47.		?		= current position

So, if we encounter a 2-tile (on x axis) object in the next position (x-1, y+1)
then we must first draw tile (x-1,y+1), and then draw our tile,
and then skip drawing next tile (as we have already drawn it!)

If we encounter a 3-tile (on x axis) object in the position (x-1,y+2)
then we must first draw (x-1,y+1), then (x-2,y+2), then (x-1,y+2), then draw our tile,
and then skip drawing next two tiles (as we have already drawn it) and skip drawing
the tile (x-1, y+2) on the next row

This is done best by having a set of Vec3<int>'s, and "skip next X tiles" variable.
When encountering a 2-tile object, we increment "skip next X tiles" by 1.
When encountering a 3-tile object, we increment "skip next X tiles" by 2,
and we add (x-1, y+2) to the set.
When trying to draw a tile we first check the "skip next X tiles" variable,
if > 0 we decrement and continue.
Second, we check if our tile is in the set. If so, we remove from set and continue.
Third, we draw normally
*/

// FIXME: A different drawing algorithm is required for battle's strategic view
/*
First, draw everything except units and items
Then, draw items only on current z-level
Then, draw agents, bottom to top, drawing hollow sprites for non-current levels
*/
