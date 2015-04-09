
#include "cursor.h"
#include "framework/framework.h"
#include "framework/palette.h"

namespace OpenApoc {

Cursor::Cursor( Framework &fw, std::shared_ptr<Palette> pal )
	: fw(fw), cursorPos{0,0}
{
	ALLEGRO_FILE* f = fw.data.load_file( "TACDATA/MOUSE.DAT", "rb" );

	while( images.size() < al_fsize( f ) / 576 )
	{
		auto palImg = std::make_shared<PaletteImage>(Vec2<int>{24,24});
		PaletteImageLock l(palImg, ImageLockUse::Write);
		for( int y = 0; y < 24; y++ )
		{
			for( int x = 0; x < 24; x++ )
			{
				int palidx = al_fgetc( f );
				l.set(Vec2<int>{x,y}, palidx);
			}
		}
		images.push_back(palImg->toRGBImage(pal));
	}

	al_fclose(f);

	CurrentType = Cursor::Normal;
}

Cursor::~Cursor()
{
}

void Cursor::EventOccured( Event* e )
{
	if( e->Type == EVENT_MOUSE_MOVE )
	{
		cursorPos.x = e->Data.Mouse.X;
		cursorPos.y = e->Data.Mouse.Y;
	}
}

void Cursor::Render()
{
	fw.renderer->draw(images.at((int)CurrentType), Vec2<float>{cursorPos.x, cursorPos.y});
}
}; //namespace OpenApoc
