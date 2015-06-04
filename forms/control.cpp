
#include "forms/control.h"
#include "framework/framework.h"
#include "forms/forms.h"
#include "game/resources/gamecore.h"

namespace OpenApoc {

Control::Control(Framework &fw, Control* Owner)
	: owningControl(Owner), focusedChild(nullptr), mouseInside(false), mouseDepressed(false), resolvedLocation(0,0), fw(fw), Name("Control"),Location(0,0), Size(0,0), BackgroundColour( 128, 80, 80 )
{
	if( Owner != nullptr )
	{
		Owner->Controls.push_back( this );
	}
}

Control::~Control()
{
	UnloadResources();
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
		resolvedLocation.x = Location.x;
		resolvedLocation.y = Location.y;
	} else {
		resolvedLocation.x = owningControl->resolvedLocation.x + Location.x;
		resolvedLocation.y = owningControl->resolvedLocation.y + Location.y;
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
			return;
		}
	}

	Event* newevent;

	if( e->Type == EVENT_MOUSE_MOVE )
	{
		if( e->Data.Mouse.X >= resolvedLocation.x && e->Data.Mouse.X < resolvedLocation.x + Size.x && e->Data.Mouse.Y >= resolvedLocation.y && e->Data.Mouse.Y < resolvedLocation.y + Size.y )
		{
			if( !mouseInside )
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseEnter;
				memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
				newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
				newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
				memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
				fw.PushEvent( newevent );
				mouseInside = true;
			}

			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag = FormEventType::MouseMove;
			memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
			newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
			newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
			memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			fw.PushEvent( newevent );

			e->Handled = true;
		} else {
			if( mouseInside )
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseLeave;
				memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
				newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
				newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
				memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
				fw.PushEvent( newevent );
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
			newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
			newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
			memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			fw.PushEvent( newevent );
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
			newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
			newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
			memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			fw.PushEvent( newevent );

			if( mouseDepressed )
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseClick;
				memcpy( (void*)&newevent->Data.Forms.MouseInfo, (void*)&e->Data.Mouse, sizeof( FRAMEWORK_MOUSE_EVENT ) );
				newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
				newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
				memset( (void*)&newevent->Data.Forms.KeyInfo, 0, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
				fw.PushEvent( newevent );
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
			fw.PushEvent( newevent );

			e->Handled = true;
		}
	}
	if( e->Type == EVENT_KEY_PRESS )
	{
		if( IsFocused() )
		{
			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag = FormEventType::KeyPress;
			memcpy( (void*)&newevent->Data.Forms.KeyInfo, (void*)&e->Data.Keyboard, sizeof( FRAMEWORK_KEYBOARD_EVENT ) );
			memset( (void*)&newevent->Data.Forms.MouseInfo, 0, sizeof( FRAMEWORK_MOUSE_EVENT ) );
			fw.PushEvent( newevent );

			e->Handled = true;
		}
	}
}

void Control::Render()
{
	if( Size.x == 0 || Size.y == 0 )
	{
		return;
	}

	if( controlArea == nullptr ||
		controlArea->size != Vec2<unsigned int>{Size.x, Size.y})
	{
		controlArea.reset(new Surface{Vec2<unsigned int>{Size.x, Size.y}});
	}
	{

		RendererSurfaceBinding b(*fw.renderer, controlArea);
		PreRender();
		OnRender();
		PostRender();
	}

	fw.renderer->draw(controlArea, Vec2<float>{Location.x, Location.y});
}

void Control::PreRender()
{
	fw.renderer->clear(BackgroundColour);
}

void Control::OnRender()
{
	// Nothing specifically for the base control
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
	UString nodename;
	UString specialpositionx = "";
	UString specialpositiony = "";
	tinyxml2::XMLElement* subnode;
	UString attribvalue;

	if( Element->Attribute("id") != nullptr && UString(Element->Attribute("id")) != "" )
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
			if( node->Attribute("a") != nullptr && UString(node->Attribute("a")) != "" )
			{
				this->BackgroundColour = Colour{ Strings::ToU8( node->Attribute("r") ), Strings::ToU8( node->Attribute("g") ), Strings::ToU8( node->Attribute("b") ), Strings::ToU8( node->Attribute("a") ) };
			} else {
				this->BackgroundColour = Colour{ Strings::ToU8( node->Attribute("r") ), Strings::ToU8( node->Attribute("g") ), Strings::ToU8( node->Attribute("b") ) };
			}
		}
		if( nodename == "position" )
		{
			if( Strings::IsNumeric( node->Attribute("x") ) )
			{
				Location.x = Strings::ToInteger( node->Attribute("x") );
			} else {
				specialpositionx = node->Attribute("x");
			}
			if( Strings::IsNumeric( node->Attribute("y") ) )
			{
				Location.y = Strings::ToInteger( node->Attribute("y") );
			} else {
				specialpositiony = node->Attribute("y");
			}
		}
		if( nodename == "size" )
		{
			Size.x = Strings::ToInteger( node->Attribute("width") );
			Size.y = Strings::ToInteger( node->Attribute("height") );
		}

		// Child controls
		if( nodename == "control" )
		{
			Control* c = new Control( fw, this );
			c->ConfigureFromXML( node );
		}
		if( nodename == "label" )
		{
			Label* l = new Label( fw, this, fw.gamecore->GetString( node->Attribute("text") ), fw.gamecore->GetFont( node->FirstChildElement("font")->GetText() ) );
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
			Graphic* g = new Graphic( fw, this, node->FirstChildElement("image")->GetText() );
			g->ConfigureFromXML( node );
		}
		if( nodename == "textbutton" )
		{
			TextButton* tb = new TextButton( fw, this,  fw.gamecore->GetString( node->Attribute("text") ), fw.gamecore->GetFont( node->FirstChildElement("font")->GetText() ) );
			tb->ConfigureFromXML( node );
		}
		if( nodename == "graphicbutton" )
		{
			GraphicButton* gb;
			UString gb_image = "";
			if( node->FirstChildElement("image")->GetText() != nullptr )
			{
				gb_image = node->FirstChildElement("image")->GetText();
			}
			UString gb_dep = node->FirstChildElement("imagedepressed")->GetText();
			if( node->FirstChildElement("imagedepressed")->GetText() != nullptr )
			{
				gb_dep = node->FirstChildElement("imagedepressed")->GetText();
			}
			if( node->FirstChildElement("imagehover") == nullptr )
			{
				gb = new GraphicButton( fw, this, gb_image, gb_dep );
			} else {
				gb = new GraphicButton( fw, this, gb_image, gb_dep, node->FirstChildElement("imagehover")->GetText() );
			}
			gb->ConfigureFromXML( node );
		}
		if( nodename == "checkbox" )
		{
			CheckBox* cb = new CheckBox( fw, this );
			cb->ConfigureFromXML( node );
		}
		if( nodename == "vscroll" )
		{
			VScrollBar* vsb = new VScrollBar( fw, this );
			vsb->ConfigureFromXML( node );

			subnode = node->FirstChildElement("grippercolour");
			if( subnode != nullptr )
			{
				if( subnode->Attribute("a") != nullptr && UString(subnode->Attribute("a")) != "" )
				{
					vsb->GripperColour = Colour( Strings::ToU8( subnode->Attribute("r") ), Strings::ToU8( subnode->Attribute("g") ), Strings::ToU8( subnode->Attribute("b") ), Strings::ToU8( subnode->Attribute("a") ) );
				} else {
					vsb->GripperColour = Colour( Strings::ToU8( subnode->Attribute("r") ), Strings::ToU8( subnode->Attribute("g") ), Strings::ToU8( subnode->Attribute("b") ) );
				}
			}
			subnode = node->FirstChildElement("range");
			if( subnode != nullptr )
			{
				if( subnode->Attribute("min") != nullptr && UString(subnode->Attribute("min")) != "" )
				{
					vsb->Minimum = Strings::ToInteger( subnode->Attribute("min") );
				}
				if( subnode->Attribute("max") != nullptr && UString(subnode->Attribute("max")) != "" )
				{
					vsb->Maximum = Strings::ToInteger( subnode->Attribute("max") );
				}
			}
		}

		if( nodename == "hscroll" )
		{
			HScrollBar* hsb = new HScrollBar( fw, this );
			hsb->ConfigureFromXML( node );

			subnode = node->FirstChildElement("grippercolour");
			if( subnode != nullptr )
			{
				if( subnode->Attribute("a") != nullptr && UString(subnode->Attribute("a")) != "" )
				{
					hsb->GripperColour = Colour( Strings::ToU8( subnode->Attribute("r") ), Strings::ToU8( subnode->Attribute("g") ), Strings::ToU8( subnode->Attribute("b") ), Strings::ToU8( subnode->Attribute("a") ) );
				} else {
					hsb->GripperColour = Colour( Strings::ToU8( subnode->Attribute("r") ), Strings::ToU8( subnode->Attribute("g") ), Strings::ToU8( subnode->Attribute("b") ) );
				}
			}
			subnode = node->FirstChildElement("range");
			if( subnode != nullptr )
			{
				if( subnode->Attribute("min") != nullptr && UString(subnode->Attribute("min")) != "" )
				{
					hsb->Minimum = Strings::ToInteger( subnode->Attribute("min") );
				}
				if( subnode->Attribute("max") != nullptr && UString(subnode->Attribute("max")) != "" )
				{
					hsb->Maximum = Strings::ToInteger( subnode->Attribute("max") );
				}
			}
		}

		if( nodename == "listbox" )
		{
			VScrollBar* vsb = nullptr;

			if( node->Attribute("scrollbarid") != nullptr && UString(node->Attribute("scrollbarid")) != "" )
			{
				attribvalue = node->Attribute("scrollbarid");
				vsb = (VScrollBar*)this->FindControl( attribvalue );
			}
			ListBox* lb = new ListBox( fw, this, vsb );
			lb->ConfigureFromXML( node );
		}

		if( nodename == "textedit" )
		{
			TextEdit* te = new TextEdit( fw, this, "", fw.gamecore->GetFont( node->FirstChildElement("font")->GetText() ) );
			te->ConfigureFromXML( node );
			subnode = node->FirstChildElement("alignment");
			if( subnode != nullptr )
			{
				if( subnode->Attribute("horizontal") != nullptr )
				{
					attribvalue = subnode->Attribute("horizontal");
					if( attribvalue == "left" )
					{
						te->TextHAlign = HorizontalAlignment::Left;
					}
					if( attribvalue == "centre" )
					{
						te->TextHAlign = HorizontalAlignment::Centre;
					}
					if( attribvalue == "right" )
					{
						te->TextHAlign = HorizontalAlignment::Right;
					}
				}
				if( subnode->Attribute("vertical") != nullptr )
				{
					attribvalue = subnode->Attribute("vertical");
					if( attribvalue == "top" )
					{
						te->TextVAlign = VerticalAlignment::Top;
					}
					if( attribvalue == "centre" )
					{
						te->TextVAlign = VerticalAlignment::Centre;
					}
					if( attribvalue == "bottom" )
					{
						te->TextVAlign = VerticalAlignment::Bottom;
					}
				}
			}
		}

	}

	if( specialpositionx != "" )
	{
		if( specialpositionx == "left" )
		{
			Location.x = 0;
		}
		if( specialpositionx == "centre" )
		{
			if( owningControl == nullptr )
			{
				Location.x = (fw.Display_GetWidth() / 2) - (Size.x / 2);
			} else {
				Location.x = (owningControl->Size.x / 2) - (Size.x / 2);
			}
		}
		if( specialpositionx == "right" )
		{
			if( owningControl == nullptr )
			{
				Location.x = fw.Display_GetWidth() - Size.x;
			} else {
				Location.x = owningControl->Size.x - Size.x;
			}
		}
	}

	if( specialpositiony != "" )
	{
		if( specialpositiony == "top" )
		{
			Location.y = 0;
		}
		if( specialpositiony == "centre" )
		{
			if( owningControl == nullptr )
			{
				Location.y = (fw.Display_GetHeight() / 2) - (Size.y / 2);
			} else {
				Location.y = (owningControl->Size.y / 2) - (Size.y / 2);
			}
		}
		if( specialpositiony == "bottom" )
		{
			if( owningControl == nullptr )
			{
				Location.y = fw.Display_GetHeight() - Size.y;
			} else {
				Location.y = owningControl->Size.y - Size.y;
			}
		}
	}
}

void Control::UnloadResources()
{
}

Control* Control::operator[]( int Index )
{
	return Controls.at( Index );
}

Control* Control::FindControl( UString ID )
{
	for( auto c = Controls.begin(); c != Controls.end(); c++ )
	{
		Control* ctrl = (Control*)*c;
		if( ctrl->Name == ID )
		{
			return ctrl;
		}
	}
	return nullptr;
}

Control* Control::GetParent()
{
	return owningControl;
}

Control* Control::GetRootControl()
{
	if( owningControl == nullptr )
	{
		return this;
	} else {
		return owningControl->GetRootControl();
	}
}

Form* Control::GetForm()
{
	Control* c = GetRootControl();
	if( dynamic_cast<Form*>( c ) != nullptr )
	{
		return (Form*)c;
	}
	return nullptr;
}

}; //namespace OpenApoc
