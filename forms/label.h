
#pragma once

#include "control.h"
#include "../game/resources/ifont.h"

class Label : public Control
{

	private:
		std::string text;
		IFont* font;

	public:
		HorizontalAlignment TextHAlign;
		VerticalAlignment TextVAlign;

		Label(Control* Owner, std::string Text, IFont* Font);

		virtual void EventOccured(Event* e);
		virtual void Render();
		virtual void Update();
};

