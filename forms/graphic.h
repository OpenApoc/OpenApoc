
#pragma once

#include "control.h"

class Graphic : public Control
{

	private:
		ALLEGRO_BITMAP* image;

	public:
		Graphic( Control* Owner, ALLEGRO_BITMAP* Image );

		virtual void EventOccured( Event* e, bool* WasHandled );
		virtual void Render();
		virtual void Update();
};

