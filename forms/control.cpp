
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

void Control::ResolveLocation()
{
	if( owningControl == nullptr )
	{
		resolvedLocation.X = Location.X;
		resolvedLocation.Y = Location.Y;
	} else {
		resolvedLocation.X = owningControl->resolvedLocation.X + Location.X;
		resolvedLocation.Y = owningControl->resolvedLocation.Y + Location.Y;
	}

	for( auto ctrlidx = Controls.rbegin(); ctrlidx != Controls.rend(); ctrlidx++ )
	{
		Control* c = (Control*)*ctrlidx;
		c->ResolveLocation();
	}
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

	Event* newevent;

	if( e->Type == EVENT_MOUSE_MOVE )
	{
		if( e->Data.Mouse.X >= resolvedLocation.X && e->Data.Mouse.X < resolvedLocation.X + Size.X && e->Data.Mouse.Y >= resolvedLocation.Y && e->Data.Mouse.Y < resolvedLocation.Y + Size.Y )
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
		al_draw_filled_rectangle( resolvedLocation.X, resolvedLocation.Y, resolvedLocation.X + Size.X, resolvedLocation.Y + Size.Y, BackgroundColour );
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
	std::string specialpositionx = "";
	std::string specialpositiony = "";
	tinyxml2::XMLElement* subnode;
	std::string attribvalue;

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
				specialpositionx = node->Attribute("x");
			}
			if( Strings::IsNumeric( node->Attribute("y") ) )
			{
				Location.Y = Strings::ToInteger( node->Attribute("y") );
			} else {
				specialpositiony = node->Attribute("y");
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
			subnode = node->FirstChildElement("alignment");
			if( subnode != nullptr )
			{
				if( subnode->Attribute("horizontal") != nullptr )
				{
					attribvalue = subnode->Attribute("horizontal");
					if( attribvalue == "left" )
					{
						l->TextHAlign = HorizontalAlignment::Left;
					}
					if( attribvalue == "centre" )
					{
						l->TextHAlign = HorizontalAlignment::Centre;
					}
					if( attribvalue == "right" )
					{
						l->TextHAlign = HorizontalAlignment::Right;
					}
				}
				if( subnode->Attribute("vertical") != nullptr )
				{
					attribvalue = subnode->Attribute("vertical");
					if( attribvalue == "top" )
					{
						l->TextVAlign = VerticalAlignment::Top;
					}
					if( attribvalue == "centre" )
					{
						l->TextVAlign = VerticalAlignment::Centre;
					}
					if( attribvalue == "bottom" )
					{
						l->TextVAlign = VerticalAlignment::Bottom;
					}
				}
			}
		}
		if( nodename == "graphic" )
		{
			Graphic* g = new Graphic( this, node->FirstChildElement("image")->GetText() );
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
			std::string gb_image = "";
			if( node->FirstChildElement("image")->GetText() != nullptr )
			{
				gb_image = node->FirstChildElement("image")->GetText();
			}
			std::string gb_dep = node->FirstChildElement("image_depressed")->GetText();
			if( node->FirstChildElement("image_depressed")->GetText() != nullptr )
			{
				gb_dep = node->FirstChildElement("image_depressed")->GetText();
			}
			if( node->FirstChildElement("image_hover") == nullptr )
			{
				gb = new GraphicButton( this, gb_image, gb_dep );
			} else {
				gb = new GraphicButton( this, gb_image, gb_dep, node->FirstChildElement("image_hover")->GetText() );
			}
			gb->ConfigureFromXML( node );
		}
		if( nodename == "checkbox" )
		{
			CheckBox* cb = new CheckBox( this );
			cb->ConfigureFromXML( node );
		}
	}

	if( specialpositionx != "" )
	{
		if( specialpositionx == "left" )
		{
			Location.X = 0;
		}
		if( specialpositionx == "centre" )
		{
			if( owningControl == nullptr )
			{
				Location.X = (FRAMEWORK->Display_GetWidth() / 2) - (Size.X / 2);
			} else {
				Location.X = (owningControl->Size.X / 2) - (Size.X / 2);
			}
		}
		if( specialpositionx == "right" )
		{
			if( owningControl == nullptr )
			{
				Location.X = FRAMEWORK->Display_GetWidth() - Size.X;
			} else {
				Location.X = owningControl->Size.X - Size.X;
			}
		}
	}

	if( specialpositiony != "" )
	{
		if( specialpositiony == "top" )
		{
			Location.Y = 0;
		}
		if( specialpositiony == "centre" )
		{
			if( owningControl == nullptr )
			{
				Location.Y = (FRAMEWORK->Display_GetHeight() / 2) - (Size.Y / 2);
			} else {
				Location.Y = (owningControl->Size.Y / 2) - (Size.Y / 2);
			}
		}
		if( specialpositiony == "bottom" )
		{
			if( owningControl == nullptr )
			{
				Location.Y = FRAMEWORK->Display_GetHeight() - Size.Y;
			} else {
				Location.Y = owningControl->Size.Y - Size.Y;
			}
		}
	}
}

void Control::UnloadResources()
{
}