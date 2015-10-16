
#include "game/apocresources/cursor.h"
#include "framework/framework.h"
#include "framework/palette.h"

namespace OpenApoc
{

ApocCursor::ApocCursor(Framework &fw, std::shared_ptr<Palette> pal) : fw(fw), cursorPos{0, 0}
{
	auto f = fw.data->fs.open("xcom3/TACDATA/MOUSE.DAT");
	if (!f)
	{
		LogError("Failed to open xcom3/TACDATA/MOUSE.DAT");
		return;
	}

	auto cursorCount = f.size() / 576;

	while (images.size() < cursorCount)
	{
		auto palImg = std::make_shared<PaletteImage>(Vec2<int>{24, 24});
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

	CurrentType = ApocCursor::Normal;
}

ApocCursor::~ApocCursor() {}

void ApocCursor::EventOccured(Event *e)
{
	if (e->Type == EVENT_MOUSE_MOVE)
	{
		cursorPos.x = e->Data.Mouse.X;
		cursorPos.y = e->Data.Mouse.Y;
	}
}

void ApocCursor::Render()
{
	fw.renderer->draw(images.at(static_cast<int>(CurrentType)),
	                  Vec2<float>{cursorPos.x, cursorPos.y});
}
}; // namespace OpenApoc
