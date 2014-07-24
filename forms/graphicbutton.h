
#pragma once

#include "control.h"
#include "../game/apocresources/rawsound.h"

class GraphicButton : public Control
{

	private:
		ALLEGRO_BITMAP* image;
		ALLEGRO_BITMAP* imagedepressed;
		ALLEGRO_BITMAP* imagehover;

		static RawSound* buttonclick;

	public:
		GraphicButton( Control* Owner, ALLEGRO_BITMAP* Image, ALLEGRO_BITMAP* ImageDepressed );
		GraphicButton( Control* Owner, ALLEGRO_BITMAP* Image, ALLEGRO_BITMAP* ImageDepressed, ALLEGRO_BITMAP* ImageHover );
		~GraphicButton();

		virtual void EventOccured( Event* e );
		virtual void Render();
		virtual void Update();
};

