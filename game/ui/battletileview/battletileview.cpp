#include "game/ui/battletileview/battletileview.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/includes.h"
#include "game/state/battle.h"

namespace OpenApoc
{

BattleTileView::BattleTileView(BattleTileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
                               TileViewMode initialMode)
    : Stage(), map(map), isoTileSize(isoTileSize), stratTileSize(stratTileSize),
      viewMode(initialMode), scrollUp(false), scrollDown(false), scrollLeft(false),
      scrollRight(false), dpySize(fw().displayGetWidth(), fw().displayGetHeight()),
      strategyViewBoxColour(212, 176, 172, 255), strategyViewBoxThickness(2.0f), hideGround(false),
      hideLeftWall(false), hideRightWall(false), hideScenery(false), currentZLevel(1),
      layerDrawingMode(BattleLayerDrawingMode::UpToCurrentLevel), maxZDraw(map.size.z),
      selectedTilePosition(0, 0, 0),
      selectedTileEmptyImageBack(fw().data->loadImage("battle/selected-battletile-empty-back.png")),
      selectedTileEmptyImageFront(
          fw().data->loadImage("battle/selected-battletile-empty-front.png")),
      selectedTileFilledImageBack(
          fw().data->loadImage("battle/selected-battletile-filled-back.png")),
      selectedTileFilledImageFront(
          fw().data->loadImage("battle/selected-battletile-filled-front.png")),
      selectedTileImageOffset(23, 42), centerPos(0, 0, 0), isoScrollSpeed(0.5, 0.5),
      stratScrollSpeed(2.0f, 2.0f), pal(fw().data->loadPalette("xcom3/tacdata/tactical.pal"))
{
	LogInfo("dpySize: {%d,%d}", dpySize.x, dpySize.y);
}

BattleTileView::~BattleTileView() = default;

void BattleTileView::begin() {}

void BattleTileView::pause() {}

void BattleTileView::resume() {}

void BattleTileView::finish() {}

void BattleTileView::eventOccurred(Event *e)
{

	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
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
			case SDLK_1:
				hideGround = !hideGround;
				break;
			case SDLK_2:
				hideLeftWall = !hideLeftWall;
				break;
			case SDLK_3:
				hideRightWall = !hideRightWall;
				break;
			case SDLK_4:
				hideScenery = !hideScenery;
				break;
			case SDLK_F6:
			{
				LogWarning("Writing voxel view to tileviewvoxels.png");
				auto imageOffset = -this->getScreenOffset();
				auto img = std::dynamic_pointer_cast<RGBImage>(
				    this->map.dumpVoxelView({imageOffset, imageOffset + dpySize}, *this));
				fw().data->writeImage("tileviewvoxels.png", img);
			}
		}
	}
	else if (e->type() == EVENT_KEY_UP)
	{
		switch (e->keyboard().KeyCode)
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
	else if (e->type() == EVENT_MOUSE_MOVE)
	{
		Vec2<float> screenOffset = {this->getScreenOffset().x, this->getScreenOffset().y};
		// Offset by 4 since ingame 4 is the typical height of the ground, and game displays cursor
		// on top of the ground
		setSelectedTilePosition(this->screenToTileCoords(
		    Vec2<float>((float)e->mouse().X, (float)e->mouse().Y + 4) - screenOffset,
		    currentZLevel - 1));
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
}

void BattleTileView::render()
{
	TRACE_FN;
	Renderer &r = *fw().renderer;
	r.clear();
	r.setPalette(this->pal);

	Vec3<float> newPos = this->centerPos;
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

	int zFrom;
	int zTo;

	switch (layerDrawingMode)
	{
		case BattleLayerDrawingMode::UpToCurrentLevel:
			zFrom = 0;
			zTo = currentZLevel;
			break;
		case BattleLayerDrawingMode::AllLevels:
			zFrom = 0;
			zTo = maxZDraw;
			break;
		case BattleLayerDrawingMode::OnlyCurrentLevel:
			zFrom = currentZLevel - 1;
			zTo = currentZLevel;
			break;
	}

	if (this->viewMode == TileViewMode::Isometric)
	{
		// Find out when to draw selection bracket parts (if ever)
		BattleTile *selectedTile = nullptr;
		sp<BattleTileObject> drawBackBeforeThis;
		sp<Image> selectionImageBack;
		sp<Image> selectionImageFront;
		if (selectedTilePosition.x >= minX && selectedTilePosition.x < maxX &&
		    selectedTilePosition.y >= minY && selectedTilePosition.y < maxY &&
		    selectedTilePosition.z >= zFrom && selectedTilePosition.z < zTo)
		{
			selectedTile =
			    map.getTile(selectedTilePosition.x, selectedTilePosition.y, selectedTilePosition.z);

			auto object_count = selectedTile->drawnObjects[0].size();
			bool foundUnit = false;
			for (size_t obj_id = 0; obj_id < object_count; obj_id++)
			{
				auto &obj = selectedTile->drawnObjects[0][obj_id];
				if (!drawBackBeforeThis && obj->getType() != BattleTileObject::Type::Ground)
					drawBackBeforeThis = obj;
				if (obj->getType() == BattleTileObject::Type::Unit)
					foundUnit = true;
			}
			if (foundUnit)
			{
				selectionImageBack = selectedTileFilledImageBack;
				selectionImageFront = selectedTileFilledImageFront;
			}
			else
			{
				selectionImageBack = selectedTileEmptyImageBack;
				selectionImageFront = selectedTileEmptyImageFront;
			}
		}

		for (int z = zFrom; z < zTo; z++)
		{
			for (int layer = 0; layer < map.getLayerCount(); layer++)
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
							// Back selection image is drawn between ground image and everything
							// else
							if (layer == 0 && tile == selectedTile && obj == drawBackBeforeThis)
								r.draw(selectedTileEmptyImageBack,
								       tileToOffsetScreenCoords(selectedTilePosition) -
								           selectedTileImageOffset);
							// FIXME: Remove this when renderer is working 100% properly to make
							// rendering faster
							if (((obj->getType() == BattleTileObject::Type::Ground) &&
							     hideGround) ||
							    ((obj->getType() == BattleTileObject::Type::LeftWall) &&
							     hideLeftWall) ||
							    ((obj->getType() == BattleTileObject::Type::RightWall) &&
							     hideRightWall) ||
							    ((obj->getType() == BattleTileObject::Type::Scenery) &&
							     hideScenery))
								continue;
							Vec2<float> pos = tileToOffsetScreenCoords(obj->getPosition());
							obj->draw(r, *this, pos, this->viewMode);
						}
						// When on the last layer, draw the front selection image (and back
						// selection image if we haven't yet)
						if (layer == 0 && tile == selectedTile)
						{
							// Back selection image is drawn here if it hasn't been drawn yet
							if (!drawBackBeforeThis)
								r.draw(selectedTileEmptyImageBack,
								       tileToOffsetScreenCoords(selectedTilePosition) -
								           selectedTileImageOffset);
							r.draw(selectedTileEmptyImageFront,
							       tileToOffsetScreenCoords(selectedTilePosition) -
							           selectedTileImageOffset);
						}
					}
				}
			}
		}
	}
	if (this->viewMode == TileViewMode::Strategy)
	{
		for (int z = zFrom; z < zTo; z++)
		{
			for (int layer = 0; layer < map.getLayerCount(); layer++)
			{
				for (int y = minY; y < maxY; y++)
				{
					for (int x = minX; x < maxX; x++)
					{
						bool selected = selectedTilePosition.x == x &&
						                selectedTilePosition.y == y && selectedTilePosition.z == z;
						auto tile = map.getTile(x, y, z);
						auto object_count = tile->drawnObjects[layer].size();
						for (size_t obj_id = 0; obj_id < object_count; obj_id++)
						{
							auto &obj = tile->drawnObjects[layer][obj_id];
							// FIXME: Remove this when renderer is working 100% properly to make
							// rendering faster
							if (((obj->getType() == BattleTileObject::Type::Ground) &&
							     hideGround) ||
							    ((obj->getType() == BattleTileObject::Type::LeftWall) &&
							     hideLeftWall) ||
							    ((obj->getType() == BattleTileObject::Type::RightWall) &&
							     hideRightWall) ||
							    ((obj->getType() == BattleTileObject::Type::Scenery) &&
							     hideScenery))
								continue;
							Vec2<float> pos = tileToOffsetScreenCoords(obj->getPosition());
							obj->draw(r, *this, pos, this->viewMode);
						}
					}
				}
			}
		}

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

void BattleTileView::setZLevel(int zLevel)
{
	currentZLevel = clamp(zLevel, 1, maxZDraw);
	setScreenCenterTile(Vec3<float>{centerPos.x, centerPos.y, currentZLevel});
}

int BattleTileView::getZLevel() { return currentZLevel; }

void BattleTileView::setLayerDrawingMode(BattleLayerDrawingMode mode) { layerDrawingMode = mode; }

bool BattleTileView::isTransition() { return false; }

void BattleTileView::setViewMode(TileViewMode newMode) { this->viewMode = newMode; }

TileViewMode BattleTileView::getViewMode() const { return this->viewMode; }

Vec2<int> BattleTileView::getScreenOffset() const
{
	Vec2<float> screenOffset = this->tileToScreenCoords(this->centerPos);

	return Vec2<int>{dpySize.x / 2 - screenOffset.x, dpySize.y / 2 - screenOffset.y};
}

void BattleTileView::setScreenCenterTile(Vec3<float> center)
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

void BattleTileView::setScreenCenterTile(Vec2<float> center)
{
	this->setScreenCenterTile(Vec3<float>{center.x, center.y, currentZLevel});
}

Vec3<int> BattleTileView::getSelectedTilePosition() { return selectedTilePosition; }

void BattleTileView::setSelectedTilePosition(Vec3<int> newPosition)
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

}; // namespace OpenApoc
