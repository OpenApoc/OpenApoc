#include "tileview.h"
#include "tile.h"

#include "framework/includes.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "game/resources/gamecore.h"


namespace OpenApoc {

TileView::TileView(Framework &fw, TileMap &map, Vec3<float> tileSize)
	: Stage(fw), map(map), tileSize(tileSize), maxZDraw(10), offsetX(0), offsetY(0),
	  selectedTilePosition(0,0,0), selectedTileImageBack(fw.data.load_image("CITY/SELECTED-CITYTILE-BACK.PNG")),
	  selectedTileImageFront(fw.data.load_image("CITY/SELECTED-CITYTILE-FRONT.PNG"))
{
}

TileView::~TileView()
{
}

void TileView::Begin()
{
}

void TileView::Pause()
{
}

void TileView::Resume()
{
}

void TileView::Finish()
{
}


Vec2<float>
TileView::tileToScreenCoords(Vec3<float> c)
{
	float x = (c.x*tileSize.x/2) - (c.y*tileSize.x/2);
	float y = (c.x*tileSize.y/2) + (c.y*tileSize.y/2)
		- (c.z*tileSize.z);

	return Vec2<float>{x,y};
}

Vec3<float>
TileView::screenToTileCoords(Vec2<float> screenPos, float z)
{
	screenPos.y += (z*tileSize.z);
	float y = ((screenPos.y/(tileSize.y/2)) - (screenPos.x/(tileSize.x/2))) / (2);
	float x = ((screenPos.y/(tileSize.y/2)) + (screenPos.x/(tileSize.x/2))) / (2);

	return Vec3<float>{x,y,z};
}

void TileView::EventOccurred(Event *e)
{
	fw.gamecore->MouseCursor->EventOccured( e );
	bool selectionChanged = false;

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}

		switch (e->Data.Keyboard.KeyCode)
		{
			case ALLEGRO_KEY_UP:
				offsetY += tileSize.y;
				break;
			case ALLEGRO_KEY_DOWN:
				offsetY -= tileSize.y;
				break;
			case ALLEGRO_KEY_LEFT:
				offsetX += tileSize.x;
				break;
			case ALLEGRO_KEY_RIGHT:
				offsetX -= tileSize.x;
				break;

			case ALLEGRO_KEY_PGDN:
				if( maxZDraw > 0)
				{
					maxZDraw--;
				}
				break;
			case ALLEGRO_KEY_PGUP:
				if( maxZDraw < map.size.z)
				{
					maxZDraw++;
				}
				break;
			case ALLEGRO_KEY_S:
				selectionChanged = true;
				if (selectedTilePosition.y < (map.size.y-1))
					selectedTilePosition.y++;
				break;
			case ALLEGRO_KEY_W:
				selectionChanged = true;
				if (selectedTilePosition.y > 0)
					selectedTilePosition.y--;
				break;
			case ALLEGRO_KEY_A:
				selectionChanged = true;
				if (selectedTilePosition.x > 0)
					selectedTilePosition.x--;
				break;
			case ALLEGRO_KEY_D:
				selectionChanged = true;
				if (selectedTilePosition.x < (map.size.x-1))
					selectedTilePosition.x++;
				break;
			case ALLEGRO_KEY_R:
				selectionChanged = true;
				if (selectedTilePosition.z < (map.size.z-1))
					selectedTilePosition.z++;
				break;
			case ALLEGRO_KEY_F:
				selectionChanged = true;
				if (selectedTilePosition.z > 0)
					selectedTilePosition.z--;
				break;
		}
	}
	else if (e->Type == EVENT_MOUSE_DOWN)
	{
		auto &ev = e->Data.Mouse;
		auto selected = screenToTileCoords(Vec2<float>{(float)ev.X - offsetX, (float)ev.Y - offsetY}, (float)maxZDraw-1);

		if (selected.x < 0) selected.x = 0;
		if (selected.y < 0) selected.y = 0;
		if (selected.x > 99) selected.x = 99;
		if (selected.y > 99) selected.y = 99;
		selectedTilePosition = Vec3<int>{(int)selected.x, (int)selected.y, (int)selected.z};
		selectionChanged = true;
	}
	if (fw.gamecore->DebugModeEnabled &&
	    selectionChanged)
	{
		auto &tile = map.tiles[selectedTilePosition.z]
			[selectedTilePosition.y]
			[selectedTilePosition.x];
		std::cerr << "Selected tile x=" << selectedTilePosition.x <<
			" y=" << selectedTilePosition.y <<
			" z=" << selectedTilePosition.z << "\n";
	}
}

void TileView::Update(StageCmd * const cmd)
{
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void TileView::Render()
{
	int dpyWidth = fw.Display_GetWidth();
	int dpyHeight = fw.Display_GetHeight();
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	for (int y = 0; y < map.size.y; y++)
	{
		for (int z = 0; z < maxZDraw; z++)
		{
			for (int x = 0; x < map.size.x; x++)
			{
				bool showSelected =
					(fw.gamecore->DebugModeEnabled &&
					 z == selectedTilePosition.z &&
					 y == selectedTilePosition.y &&
					 x == selectedTilePosition.x);

				auto &tile = map.tiles[z][y][x];
				// Skip over transparent (missing) tiles
				auto screenPos = tileToScreenCoords(Vec3<float>{(float)x,(float)y,(float)z});
				screenPos.x += offsetX;
				screenPos.y += offsetY;
				//Skip over tiles that would be outside the window
				if (screenPos.x + tileSize.x < 0 || screenPos.y + tileSize.y < 0
					|| screenPos.x - tileSize.x > dpyWidth || screenPos.y - tileSize.y > dpyHeight)
					continue;

				if (showSelected)
					selectedTileImageBack->draw(screenPos.x, screenPos.y);
				for (auto obj : tile.objects)
				{
					if (obj->visible)
					{
						auto objScreenPos = tileToScreenCoords(obj->getPosition());
						objScreenPos.x += offsetX;
						objScreenPos.y += offsetY;
						auto &img = obj->getSprite();
						img.draw(objScreenPos.x, objScreenPos.y);
					}

				}

				if (showSelected)
					selectedTileImageFront->draw(screenPos.x, screenPos.y);
			}
		}
	}
	fw.gamecore->MouseCursor->Render();
}

bool TileView::IsTransition()
{
	return false;
}

}; //namespace OpenApoc
