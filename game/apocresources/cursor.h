
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Palette;
class Event;
class Framework;

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

		Cursor( Framework &fw, std::shared_ptr<Palette> ColourPalette );
		~Cursor();

		void EventOccured( Event* e );
		void Render();
};
}; //namespace OpenApoc
