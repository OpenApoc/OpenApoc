
#pragma once

#include "control.h"
#include "../game/apocresources/apocfont.h"
#include "../game/apocresources/rawsound.h"

class TextButton : public Control
{

	private:
		std::string text;
		ApocalypseFont* font;

		static RawSound* buttonclick;
		static ALLEGRO_BITMAP* buttonbackground;

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		TextButton(Control* Owner, std::string Text, ApocalypseFont* Font);

		virtual void EventOccured(Event* e);
		virtual void Render();
		virtual void Update();
};

