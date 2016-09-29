#include "game/ui/tileview/citytileview.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/includes.h"

namespace OpenApoc
{
CityTileView::CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
                           TileViewMode initialMode)
    : TileView(map, isoTileSize, stratTileSize, initialMode)
{
	selectedTileImageBack = fw().data->loadImage("city/selected-citytile-back.png");
	selectedTileImageFront = fw().data->loadImage("city/selected-citytile-front.png");
	selectedTileImageOffset = {32, 16};
	pal = fw().data->loadPalette("xcom3/ufodata/pal_01.dat");
};

CityTileView::~CityTileView() = default;

void CityTileView::eventOccurred(Event *e)
{
	if (e->type() == EVENT_KEY_DOWN &&
	    (e->keyboard().KeyCode == SDLK_1 || e->keyboard().KeyCode == SDLK_2 ||
	     e->keyboard().KeyCode == SDLK_3 || e->keyboard().KeyCode == SDLK_F6 ||
	     e->keyboard().KeyCode == SDLK_F7))
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_1:
				pal = fw().data->loadPalette("xcom3/ufodata/pal_01.dat");
				break;
			case SDLK_2:
				pal = fw().data->loadPalette("xcom3/ufodata/pal_02.dat");
				break;
			case SDLK_3:
				pal = fw().data->loadPalette("xcom3/ufodata/pal_03.dat");
				break;
			case SDLK_F6:
			{
				LogWarning("Writing voxel view to tileviewvoxels.png");
				auto imageOffset = -this->getScreenOffset();
				auto img = std::dynamic_pointer_cast<RGBImage>(
				    this->map.dumpVoxelView({imageOffset, imageOffset + dpySize}, *this, 11.0f));
				fw().data->writeImage("tileviewvoxels.png", img);
			}
			break;
			case SDLK_F7:
			{
				LogWarning("Writing voxel view (fast) to tileviewvoxels.png");
				auto imageOffset = -this->getScreenOffset();
				auto img = std::dynamic_pointer_cast<RGBImage>(this->map.dumpVoxelView(
				    {imageOffset, imageOffset + dpySize}, *this, 11.0f, true));
				fw().data->writeImage("tileviewvoxels.png", img);
			}
			break;
		}
	}
	else
	{
		TileView::eventOccurred(e);
	}
}

void CityTileView::render()
{
	TRACE_FN;
	Renderer &r = *fw().renderer;
	r.clear();
	r.setPalette(this->pal);

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

	for (int z = 0; z < maxZDraw; z++)
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
						Vec2<float> pos = tileToOffsetScreenCoords(obj->getCenter());
						obj->draw(r, *this, pos, this->viewMode);
					}
#ifdef PATHFINDING_DEBUG
					if (tile->pathfindingDebugFlag && viewMode == TileViewMode::Isometric)
						r.draw(selectedTileImageFront,
						       tileToOffsetScreenCoords(Vec3<int>{x, y, z}) -
						           selectedTileImageOffset);
#endif
				}
			}
		}
	}

	renderStrategyOverlay(r);
}
}