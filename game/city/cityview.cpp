#include "cityview.h"
#include "city.h"

#include <iostream>

CityView::CityView()
	: pal(new Palette("UFODATA/PAL_04.DAT")),
	  cityPck(new PCK("UFODATA/CITY.PCK", "UFODATA/CITY.TAB", *pal)),
	  offsetX(0), offsetY(0), maxZDraw(2),
	  selectedTilePosition(0,0,0), selectedTileImageBack(DATA->load_image("CITY/SELECTED-CITYTILE-BACK.PNG")),
	  selectedTileImageFront(DATA->load_image("CITY/SELECTED-CITYTILE-FRONT.PNG"))
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

void CityView::EventOccurred(Event *e)
{
	GAMECORE->MouseCursor->EventOccured( e );
	bool selectionChanged = false;

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			delete FRAMEWORK->ProgramStages->Pop();
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
				if( maxZDraw < CITY->sizeZ)
				{
					maxZDraw++;
				}
				break;
			case ALLEGRO_KEY_S:
				selectionChanged = true;
				if (selectedTilePosition.y < (CITY->sizeY-1))
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
				if (selectedTilePosition.x < (CITY->sizeX-1))
					selectedTilePosition.x++;
				break;
			case ALLEGRO_KEY_R:
				selectionChanged = true;
				if (selectedTilePosition.z < (CITY->sizeZ-1))
					selectedTilePosition.z++;
				break;
			case ALLEGRO_KEY_F:
				selectionChanged = true;
				if (selectedTilePosition.z > 0)
					selectedTilePosition.z--;
				break;
		}
	}
	if (GAMECORE->DebugModeEnabled &&
	    selectionChanged)
	{
		auto &tile = CITY->tiles[selectedTilePosition.z]
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

void CityView::Update()
{
}

void CityView::Render()
{
	int dpyWidth = FRAMEWORK->Display_GetWidth();
	int dpyHeight = FRAMEWORK->Display_GetHeight();
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	for (int y = 0; y < CITY->sizeY; y++)
	{
		for (int z = 0; z < maxZDraw; z++)
		{
			for (int x = 0; x < CITY->sizeX; x++)
			{
				bool showSelected =
					(GAMECORE->DebugModeEnabled &&
					 z == selectedTilePosition.z &&
					 y == selectedTilePosition.y &&
					 x == selectedTilePosition.x);

				auto &tile = CITY->tiles[z][y][x];
				// Skip over transparent (missing) tiles
				int posX = offsetX
					+ (x*CITY_TILE_WIDTH/2)
					- (y*CITY_TILE_WIDTH/2);
				int posY = offsetY
					+ (x*CITY_TILE_HEIGHT/2)
					+ (y*CITY_TILE_HEIGHT/2)
					+ (z*CITY_TILE_ZOFFSET);

				//Skip over tiles that would be outside the window
				if (posX + CITY_TILE_WIDTH < 0 || posY + CITY_TILE_HEIGHT < 0
					|| posX - CITY_TILE_WIDTH > dpyWidth || posY - CITY_TILE_HEIGHT > dpyHeight)
					continue;

				if (showSelected)
					selectedTileImageBack->draw(posX, posY);

				if (tile.id != 0)
					cityPck->RenderImage(tile.id, posX, posY);

				if (showSelected)
					selectedTileImageFront->draw(posX, posY);
			}
		}
	}
	GAMECORE->MouseCursor->Render();
}

bool CityView::IsTransition()
{
	return false;
}
