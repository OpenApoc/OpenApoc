#include "form.h"

Form::Form( tinyxml2::XMLDocument FormConfiguration ) : Control( nullptr )
{
	// TODO: Load form
}

void Form::EventOccured( Event* e, bool* WasHandled )
{
	Control::EventOccured( e, WasHandled );
}

void Form::Render()
{
	Control::Render();
}

void Form::Update()
{
	Control::Update();
}