#include "cityview.h"
#include "city.h"

CityView::CityView()
	: pal(new Palette("UFODATA/PAL_04.DAT")),
	  cityPck(new PCK("UFODATA/CITY.PCK", "UFODATA/CITY.TAB", pal.get())),
	  offsetX(0), offsetY(0), maxZDraw(2)
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
		}
	}
}

void CityView::Update()
{
}

void CityView::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	for (int y = 0; y < CITY->sizeY; y++)
	{
		for (int z = 0; z < maxZDraw; z++)
		{
			for (int x = 0; x < CITY->sizeX; x++)
			{
				auto &tile = CITY->tiles[z][y][x];
				// Skip over transparent (missing) tiles
				if (tile.id == 0)
					continue;
				int posX = offsetX
					+ (x*CITY_TILE_WIDTH/2)
					- (y*CITY_TILE_WIDTH/2);
				int posY = offsetY
					+ (x*CITY_TILE_HEIGHT/2)
					+ (y*CITY_TILE_HEIGHT/2)
					+ (z*CITY_TILE_ZOFFSET);

				//Skip over tiles that would be outside the window
				if (posX + CITY_TILE_WIDTH < 0 || posY + CITY_TILE_HEIGHT < 0
					|| posX - CITY_TILE_WIDTH > 640 || posY - CITY_TILE_HEIGHT > 480)
					continue;

				cityPck->RenderImage(tile.id, posX, posY);
			}
		}
	}
	GAMECORE->MouseCursor->Render();
}

bool CityView::IsTransition()
{
	return false;
}
