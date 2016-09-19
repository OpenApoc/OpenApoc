#include "game/ui/tileview/battletileview.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/includes.h"
#include "game/state/battle/battle.h"

namespace OpenApoc
{
BattleTileView::BattleTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize, TileViewMode initialMode)
	:TileView(map, isoTileSize, stratTileSize, initialMode), currentZLevel(1)
{
	layerDrawingMode = LayerDrawingMode::UpToCurrentLevel;
	selectedTileEmptyImageBack =
		fw().data->loadImage("battle/selected-battletile-empty-back.png");
	selectedTileEmptyImageFront =
		fw().data->loadImage("battle/selected-battletile-empty-front.png");
	selectedTileFilledImageBack =
		fw().data->loadImage("battle/selected-battletile-filled-back.png");
	selectedTileFilledImageFront =
		fw().data->loadImage("battle/selected-battletile-filled-front.png");
	selectedTileBackgroundImageBack =
		fw().data->loadImage("battle/selected-battletile-background-back.png");
	selectedTileBackgroundImageFront =
		fw().data->loadImage("battle/selected-battletile-background-front.png");
	selectedTileImageOffset = { 23, 22 };
	pal = fw().data->loadPalette("xcom3/tacdata/tactical.pal");
};

BattleTileView::~BattleTileView() = default;


void BattleTileView::eventOccurred(Event *e)
{
	if (e->type() == EVENT_KEY_DOWN && (
		e->keyboard().KeyCode == SDLK_F6
		|| e->keyboard().KeyCode == SDLK_F6
		))
	{
		switch (e->keyboard().KeyCode)
		{
		
		case SDLK_F6:
		{
			LogWarning("Writing voxel view to tileviewvoxels.png");
			auto imageOffset = -this->getScreenOffset();
			auto img = std::dynamic_pointer_cast<RGBImage>(
				this->map.dumpVoxelView({ imageOffset, imageOffset + dpySize }, *this,
					currentZLevel));
			fw().data->writeImage("tileviewvoxels.png", img);
		}
		break;
		}
	}
	else if (e->type() == EVENT_MOUSE_MOVE)
	{
		Vec2<float> screenOffset = { this->getScreenOffset().x, this->getScreenOffset().y };
		// Offset by 4 since ingame 4 is the typical height of the ground, and game displays cursor
		// on top of the ground
		setSelectedTilePosition(this->screenToTileCoords(
			Vec2<float>((float)e->mouse().X, (float)e->mouse().Y + 4 - 20) - screenOffset,
			currentZLevel - 1));
	}
	else
	{
		TileView::eventOccurred(e);
	}
}

void BattleTileView::render()
{
	TRACE_FN;
	Renderer &r = *fw().renderer;
	r.clear();
	r.setPalette(this->pal);

	applyScrolling();

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
		zTo = currentZLevel;
		break;
	case LayerDrawingMode::AllLevels:
		zFrom = 0;
		zTo = maxZDraw;
		break;
	case LayerDrawingMode::OnlyCurrentLevel:
		zFrom = currentZLevel - 1;
		zTo = currentZLevel;
		break;
	}

	// FIXME: A different algorithm is required in order to properly display big units.
	/*
	1) Rendering must go in diagonal lines. Illustration:

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
	When encountering a 2-tile object, we inrement "skip next X tiles" by 1.
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

	// FIXME: Draw double selection bracket for big units?

	for (int z = zFrom; z < zTo; z++)
	{
		int currentLevel = z - currentZLevel + 1;

		// Find out when to draw selection bracket parts (if ever)
		Tile *selTileOnCurLevel = nullptr;
		Vec3<int> selTilePosOnCurLevel;
		sp<TileObject> drawBackBeforeThis;
		sp<Image> selectionImageBack;
		sp<Image> selectionImageFront;
		if (this->viewMode == TileViewMode::Isometric)
		{
			if (selectedTilePosition.z >= z &&
				selectedTilePosition.x >= minX && selectedTilePosition.x < maxX &&
				selectedTilePosition.y >= minY && selectedTilePosition.y < maxY)
			{
				selTilePosOnCurLevel = { selectedTilePosition.x, selectedTilePosition.y, z };
				selTileOnCurLevel = map.getTile(selTilePosOnCurLevel.x, selTilePosOnCurLevel.y,
					selTilePosOnCurLevel.z);

				// Find where to draw back selection bracket
				auto object_count = selTileOnCurLevel->drawnObjects[0].size();
				for (size_t obj_id = 0; obj_id < object_count; obj_id++)
				{
					auto &obj = selTileOnCurLevel->drawnObjects[0][obj_id];
					if (!drawBackBeforeThis && obj->getType() != TileObject::Type::Ground)
						drawBackBeforeThis = obj;
				}
				// Find what kind of selection bracket to draw (yellow or green)
				// Yellow if this tile intersects with a unit
				if (selectedTilePosition.z == z)
				{
					if (selTileOnCurLevel->getUnitIfPresent())
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
				else
				{
					selectionImageBack = selectedTileBackgroundImageBack;
					selectionImageFront = selectedTileBackgroundImageFront;
				}
			}
		}

		// Actually draw stuff
		for (int layer = 0; layer < map.getLayerCount(); layer++)
		{
			for (int y = minY; y < maxY; y++)
			{
				for (int x = minX; x < maxX; x++)
				{
					auto tile = map.getTile(x, y, z);
					auto object_count = tile->drawnObjects[layer].size();
					// I assume splitting it here will improve performance?
					if (tile == selTileOnCurLevel && layer == 0)
					{
						for (size_t obj_id = 0; obj_id < object_count; obj_id++)
						{
							auto &obj = tile->drawnObjects[layer][obj_id];
							// Back selection image is drawn
							// between ground image and everything else
							if (obj == drawBackBeforeThis)
								r.draw(selectionImageBack,
									tileToOffsetScreenCoords(selTilePosOnCurLevel) -
									selectedTileImageOffset);
							Vec2<float> pos = tileToOffsetScreenCoords(obj->getPosition());
							obj->draw(r, *this, pos, this->viewMode, currentLevel);
						}
						// When done with all objects, draw the front selection image
						// (and back selection image if we haven't yet)
						if (!drawBackBeforeThis)
							r.draw(selectionImageBack,
								tileToOffsetScreenCoords(selTilePosOnCurLevel) -
								selectedTileImageOffset);
						r.draw(selectionImageFront,
							tileToOffsetScreenCoords(selTilePosOnCurLevel) -
							selectedTileImageOffset);
					}
					else
					{
						auto tile = map.getTile(x, y, z);
						auto object_count = tile->drawnObjects[layer].size();
						for (size_t obj_id = 0; obj_id < object_count; obj_id++)
						{
							auto &obj = tile->drawnObjects[layer][obj_id];
							Vec2<float> pos = tileToOffsetScreenCoords(obj->getPosition());
							obj->draw(r, *this, pos, this->viewMode, currentLevel);
						}
					}
				}
			}
		}
	}

	renderStrategyOverlay(r);
}

void BattleTileView::setZLevel(int zLevel)
{
	currentZLevel = clamp(zLevel, 1, maxZDraw);
	setScreenCenterTile(Vec3<float>{centerPos.x, centerPos.y, currentZLevel - 1});
}

int BattleTileView::getZLevel() { return currentZLevel; }

void BattleTileView::setLayerDrawingMode(LayerDrawingMode mode)
{
	layerDrawingMode = mode;
}


}