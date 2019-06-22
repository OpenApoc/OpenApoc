#include "game/ui/tileview/tileview.h"
#include "framework/configfile.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "game/state/battle/battle.h"

namespace OpenApoc
{

TileView::TileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
                   TileViewMode initialMode)
    : Stage(), map(map), isoTileSize(isoTileSize), stratTileSize(stratTileSize),
      viewMode(initialMode), dpySize(fw().displayGetWidth(), fw().displayGetHeight()),
      strategyViewBoxColour(212, 176, 172, 255), strategyViewBoxThickness(2.0f),
      selectedTilePosition(0, 0, 0), maxZDraw(map.size.z), centerPos(0, 0, 0),
      isoScrollSpeed(0.5, 0.5), stratScrollSpeed(2.0f, 2.0f)
{
	LogInfo("dpySize: %s", dpySize);
}

TileView::~TileView() = default;

void TileView::begin() { autoScroll = config().getBool("Options.Misc.AutoScroll"); }

void TileView::pause() {}

void TileView::resume() { autoScroll = config().getBool("Options.Misc.AutoScroll"); }

void TileView::finish() {}

void TileView::eventOccurred(Event *e)
{
	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_F1:
				debugHotkeyMode = !debugHotkeyMode;
				LogWarning("DEBUG MODE %s", debugHotkeyMode);
				break;
			case SDLK_UP:
				scrollUpKB = true;
				break;
			case SDLK_DOWN:
				scrollDownKB = true;
				break;
			case SDLK_LEFT:
				scrollLeftKB = true;
				break;
			case SDLK_RIGHT:
				scrollRightKB = true;
				break;
			case SDLK_s:
				if (selectedTilePosition.y < (map.size.y - 1))
					selectedTilePosition.y++;
				break;
			case SDLK_w:
				if (selectedTilePosition.y > 0)
					selectedTilePosition.y--;
				break;
			case SDLK_a:
				if (selectedTilePosition.x > 0)
					selectedTilePosition.x--;
				break;
			case SDLK_d:
				if (selectedTilePosition.x < (map.size.x - 1))
					selectedTilePosition.x++;
				break;
			case SDLK_r:
				if (selectedTilePosition.z < (map.size.z - 1))
					selectedTilePosition.z++;
				break;
			case SDLK_f:
				if (selectedTilePosition.z > 0)
					selectedTilePosition.z--;
				break;
		}
	}
	else if (e->type() == EVENT_KEY_UP)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_UP:
				scrollUpKB = false;
				break;
			case SDLK_DOWN:
				scrollDownKB = false;
				break;
			case SDLK_LEFT:
				scrollLeftKB = false;
				break;
			case SDLK_RIGHT:
				scrollRightKB = false;
				break;
		}
	}
	else if (e->type() == EVENT_FINGER_MOVE)
	{
		// FIXME: Review this code for sanity
		if (e->finger().IsPrimary)
		{
			Vec3<float> deltaPos(e->finger().DeltaX, e->finger().DeltaY, 0);
			if (this->viewMode == TileViewMode::Isometric)
			{
				deltaPos.x /= isoTileSize.x;
				deltaPos.y /= isoTileSize.y;
				Vec3<float> isoDelta(deltaPos.x + deltaPos.y, deltaPos.y - deltaPos.x, 0);
				deltaPos = isoDelta;
			}
			else
			{
				deltaPos.x /= stratTileSize.x;
				deltaPos.y /= stratTileSize.y;
			}
			Vec3<float> newPos = this->centerPos - deltaPos;
			this->setScreenCenterTile(newPos);
		}
	}
	else if (e->type() == EVENT_MOUSE_MOVE)
	{
		scrollLeftM = autoScroll && e->mouse().X < MOUSE_SCROLL_MARGIN;
		scrollRightM = autoScroll && e->mouse().X >= dpySize.x - MOUSE_SCROLL_MARGIN;
		scrollUpM = autoScroll && e->mouse().Y < MOUSE_SCROLL_MARGIN;
		scrollDownM = autoScroll && e->mouse().Y >= dpySize.y - MOUSE_SCROLL_MARGIN;
	}
}

bool TileView::isTransition() { return false; }

void TileView::setViewMode(TileViewMode newMode) { this->viewMode = newMode; }

TileViewMode TileView::getViewMode() const { return this->viewMode; }

Vec2<int> TileView::getScreenOffset() const
{
	Vec2<float> screenOffset = this->tileToScreenCoords(this->centerPos);

	return Vec2<int>{dpySize.x / 2 - screenOffset.x, dpySize.y / 2 - screenOffset.y};
}

void TileView::setScreenCenterTile(Vec3<float> center)
{
	fw().soundBackend->setListenerPosition({center.x, center.y, map.size.z / 2});
	Vec3<float> clampedCenter;
	if (center.x < 0.0f)
		clampedCenter.x = 0.0f;
	else if (center.x > map.size.x)
		clampedCenter.x = map.size.x;
	else
		clampedCenter.x = center.x;
	if (center.y < 0.0f)
		clampedCenter.y = 0.0f;
	else if (center.y > map.size.y)
		clampedCenter.y = map.size.y;
	else
		clampedCenter.y = center.y;
	if (center.z < 0.0f)
		clampedCenter.z = 0.0f;
	else if (center.z > map.size.z)
		clampedCenter.z = map.size.z;
	else
		clampedCenter.z = center.z;

	this->centerPos = clampedCenter;
}

void TileView::setScreenCenterTile(Vec2<float> center)
{
	this->setScreenCenterTile(Vec3<float>{center.x, center.y, 1});
}

Vec3<int> TileView::getSelectedTilePosition() { return selectedTilePosition; }

void TileView::setSelectedTilePosition(Vec3<int> newPosition)
{
	selectedTilePosition = newPosition;
	if (selectedTilePosition.x < 0)
		selectedTilePosition.x = 0;
	if (selectedTilePosition.y < 0)
		selectedTilePosition.y = 0;
	if (selectedTilePosition.z < 0)
		selectedTilePosition.z = 0;
	if (selectedTilePosition.x >= map.size.x)
		selectedTilePosition.x = map.size.x - 1;
	if (selectedTilePosition.y >= map.size.y)
		selectedTilePosition.y = map.size.y - 1;
	if (selectedTilePosition.z >= map.size.z)
		selectedTilePosition.z = map.size.z - 1;
}

void TileView::applyScrolling()
{
	Vec3<float> newPos = this->centerPos;
	if (this->viewMode == TileViewMode::Isometric)
	{
		if (scrollLeftKB || scrollLeftM)
		{
			newPos.x -= isoScrollSpeed.x;
			newPos.y += isoScrollSpeed.y;
		}
		if (scrollRightKB || scrollRightM)
		{
			newPos.x += isoScrollSpeed.x;
			newPos.y -= isoScrollSpeed.y;
		}
		if (scrollUpKB || scrollUpM)
		{
			newPos.y -= isoScrollSpeed.y;
			newPos.x -= isoScrollSpeed.x;
		}
		if (scrollDownKB || scrollDownM)
		{
			newPos.y += isoScrollSpeed.y;
			newPos.x += isoScrollSpeed.x;
		}
	}
	else if (this->viewMode == TileViewMode::Strategy)
	{
		if (scrollLeftKB || scrollLeftM)
			newPos.x -= stratScrollSpeed.x;
		if (scrollRightKB || scrollRightM)
			newPos.x += stratScrollSpeed.x;
		if (scrollUpKB || scrollUpM)
			newPos.y -= stratScrollSpeed.y;
		if (scrollDownKB || scrollDownM)
			newPos.y += stratScrollSpeed.y;
	}
	else
	{
		LogError("Unknown view mode");
	}

	this->setScreenCenterTile(newPos);
}

void TileView::renderStrategyOverlay(Renderer &r)
{
	if (this->viewMode == TileViewMode::Strategy)
	{
		Vec2<float> centerIsoScreenPos = this->tileToScreenCoords(
		    Vec3<float>{this->centerPos.x, this->centerPos.y, 0}, TileViewMode::Isometric);

		/* Draw the rectangle of where the isometric view would be */
		Vec2<float> topLeftIsoScreenPos = centerIsoScreenPos;
		topLeftIsoScreenPos.x -= dpySize.x / 2;
		topLeftIsoScreenPos.y -= dpySize.y / 2;

		Vec2<float> topRightIsoScreenPos = centerIsoScreenPos;
		topRightIsoScreenPos.x += dpySize.x / 2;
		topRightIsoScreenPos.y -= dpySize.y / 2;

		Vec2<float> bottomLeftIsoScreenPos = centerIsoScreenPos;
		bottomLeftIsoScreenPos.x -= dpySize.x / 2;
		bottomLeftIsoScreenPos.y += dpySize.y / 2;

		Vec2<float> bottomRightIsoScreenPos = centerIsoScreenPos;
		bottomRightIsoScreenPos.x += dpySize.x / 2;
		bottomRightIsoScreenPos.y += dpySize.y / 2;

		Vec3<float> topLeftIsoTilePos =
		    this->screenToTileCoords(topLeftIsoScreenPos, 0.0f, TileViewMode::Isometric);
		Vec3<float> topRightIsoTilePos =
		    this->screenToTileCoords(topRightIsoScreenPos, 0.0f, TileViewMode::Isometric);
		Vec3<float> bottomLeftIsoTilePos =
		    this->screenToTileCoords(bottomLeftIsoScreenPos, 0.0f, TileViewMode::Isometric);
		Vec3<float> bottomRightIsoTilePos =
		    this->screenToTileCoords(bottomRightIsoScreenPos, 0.0f, TileViewMode::Isometric);

		Vec2<float> topLeftRectPos = this->tileToOffsetScreenCoords(topLeftIsoTilePos);
		Vec2<float> topRightRectPos = this->tileToOffsetScreenCoords(topRightIsoTilePos);
		Vec2<float> bottomLeftRectPos = this->tileToOffsetScreenCoords(bottomLeftIsoTilePos);
		Vec2<float> bottomRightRectPos = this->tileToOffsetScreenCoords(bottomRightIsoTilePos);

		r.drawLine(topLeftRectPos, topRightRectPos, this->strategyViewBoxColour,
		           this->strategyViewBoxThickness);
		r.drawLine(topRightRectPos, bottomRightRectPos, this->strategyViewBoxColour,
		           this->strategyViewBoxThickness);

		r.drawLine(bottomRightRectPos, bottomLeftRectPos, this->strategyViewBoxColour,
		           this->strategyViewBoxThickness);
		r.drawLine(bottomLeftRectPos, topLeftRectPos, this->strategyViewBoxColour,
		           this->strategyViewBoxThickness);
	}
}

void TileView::update() { applyScrolling(); }
}; // namespace OpenApoc
