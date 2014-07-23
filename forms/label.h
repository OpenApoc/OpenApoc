
#pragma once

#include "control.h"
#include "../game/apocresources/apocfont.h"



class Label : public Control
{

	private:
		std::string text;
		ApocalypseFont* font;

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		Label(Control* Owner, std::string Text, ApocalypseFont* Font);

		virtual void EventOccured(Event* e, bool* WasHandled);
		virtual void Render();
		virtual void Update();
};

