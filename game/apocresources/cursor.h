
#pragma once

#include "../../framework/includes.h"
#include "../../framework/event.h"
#include "palette.h"

namespace OpenApoc {
class Cursor
{

	private:
		std::vector<ALLEGRO_BITMAP*> images;
		int cursorx;
		int cursory;

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

		Cursor( Palette* ColourPalette );
		~Cursor();

		void EventOccured( Event* e );
		void Render();
};
}; //namespace OpenApoc
