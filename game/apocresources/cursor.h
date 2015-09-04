
#pragma once

#include "framework/includes.h"

namespace OpenApoc
{

class Palette;
class Event;
class Framework;
class Image;

class ApocCursor
{

  private:
	std::vector<std::shared_ptr<Image>> images;
	Framework &fw;
	Vec2<int> cursorPos;

  public:
	enum CursorType {
		Normal,
		ThrowTarget,
		PsiTarget,
		NoTarget,
		Add,
		Shoot,
		Control,
		Teleport,
		NoTeleport
	};

	CursorType CurrentType;

	ApocCursor(Framework &fw, std::shared_ptr<Palette> ColourPalette);
	~ApocCursor();

	void EventOccured(Event *e);
	void Render();
};
} // namespace OpenApoc
