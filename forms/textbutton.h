
#pragma once

#include "control.h"
#include "../game/resources/ifont.h"
#include "../game/apocresources/rawsound.h"

class TextButton : public Control
{

	private:
		std::string text;
		IFont* font;

		static RawSound* buttonclick;
		static ALLEGRO_BITMAP* buttonbackground;

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		TextButton(Control* Owner, std::string Text, IFont* Font);

		virtual void EventOccured(Event* e);
		virtual void Render();
		virtual void Update();
};

