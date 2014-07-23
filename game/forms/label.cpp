
#include "label.h"

Label::Label(Control* Owner, std::wstring Text, ApocalypseFont* Font) : Control(Owner), text(Text), font(Font)
{
}

Label::~Label()
{
	// Delete controls
	while (Controls.size() > 0)
	{
		Control* c = Controls.back();
		Controls.pop_back();
		delete c;
	}
}

void Label::EventOccured(Event* e)
{
	// No events for labels
}

void Label::Render()
{
	font->DrawString(Location.X, Location.Y, text, APOCFONT_ALIGN_CENTRE);
}

void Label::Update()
{
	// No "updates"
}
