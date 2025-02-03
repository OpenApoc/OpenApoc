#include "framework/apocresources/cursor.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/logger.h"
#include "framework/renderer.h"
#include "library/sp.h"

namespace OpenApoc
{

ApocCursor::ApocCursor(sp<Palette> pal) : cursorPos{0, 0}
{
	auto f = fw().data->fs.open("xcom3/tacdata/mouse.dat");
	if (!f)
	{
		LogError("Failed to open xcom3/tacdata/mouse.dat");
		return;
	}

	auto cursorCount = f.size() / 576;

	while (images.size() < cursorCount)
	{
		auto palImg = mksp<PaletteImage>(Vec2<int>{24, 24});
		PaletteImageLock l(palImg, ImageLockUse::Write);
		for (int y = 0; y < 24; y++)
		{
			for (int x = 0; x < 24; x++)
			{
				char palidx;
				f.read(&palidx, 1);
				l.set(Vec2<int>{x, y}, palidx);
			}
		}
		images.push_back(palImg->toRGBImage(pal));
	}

	CurrentType = CursorType::Normal;
}

ApocCursor::~ApocCursor() = default;

void ApocCursor::eventOccured(Event *e)
{
	if (e->type() == EVENT_MOUSE_MOVE)
	{
		cursorPos.x = e->mouse().X;
		cursorPos.y = e->mouse().Y;
	}
}

void ApocCursor::render()
{
	fw().renderer->draw(images.at(static_cast<int>(CurrentType)),
	                    Vec2<float>{cursorPos.x, cursorPos.y});
}

}; // namespace OpenApoc
