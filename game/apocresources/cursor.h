
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Palette;
class Event;
class Framework;
class Image;

class Cursor
{

	private:
		std::vector<std::shared_ptr<Image> > images;
		Framework &fw;
		Vec2<int> cursorPos;

	public:
		enum CursorType
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

		Cursor( Framework &fw, std::shared_ptr<Palette> ColourPalette );
		~Cursor();

		void EventOccured( Event* e );
		void Render();
};
}; //namespace OpenApoc
