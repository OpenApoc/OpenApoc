#include "game/ui/tileview/citytileview.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/keycodes.h"
#include "framework/renderer.h"
#include "framework/trace.h"
#include "game/state/city/building.h"
#include "game/state/city/city.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{
CityTileView::CityTileView(TileMap &map, Vec3<int> isoTileSize, Vec2<int> stratTileSize,
                           TileViewMode initialMode, GameState &gameState)
    : TileView(map, isoTileSize, stratTileSize, initialMode), state(gameState)
{
	selectedTileImageBack = fw().data->loadImage("city/selected-citytile-back.png");
	selectedTileImageFront = fw().data->loadImage("city/selected-citytile-front.png");
	selectedTileImageOffset = {32, 16};
	pal = fw().data->loadPalette("xcom3/ufodata/pal_01.dat");
	alienDetectionColour = {212, 0, 0, 255};
	alienDetectionThickness = 3.0f;
};

CityTileView::~CityTileView() = default;

void CityTileView::eventOccurred(Event *e)
{
	if (e->type() == EVENT_KEY_DOWN)
	{
		switch (e->keyboard().KeyCode)
		{
			case SDLK_1:
				pal = fw().data->loadPalette("xcom3/ufodata/pal_01.dat");
				return;
			case SDLK_2:
				pal = fw().data->loadPalette("xcom3/ufodata/pal_02.dat");
				return;
			case SDLK_3:
				pal = fw().data->loadPalette("xcom3/ufodata/pal_03.dat");
				return;
			case SDLK_F10:
			{
				DEBUG_SHOW_ALIEN_CREW = !DEBUG_SHOW_ALIEN_CREW;
				LogWarning("Debug Alien display set to %s", DEBUG_SHOW_ALIEN_CREW);
			}
				return;
			case SDLK_F6:
			{
				LogWarning("Writing voxel view to tileviewvoxels.png");
				auto imageOffset = -this->getScreenOffset();
				auto img = std::dynamic_pointer_cast<RGBImage>(
				    this->map.dumpVoxelView({imageOffset, imageOffset + dpySize}, *this, 11.0f));
				fw().data->writeImage("tileviewvoxels.png", img);
			}
				return;
			case SDLK_F7:
			{
				LogWarning("Writing voxel view (fast) to tileviewvoxels.png");
				auto imageOffset = -this->getScreenOffset();
				auto img = std::dynamic_pointer_cast<RGBImage>(this->map.dumpVoxelView(
				    {imageOffset, imageOffset + dpySize}, *this, 11.0f, true));
				fw().data->writeImage("tileviewvoxels.png", img);
			}
				return;
			case SDLK_F8:
			{
				LogWarning("Writing voxel view to tileviewvoxels.png");
				auto imageOffset = -this->getScreenOffset();
				auto img = std::dynamic_pointer_cast<RGBImage>(this->map.dumpVoxelView(
				    {imageOffset, imageOffset + dpySize}, *this, 11.0f, false, true));
				fw().data->writeImage("tileviewvoxels.png", img);
			}
				return;
			case SDLK_F9:
			{
				LogWarning("Writing voxel view (fast) to tileviewvoxels.png");
				auto imageOffset = -this->getScreenOffset();
				auto img = std::dynamic_pointer_cast<RGBImage>(this->map.dumpVoxelView(
				    {imageOffset, imageOffset + dpySize}, *this, 11.0f, true, true));
				fw().data->writeImage("tileviewvoxels.png", img);
			}
				return;
		}
	}
	TileView::eventOccurred(e);
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

	// Detection
	if (this->viewMode == TileViewMode::Strategy)
	{
		for (auto &b : state.current_city->buildings)
		{
			if (!b.second->detected)
			{
				continue;
			}
			Vec2<float> pos = tileToOffsetScreenCoords(
			    Vec3<int>{(b.second->bounds.p0.x + b.second->bounds.p1.x) / 2,
			              (b.second->bounds.p0.y + b.second->bounds.p1.y) / 2, 2});

			float radius =
			    70.0f * (float)b.second->ticksDetectionTimeOut / (float)TICKS_DETECTION_TIMEOUT +
			    30.0f;
			float interval = M_PI / 8.0f;
			float angle = 0.0f;
			Vec2<float> posNew = {pos.x + radius, pos.y};
			auto posOld = posNew;
			while (angle < M_PI * 2.0f)
			{
				angle += interval;
				posOld = posNew;
				posNew = {pos.x + cos(angle) * radius, pos.y + sin(angle) * radius};
				r.drawLine(posOld, posNew, alienDetectionColour, alienDetectionThickness);
			}
		}
	}

	// Alien debug display
	if (this->viewMode == TileViewMode::Strategy && DEBUG_SHOW_ALIEN_CREW)
	{
		for (auto &b : state.current_city->buildings)
		{
			Vec2<float> pos = tileToOffsetScreenCoords(
			    Vec3<int>{b.second->bounds.p0.x, b.second->bounds.p0.y, 2});
			for (auto &a : b.second->current_crew)
			{
				for (int i = 0; i < a.second; i++)
				{
					auto icon =
					    a.first->portraits.at(*a.first->possible_genders.begin()).at(0).icon;
					r.draw(icon, pos);
					pos.x += icon->size.x / 2;
				}
			}
		}
	}
}
}
