#include "form.h"

Form::Form(tinyxml2::XMLDocument FormConfiguration) : Control( nullptr )
{
	// TODO: Load form
}

Form::~Form()
{
	// Delete controls
	while (Controls.size() > 0)
	{
		Control* c = Controls.back();
		Controls.pop_back();
		delete c;
	}
}

void Form::EventOccured(Event* e)
{

}

void Form::Render()
{
	al_draw_filled_rectangle(this->Location.X, this->Location.Y, this->Location.X + this->Size.X, this->Location.Y + this->Size.Y, al_map_rgb(128, 128, 128));
}

void Form::Update()
{
}