#include "library/sp.h"
#include "game/tileview/tileview.h"
#include "game/tileview/tile.h"
#include "game/tileview/tileobject.h"

#include "framework/includes.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "game/resources/gamecore.h"
#include "game/city/vehicle.h"

namespace OpenApoc
{

TileView::TileView(Framework &fw, TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
                   TileViewMode initialMode)
    : Stage(fw), map(map), isoTileSize(isoTileSize), stratTileSize(stratTileSize),
      viewMode(initialMode), scrollUp(false), scrollDown(false), scrollLeft(false),
      scrollRight(false), dpySize(fw.Display_GetWidth(), fw.Display_GetHeight()),
      strategyViewBoxColour(128, 128, 128, 255), strategyViewBoxThickness(2.0f), maxZDraw(10),
      centerPos(0, 0), isoScrollSpeed(0.5, 0.5), stratScrollSpeed(2.0f, 2.0f),
      selectedTilePosition(0, 0, 0),
      selectedTileImageBack(fw.data->load_image("CITY/SELECTED-CITYTILE-BACK.PNG")),
      selectedTileImageFront(fw.data->load_image("CITY/SELECTED-CITYTILE-FRONT.PNG")),
      pal(fw.data->load_palette("xcom3/ufodata/PAL_01.DAT"))
{
}

TileView::~TileView() {}

void TileView::Begin() {}

void TileView::Pause() {}

void TileView::Resume() {}

void TileView::Finish() {}

void TileView::EventOccurred(Event *e)
{
	bool selectionChanged = false;

	if (e->Type == EVENT_KEY_DOWN)
	{
		switch (e->Data.Keyboard.KeyCode)
		{
			case SDLK_UP:
				scrollUp = true;
				break;
			case SDLK_DOWN:
				scrollDown = true;
				break;
			case SDLK_LEFT:
				scrollLeft = true;
				break;
			case SDLK_RIGHT:
				scrollRight = true;
				break;

			case SDLK_PAGEDOWN:
				if (fw.gamecore->DebugModeEnabled && maxZDraw > 1)
				{
					maxZDraw--;
				}
				break;
			case SDLK_PAGEUP:
				if (fw.gamecore->DebugModeEnabled && maxZDraw < map.size.z)
				{
					maxZDraw++;
				}
				break;
			case SDLK_s:
				selectionChanged = true;
				if (selectedTilePosition.y < (map.size.y - 1))
					selectedTilePosition.y++;
				break;
			case SDLK_w:
				selectionChanged = true;
				if (selectedTilePosition.y > 0)
					selectedTilePosition.y--;
				break;
			case SDLK_a:
				selectionChanged = true;
				if (selectedTilePosition.x > 0)
					selectedTilePosition.x--;
				break;
			case SDLK_d:
				selectionChanged = true;
				if (selectedTilePosition.x < (map.size.x - 1))
					selectedTilePosition.x++;
				break;
			case SDLK_r:
				selectionChanged = true;
				if (selectedTilePosition.z < (map.size.z - 1))
					selectedTilePosition.z++;
				break;
			case SDLK_f:
				selectionChanged = true;
				if (selectedTilePosition.z > 0)
					selectedTilePosition.z--;
				break;
			case SDLK_1:
				pal = fw.data->load_palette("xcom3/ufodata/PAL_01.DAT");
				break;
			case SDLK_2:
				pal = fw.data->load_palette("xcom3/ufodata/PAL_02.DAT");
				break;
			case SDLK_3:
				pal = fw.data->load_palette("xcom3/ufodata/PAL_03.DAT");
				break;
		}
	}
	else if (e->Type == EVENT_MOUSE_DOWN)
	{
		// FIXME: Object selection
	}
	else if (e->Type == EVENT_KEY_UP)
	{
		switch (e->Data.Keyboard.KeyCode)
		{
			case SDLK_UP:
				scrollUp = false;
				break;
			case SDLK_DOWN:
				scrollDown = false;
				break;
			case SDLK_LEFT:
				scrollLeft = false;
				break;
			case SDLK_RIGHT:
				scrollRight = false;
				break;
		}
	}
	else if (e->Type == EVENT_FINGER_MOVE)
	{
		// FIXME: Review this code for sanity
		if (e->Data.Finger.IsPrimary)
		{
			Vec2<float> deltaPos(e->Data.Finger.DeltaX, e->Data.Finger.DeltaY);
			if (this->viewMode == TileViewMode::Isometric)
			{
				deltaPos.x /= isoTileSize.x;
				deltaPos.y /= isoTileSize.y;
				Vec2<float> isoDelta(deltaPos.x + deltaPos.y, deltaPos.y - deltaPos.x);
				deltaPos = isoDelta;
			}
			else
			{
				deltaPos.x /= stratTileSize.x;
				deltaPos.y /= stratTileSize.y;
			}
			Vec2<float> newPos = this->centerPos - deltaPos;
			this->setScreenCenterTile(newPos);
		}
	}
	if (fw.gamecore->DebugModeEnabled && selectionChanged)
	{
		LogInfo("Selected tile {%d,%d,%d}", selectedTilePosition.x, selectedTilePosition.y,
		        selectedTilePosition.z);
	}
}

void TileView::Render()
{
	Renderer &r = *fw.renderer;
	r.clear();
	r.setPalette(this->pal);

	Vec2<float> newPos = this->centerPos;
	if (this->viewMode == TileViewMode::Isometric)
	{
		if (scrollLeft)
		{
			newPos.x -= isoScrollSpeed.x;
			newPos.y += isoScrollSpeed.y;
		}
		if (scrollRight)
		{
			newPos.x += isoScrollSpeed.x;
			newPos.y -= isoScrollSpeed.y;
		}
		if (scrollUp)
		{
			newPos.y -= isoScrollSpeed.y;
			newPos.x -= isoScrollSpeed.x;
		}
		if (scrollDown)
		{
			newPos.y += isoScrollSpeed.y;
			newPos.x += isoScrollSpeed.x;
		}
	}
	else if (this->viewMode == TileViewMode::Strategy)
	{
		if (scrollLeft)
			newPos.x -= stratScrollSpeed.x;
		if (scrollRight)
			newPos.x += stratScrollSpeed.x;
		if (scrollUp)
			newPos.y -= stratScrollSpeed.y;
		if (scrollDown)
			newPos.y += stratScrollSpeed.y;
	}
	else
	{
		LogError("Unknown view mode");
	}

	this->setScreenCenterTile(newPos);

	auto screenOffset = this->getScreenOffset();

	// screenOffset.x/screenOffset.y is the 'amount added to the tile coords' - so we want
	// the inverse to tell which tiles are at the screen bounds
	auto topLeft = screenToTileCoords(
	    Vec2<int>{-screenOffset.x - isoTileSize.x, -screenOffset.y - isoTileSize.y}, 0);
	auto topRight = screenToTileCoords(
	    Vec2<int>{-screenOffset.x + dpySize.x, -screenOffset.y - isoTileSize.y}, 0);
	auto bottomLeft = screenToTileCoords(
	    Vec2<int>{-screenOffset.x - isoTileSize.x, -screenOffset.y + dpySize.y}, map.size.z);
	auto bottomRight = screenToTileCoords(
	    Vec2<int>{-screenOffset.x + dpySize.x, -screenOffset.y + dpySize.y}, map.size.z);

	int minX = std::max(0, topLeft.x);
	int maxX = std::min(map.size.x, bottomRight.x);

	int minY = std::max(0, topRight.y);
	int maxY = std::min(map.size.y, bottomLeft.y);

	for (int z = 0; z < maxZDraw; z++)
	{
		for (int layer = 0; layer < map.getLayerCount(); layer++)
		{
			for (int y = minY; y < maxY; y++)
			{
				for (int x = minX; x < maxX; x++)
				{
					bool showSelected =
					    (fw.gamecore->DebugModeEnabled && z == selectedTilePosition.z &&
					     y == selectedTilePosition.y && x == selectedTilePosition.x);
					auto tile = map.getTile(x, y, z);
					auto screenPos = tileToScreenCoords(Vec3<float>{
					    static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)});
					screenPos.x += screenOffset.x;
					screenPos.y += screenOffset.y;

					if (showSelected)
						r.draw(selectedTileImageBack, screenPos);

					for (auto obj : tile->drawnObjects[layer])
					{
						Vec2<float> pos = tileToScreenCoords(obj->getPosition());
						pos.x += screenOffset.x;
						pos.y += screenOffset.y;
						obj->draw(r, *this, pos, this->viewMode);
					}

					if (showSelected)
						r.draw(selectedTileImageFront, screenPos);
				}
			}
		}
	}

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

		Vec2<float> topLeftRectPos = this->tileToScreenCoords(topLeftIsoTilePos);
		Vec2<float> topRightRectPos = this->tileToScreenCoords(topRightIsoTilePos);
		Vec2<float> bottomLeftRectPos = this->tileToScreenCoords(bottomLeftIsoTilePos);
		Vec2<float> bottomRightRectPos = this->tileToScreenCoords(bottomRightIsoTilePos);

		topLeftRectPos += screenOffset;
		topRightRectPos += screenOffset;
		bottomLeftRectPos += screenOffset;
		bottomRightRectPos += screenOffset;

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

bool TileView::IsTransition() { return false; }

void TileView::setViewMode(TileViewMode newMode) { this->viewMode = newMode; }

TileViewMode TileView::getViewMode() const { return this->viewMode; }

Vec2<int> TileView::getScreenOffset() const
{
	Vec2<float> screenOffset =
	    this->tileToScreenCoords(Vec3<float>{this->centerPos.x, this->centerPos.y, 0.0f});

	return Vec2<int>{dpySize.x / 2 - screenOffset.x, dpySize.y / 2 - screenOffset.y};
}

void TileView::setScreenCenterTile(Vec2<float> center)
{
	fw.soundBackend->setListenerPosition({center.x, center.y, map.size.z / 2});
	Vec2<float> clampedCenter;
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

	this->centerPos = clampedCenter;
}

}; // namespace OpenApoc
