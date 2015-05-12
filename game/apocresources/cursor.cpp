
#include "game/apocresources/cursor.h"
#include "framework/framework.h"
#include "framework/palette.h"

namespace OpenApoc {

Cursor::Cursor( Framework &fw, std::shared_ptr<Palette> pal )
	: fw(fw), cursorPos{0,0}
{
	PHYSFS_file* f = fw.data->load_file( "xcom3/TACDATA/MOUSE.DAT", "rb" );

	size_t fileLength = PHYSFS_fileLength(f) / 576;

	while( images.size() < fileLength )
	{
		auto palImg = std::make_shared<PaletteImage>(Vec2<int>{24,24});
		PaletteImageLock l(palImg, ImageLockUse::Write);
		for( int y = 0; y < 24; y++ )
		{
			for( int x = 0; x < 24; x++ )
			{
				char palidx;
				PHYSFS_readBytes(f, &palidx, 1);
				l.set(Vec2<int>{x,y}, palidx);
			}
		}
		images.push_back(palImg->toRGBImage(pal));
	}

	PHYSFS_close(f);

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
