
#pragma once

#include "control.h"
#include "../apocresources/apocfont.h"

class Label : public Control
{

	private:
		std::string text;
		ApocalypseFont* font;

	public:
		Label(Control* Owner, std::string Text, ApocalypseFont* Font);
		~Label();

		virtual void EventOccured(Event* e);
		virtual void Render();
		virtual void Update();
};

