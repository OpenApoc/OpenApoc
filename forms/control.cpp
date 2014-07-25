
#include "control.h"
#include "../framework/framework.h"
#include "forms.h"
#include "../game/resources/gamecore.h"

Control::Control(Control* Owner) : Name("Control"), owningControl(Owner), focusedChild(nullptr), BackgroundColour(al_map_rgb( 128, 80, 80 )), mouseInside(false), mouseDepressed(false)
{
	if( Owner != nullptr )
	{
		Owner->Controls.push_back( this );
	}
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

void Control::EventOccured( Event* e )
{
	for( auto ctrlidx = Controls.rbegin(); ctrlidx != Controls.rend(); ctrlidx++ )
	{
		Control* c = (Control*)*ctrlidx;
		c->EventOccured( e );
		if( e->Handled )
		{
			break;
		}
	}

	if( e->Handled )
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

			e->Handled = true;
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

			e->Handled = true;
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

			e->Handled = true;
		}
		mouseDepressed = false;
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

			e->Handled = true;
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

void Control::ConfigureFromXML( tinyxml2::XMLElement* Element )
{
	std::string nodename;

	if( Element->Attribute("id") != nullptr && Element->Attribute("id") != "" )
	{
		nodename = Element->Attribute("id");
		this->Name = nodename;
	}

	tinyxml2::XMLElement* node;
	for( node = Element->FirstChildElement(); node != nullptr; node = node->NextSiblingElement() )
	{
		nodename = node->Name();

		if( nodename == "backcolour" )
		{
			if( node->Attribute("a") != nullptr && node->Attribute("a") != "" )
			{
				this->BackgroundColour = al_map_rgba( Strings::ToInteger( node->Attribute("r") ), Strings::ToInteger( node->Attribute("g") ), Strings::ToInteger( node->Attribute("b") ), Strings::ToInteger( node->Attribute("a") ) );
			} else {
				this->BackgroundColour = al_map_rgb( Strings::ToInteger( node->Attribute("r") ), Strings::ToInteger( node->Attribute("g") ), Strings::ToInteger( node->Attribute("b") ) );
			}
		}
		if( nodename == "position" )
		{
			if( Strings::IsNumeric( node->Attribute("x") ) )
			{
				Location.X = Strings::ToInteger( node->Attribute("x") );
			} else {
				// TODO: Parse centre/left/right (needs size first)
			}
			if( Strings::IsNumeric( node->Attribute("y") ) )
			{
				Location.Y = Strings::ToInteger( node->Attribute("y") );
			} else {
				// TODO: Parse centre/top/bottom (needs size first)
			}
		}
		if( nodename == "size" )
		{
			Size.X = Strings::ToInteger( node->Attribute("width") );
			Size.Y = Strings::ToInteger( node->Attribute("height") );
		}

		// Child controls
		if( nodename == "control" )
		{
			Control* c = new Control( this );
			c->ConfigureFromXML( node );
		}
		if( nodename == "label" )
		{
			Label* l = new Label( this, GAMECORE->GetString( node->Attribute("text") ), GAMECORE->GetFont( node->FirstChildElement("font")->GetText() ) );
			l->ConfigureFromXML( node );
		}
		if( nodename == "graphic" )
		{
			Graphic* g = new Graphic( this, GAMECORE->GetImage( node->FirstChildElement("image")->GetText() ) );
			g->ConfigureFromXML( node );
		}
		if( nodename == "textbutton" )
		{
			TextButton* tb = new TextButton( this,  GAMECORE->GetString( node->Attribute("text") ), GAMECORE->GetFont( node->FirstChildElement("font")->GetText() ) );
			tb->ConfigureFromXML( node );
		}
		if( nodename == "graphicbutton" )
		{
			GraphicButton* gb;
			if( node->FirstChildElement("image_hover") == nullptr )
			{
				gb = new GraphicButton( this, GAMECORE->GetImage( node->FirstChildElement("image")->GetText() ), GAMECORE->GetImage( node->FirstChildElement("image_depressed")->GetText() ) );
			} else {
				gb = new GraphicButton( this, GAMECORE->GetImage( node->FirstChildElement("image")->GetText() ), GAMECORE->GetImage( node->FirstChildElement("image_depressed")->GetText() ), GAMECORE->GetImage( node->FirstChildElement("image_hover")->GetText() ) );
			}
			gb->ConfigureFromXML( node );
		}
	}
}
