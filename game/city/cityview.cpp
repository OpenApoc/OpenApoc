#include "cityview.h"
#include "city.h"

#include "framework/includes.h"


namespace OpenApoc {

CityView::CityView()
	: pal(new Palette("UFODATA/PAL_04.DAT")),
	  cityPck(new PCK("UFODATA/CITY.PCK", "UFODATA/CITY.TAB", *pal)),
	  offsetX(0), offsetY(0), maxZDraw(2),
	  selectedTilePosition(0,0,0), selectedTileImageBack(FRAMEWORK->data.load_image("CITY/SELECTED-CITYTILE-BACK.PNG")),
	  selectedTileImageFront(FRAMEWORK->data.load_image("CITY/SELECTED-CITYTILE-FRONT.PNG"))
{
}

CityView::~CityView()
{
}

void CityView::Begin()
{
}

void CityView::Pause()
{
}

void CityView::Resume()
{
}

void CityView::Finish()
{
}


static Vec2<int> translateCityToScreenCoords(Vec3<int> c)
{
	int x = (c.x*CITY_TILE_WIDTH/2) - (c.y*CITY_TILE_WIDTH/2);
	int y = (c.x*CITY_TILE_HEIGHT/2) + (c.y*CITY_TILE_HEIGHT/2)
		+ (c.z*CITY_TILE_ZOFFSET);

	return Vec2<int>{x,y};
}

static Vec3<int> translateScreenToCityCoords(Vec2<int> screenPos, int z)
{
	screenPos.y -= (z*CITY_TILE_ZOFFSET);
	int y = ((screenPos.y/(CITY_TILE_HEIGHT/2)) - (screenPos.x/(CITY_TILE_WIDTH/2))) / (2);
	int x = ((screenPos.y/(CITY_TILE_HEIGHT/2)) + (screenPos.x/(CITY_TILE_WIDTH/2))) / (2);

	return Vec3<int>{x,y,z};
}

void CityView::EventOccurred(Event *e)
{
	FRAMEWORK->gamecore->MouseCursor->EventOccured( e );
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
				offsetY += CITY_TILE_HEIGHT;
				break;
			case ALLEGRO_KEY_DOWN:
				offsetY -= CITY_TILE_HEIGHT;
				break;
			case ALLEGRO_KEY_LEFT:
				offsetX += CITY_TILE_WIDTH;
				break;
			case ALLEGRO_KEY_RIGHT:
				offsetX -= CITY_TILE_WIDTH;
				break;

			case ALLEGRO_KEY_PGDN:
				if( maxZDraw > 0)
				{
					maxZDraw--;
				}
				break;
			case ALLEGRO_KEY_PGUP:
				if( maxZDraw < FRAMEWORK->state.city->sizeZ)
				{
					maxZDraw++;
				}
				break;
			case ALLEGRO_KEY_S:
				selectionChanged = true;
				if (selectedTilePosition.y < (FRAMEWORK->state.city->sizeY-1))
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
				if (selectedTilePosition.x < (FRAMEWORK->state.city->sizeX-1))
					selectedTilePosition.x++;
				break;
			case ALLEGRO_KEY_R:
				selectionChanged = true;
				if (selectedTilePosition.z < (FRAMEWORK->state.city->sizeZ-1))
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
		auto selected = translateScreenToCityCoords(Vec2<int>{ev.X - offsetX, ev.Y - offsetY}, maxZDraw-1);

		if (selected.x < 0) selected.x = 0;
		if (selected.y < 0) selected.y = 0;
		if (selected.x > 99) selected.x = 99;
		if (selected.y > 99) selected.y = 99;
		selectedTilePosition = selected;
		selectionChanged = true;
	}
	if (FRAMEWORK->gamecore->DebugModeEnabled &&
	    selectionChanged)
	{
		auto &tile = FRAMEWORK->state.city->tiles[selectedTilePosition.z]
			[selectedTilePosition.y]
			[selectedTilePosition.x];
		std::cout << "Selection: X=" << selectedTilePosition.x
			<< " Y=" << selectedTilePosition.y
			<< " Z=" << selectedTilePosition.z
			<< " TileID =" << tile.id << "\n";
		if (tile.building)
		{
			std::cout << "\tBuilding: \"" << tile.building->name
				<< "\" Owner: \"" << tile.building->owner.name << "\"\n";
		}
		else
		{
			std::cout << "\tNot part of building\n";
		}
	}
}

void CityView::Update(StageCmd * const cmd)
{
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void CityView::Render()
{
	int dpyWidth = FRAMEWORK->Display_GetWidth();
	int dpyHeight = FRAMEWORK->Display_GetHeight();
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	for (int y = 0; y < FRAMEWORK->state.city->sizeY; y++)
	{
		for (int z = 0; z < maxZDraw; z++)
		{
			for (int x = 0; x < FRAMEWORK->state.city->sizeX; x++)
			{
				bool showSelected =
					(FRAMEWORK->gamecore->DebugModeEnabled &&
					 z == selectedTilePosition.z &&
					 y == selectedTilePosition.y &&
					 x == selectedTilePosition.x);

				auto &tile = FRAMEWORK->state.city->tiles[z][y][x];
				// Skip over transparent (missing) tiles
				auto screenPos = translateCityToScreenCoords(Vec3<int>{x,y,z});
				screenPos.x += offsetX;
				screenPos.y += offsetY;
				//Skip over tiles that would be outside the window
				if (screenPos.x + CITY_TILE_WIDTH < 0 || screenPos.y + CITY_TILE_HEIGHT < 0
					|| screenPos.x - CITY_TILE_WIDTH > dpyWidth || screenPos.y - CITY_TILE_HEIGHT > dpyHeight)
					continue;

				if (showSelected)
					selectedTileImageBack->draw(screenPos.x, screenPos.y);

				if (tile.id != 0)
					cityPck->RenderImage(tile.id, screenPos.x, screenPos.y);

				if (showSelected)
					selectedTileImageFront->draw(screenPos.x, screenPos.y);
			}
		}
	}
	FRAMEWORK->gamecore->MouseCursor->Render();
}

bool CityView::IsTransition()
{
	return false;
}

}; //namespace OpenApoc
