#pragma once

#include "library/sp.h"
#include "library/vec.h"
#include <vector>

namespace OpenApoc
{

class Palette;
class Event;
class Image;

class ApocCursor
{

  private:
	std::vector<sp<Image>> images;
	Vec2<int> cursorPos;

  public:
	enum class CursorType
	{
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

	const Vec2<int> &getPosition() const { return cursorPos; }

	ApocCursor(sp<Palette> ColourPalette);
	~ApocCursor();

	void eventOccured(Event *e);
	void render();
};
}; // namespace OpenApoc
