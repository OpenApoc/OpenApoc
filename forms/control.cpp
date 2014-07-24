
#include "control.h"
#include "../framework/framework.h"

Control::Control(Control* Owner) : owningControl(Owner), focusedChild(nullptr), BackgroundColour(al_map_rgb( 128, 80, 80 )), mouseInside(false), mouseDepressed(false)
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

void Control::SetFocus(Control* Child)
{
	focusedChild = Child;
}

Control* Control::GetActiveControl()
{
	return focusedChild;
}

void Control::Focus()
{
	if( owningControl != nullptr )
	{
		owningControl->SetFocus( this );
	}
}

bool Control::IsFocused()
{
	if( owningControl != nullptr )
	{
		return (owningControl->GetActiveControl() == this);
	}
	return true;
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
	for( auto ctrlidx = Controls.rbegin(); ctrlidx != Controls.rend(); ctrlidx++ )
	{
		Control* c = (Control*)*ctrlidx;
		c->EventOccured( e, WasHandled );
		if( WasHandled )
		{
			break;
		}
	}

	if( *WasHandled )
	{
			return;
	}

	Vector2* v = GetResolvedLocation();
	Event* newevent;

	if( e->Type == EVENT_MOUSE_MOVE )
	{
		if( e->Data.Mouse.X >= v->X && e->Data.Mouse.X < v->X + Size.X && e->Data.Mouse.Y >= v->Y && e->Data.Mouse.Y < v->Y + Size.Y )
		{
			if( !mouseInside )
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseEnter;
				memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
				memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
				newevent->Data.Forms.AdditionalData = nullptr;
				FRAMEWORK->PushEvent( newevent );
				mouseInside = true;
			}

			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag = FormEventType::MouseMove;
			memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
			memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			newevent->Data.Forms.AdditionalData = nullptr;
			FRAMEWORK->PushEvent( newevent );
		} else {
			if( mouseInside )
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseLeave;
				memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
				memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
				newevent->Data.Forms.AdditionalData = nullptr;
				FRAMEWORK->PushEvent( newevent );
				mouseInside = false;
			}
		}
	}

	if( e->Type == EVENT_MOUSE_DOWN )
	{
		if( mouseInside )
		{
			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag = FormEventType::MouseDown;
			memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
			memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			newevent->Data.Forms.AdditionalData = nullptr;
			FRAMEWORK->PushEvent( newevent );
			mouseDepressed = true;
		}
	}

	if( e->Type == EVENT_MOUSE_UP )
	{
		if( mouseInside )
		{
			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag = FormEventType::MouseUp;
			memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
			memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			newevent->Data.Forms.AdditionalData = nullptr;
			FRAMEWORK->PushEvent( newevent );

			if( mouseDepressed )
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseClick;
				memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
				memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
				newevent->Data.Forms.AdditionalData = nullptr;
				FRAMEWORK->PushEvent( newevent );
			}
		} else {
			mouseDepressed = false;
		}
	}

	delete v;

	if( e->Type == EVENT_KEY_DOWN || e->Type == EVENT_KEY_UP )
	{
		if( IsFocused() )
		{
			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag = (e->Type == EVENT_KEY_DOWN ? FormEventType::KeyDown : FormEventType::KeyUp);
			memcpy( (void*)&newevent->Data.Forms.KeyInfo, (void*)&e->Data.Keyboard, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			memset( (void*)&newevent->Data.Forms.MouseInfo, 0, sizeof( FRAMEWORK_MOUSE_EVENT ) );
			newevent->Data.Forms.AdditionalData = nullptr;
			FRAMEWORK->PushEvent( newevent );
		}
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