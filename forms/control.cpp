
#include "control.h"

Control::Control(Control* Owner) : owningControl(Owner), BackgroundColour( al_map_rgb( 128, 128, 128 ) )
{
	
}

Control::~Control()
{
	// Delete controls
	while( Controls.size() > 0 )
	{
		Control* c = Controls.back();
		Controls.pop_back();
		delete c;
	}
}

Vector2* Control::GetResolvedLocation()
{
	Vector2* v = new Vector2( Location.X, Location.Y );
	if( owningControl != nullptr )
	{
		Vector2* tv = owningControl->GetResolvedLocation();
		v->Add( tv );
		delete tv;
	}
	return v;
}

void Control::EventOccured( Event* e, bool* WasHandled )
{
	for( auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++ )
	{
		Control* c = (Control*)*ctrlidx;
		c->EventOccured( e, WasHandled );
	}
}

void Control::Render()
{
	PreRender();
	PostRender();
}

void Control::PreRender()
{
	if( BackgroundColour.a != 0.0f )
	{
		Vector2* v = GetResolvedLocation();
		al_draw_filled_rectangle( v->X, v->Y, v->X + Size.X, v->Y + Size.Y, BackgroundColour );
		delete v;
	}
}

void Control::PostRender()
{
	for( auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++ )
	{
		Control* c = (Control*)*ctrlidx;
		c->Render();
	}
}

void Control::Update()
{
	for( auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++ )
	{
		Control* c = (Control*)*ctrlidx;
		c->Update();
	}
}