#include "library/sp.h"

#include "forms/control.h"
#include "framework/framework.h"
#include "forms/forms.h"

namespace OpenApoc
{

Control::Control(Control *Owner, bool takesFocus)
    : owningControl(Owner), focusedChild(nullptr), mouseInside(false), mouseDepressed(false),
      resolvedLocation(0, 0), Name("Control"), Location(0, 0), Size(0, 0),
      BackgroundColour(0, 0, 0, 0), takesFocus(takesFocus), showBounds(false), Visible(true),
      canCopy(true), lastCopiedTo(nullptr), data(nullptr)
{
	if (Owner != nullptr)
	{
		Owner->Controls.push_back(this);
	}
}

Control::~Control()
{
	UnloadResources();
	// Delete controls
	while (Controls.size() > 0)
	{
		Control *c = Controls.back();
		Controls.pop_back();
		delete c;
	}
}

void Control::SetFocus(Control *Child) { focusedChild = Child; }

Control *Control::GetActiveControl() const { return focusedChild; }

void Control::Focus()
{
	if (owningControl != nullptr)
	{
		owningControl->SetFocus(this);
	}
}

bool Control::IsFocused() const
{
	if (!this->takesFocus)
		return false;
	if (owningControl != nullptr)
	{
		return (owningControl->GetActiveControl() == this);
	}
	return true;
}

void Control::ResolveLocation()
{
	if (owningControl == nullptr)
	{
		resolvedLocation.x = Location.x;
		resolvedLocation.y = Location.y;
	}
	else
	{
		if (Location.x > owningControl->Size.x || Location.y > owningControl->Size.y)
		{
			resolvedLocation.x = -99999;
			resolvedLocation.y = -99999;
		}
		else
		{
			resolvedLocation.x = owningControl->resolvedLocation.x + Location.x;
			resolvedLocation.y = owningControl->resolvedLocation.y + Location.y;
		}
	}

	for (auto ctrlidx = Controls.rbegin(); ctrlidx != Controls.rend(); ctrlidx++)
	{
		Control *c = *ctrlidx;
		c->ResolveLocation();
	}
}

void Control::EventOccured(Event *e)
{
	for (auto ctrlidx = Controls.rbegin(); ctrlidx != Controls.rend(); ctrlidx++)
	{
		Control *c = *ctrlidx;
		if (c->Visible)
		{
			c->EventOccured(e);
			if (e->Handled)
			{
				return;
			}
		}
	}

	Event *newevent;

	if (e->Type == EVENT_MOUSE_MOVE || e->Type == EVENT_MOUSE_DOWN || e->Type == EVENT_MOUSE_UP)
	{
		bool newInside = (e->Data.Mouse.X >= resolvedLocation.x &&
		                  e->Data.Mouse.X < resolvedLocation.x + Size.x &&
		                  e->Data.Mouse.Y >= resolvedLocation.y &&
		                  e->Data.Mouse.Y < resolvedLocation.y + Size.y);

		if (e->Type == EVENT_MOUSE_MOVE)
		{
			if (newInside)
			{
				if (!mouseInside)
				{
					newevent = new Event();
					newevent->Type = EVENT_FORM_INTERACTION;
					newevent->Data.Forms.RaisedBy = this;
					newevent->Data.Forms.EventFlag = FormEventType::MouseEnter;
					newevent->Data.Forms.MouseInfo = e->Data.Mouse;
					newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
					newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
					fw().PushEvent(newevent);
				}

				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseMove;
				newevent->Data.Forms.MouseInfo = e->Data.Mouse;
				newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
				newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
				fw().PushEvent(newevent);

				e->Handled = true;
			}
			else
			{
				if (mouseInside)
				{
					newevent = new Event();
					newevent->Type = EVENT_FORM_INTERACTION;
					newevent->Data.Forms.RaisedBy = this;
					newevent->Data.Forms.EventFlag = FormEventType::MouseLeave;
					newevent->Data.Forms.MouseInfo = e->Data.Mouse;
					newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
					newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
					fw().PushEvent(newevent);
				}
			}
		}

		if (e->Type == EVENT_MOUSE_DOWN)
		{
			if (newInside)
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseDown;
				newevent->Data.Forms.MouseInfo = e->Data.Mouse;
				newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
				newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
				fw().PushEvent(newevent);
				mouseDepressed = true;
			}
		}

		if (e->Type == EVENT_MOUSE_UP)
		{
			if (newInside)
			{
				newevent = new Event();
				newevent->Type = EVENT_FORM_INTERACTION;
				newevent->Data.Forms.RaisedBy = this;
				newevent->Data.Forms.EventFlag = FormEventType::MouseUp;
				newevent->Data.Forms.MouseInfo = e->Data.Mouse;
				newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
				newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
				fw().PushEvent(newevent);

				if (mouseDepressed)
				{
					newevent = new Event();
					newevent->Type = EVENT_FORM_INTERACTION;
					newevent->Data.Forms.RaisedBy = this;
					newevent->Data.Forms.EventFlag = FormEventType::MouseClick;
					newevent->Data.Forms.MouseInfo = e->Data.Mouse;
					newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
					newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
					fw().PushEvent(newevent);
				}
			}
			mouseDepressed = false;
		}
		mouseInside = newInside;
	}

	if (e->Type == EVENT_FINGER_DOWN || e->Type == EVENT_FINGER_UP || e->Type == EVENT_FINGER_MOVE)
	{
		// This right now is a carbon copy of mouse event-handling. Maybe we should do something
		// else?
		// FIXME: Use something other than mouseInside? fingerInside maybe?
		bool newInside = (e->Data.Finger.X >= resolvedLocation.x &&
		                  e->Data.Finger.X < resolvedLocation.x + Size.x &&
		                  e->Data.Finger.Y >= resolvedLocation.y &&
		                  e->Data.Finger.Y < resolvedLocation.y + Size.y);

		// FIXME: Right now we'll just copy touch data to the mouse location. That's... not exactly
		// right.
		// FIXME: We're only doing event processing for the "primary" finger.
		if (e->Data.Finger.IsPrimary || 1)
		{
			FRAMEWORK_MOUSE_EVENT FakeMouseData;
			FakeMouseData.X = e->Data.Finger.X;
			FakeMouseData.Y = e->Data.Finger.Y;
			FakeMouseData.DeltaX = e->Data.Finger.DeltaX;
			FakeMouseData.DeltaY = e->Data.Finger.DeltaY;
			FakeMouseData.WheelHorizontal = 0;
			FakeMouseData.WheelVertical = 0;
			FakeMouseData.Button = 1; // Always left mouse button?

			if (e->Type == EVENT_FINGER_MOVE)
			{
				if (newInside)
				{
					if (!mouseInside)
					{
						newevent = new Event();
						newevent->Type = EVENT_FORM_INTERACTION;
						newevent->Data.Forms.RaisedBy = this;
						newevent->Data.Forms.EventFlag = FormEventType::MouseEnter;
						// newevent->Data.Forms.MouseInfo = e->Data.Mouse;
						newevent->Data.Forms.MouseInfo = FakeMouseData;
						newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
						newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
						fw().PushEvent(newevent);
					}

					newevent = new Event();
					newevent->Type = EVENT_FORM_INTERACTION;
					newevent->Data.Forms.RaisedBy = this;
					newevent->Data.Forms.EventFlag = FormEventType::MouseMove;
					// newevent->Data.Forms.MouseInfo = e->Data.Mouse;
					newevent->Data.Forms.MouseInfo = FakeMouseData;
					newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
					newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
					fw().PushEvent(newevent);

					e->Handled = true;
				}
				else
				{
					if (mouseInside)
					{
						newevent = new Event();
						newevent->Type = EVENT_FORM_INTERACTION;
						newevent->Data.Forms.RaisedBy = this;
						newevent->Data.Forms.EventFlag = FormEventType::MouseLeave;
						// newevent->Data.Forms.MouseInfo = e->Data.Mouse;
						newevent->Data.Forms.MouseInfo = FakeMouseData;
						newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
						newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
						fw().PushEvent(newevent);
					}
				}
			}

			if (e->Type == EVENT_FINGER_DOWN)
			{
				if (newInside)
				{
					newevent = new Event();
					newevent->Type = EVENT_FORM_INTERACTION;
					newevent->Data.Forms.RaisedBy = this;
					newevent->Data.Forms.EventFlag = FormEventType::MouseDown;
					// newevent->Data.Forms.MouseInfo = e->Data.Mouse;
					newevent->Data.Forms.MouseInfo = FakeMouseData;
					newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
					newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
					fw().PushEvent(newevent);
					mouseDepressed = true;

					e->Handled = true;
				}
			}

			if (e->Type == EVENT_FINGER_UP)
			{
				if (newInside)
				{
					newevent = new Event();
					newevent->Type = EVENT_FORM_INTERACTION;
					newevent->Data.Forms.RaisedBy = this;
					newevent->Data.Forms.EventFlag = FormEventType::MouseUp;
					// newevent->Data.Forms.MouseInfo = e->Data.Mouse;
					newevent->Data.Forms.MouseInfo = FakeMouseData;
					newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
					newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
					fw().PushEvent(newevent);

					if (mouseDepressed)
					{
						newevent = new Event();
						newevent->Type = EVENT_FORM_INTERACTION;
						newevent->Data.Forms.RaisedBy = this;
						newevent->Data.Forms.EventFlag = FormEventType::MouseClick;
						// newevent->Data.Forms.MouseInfo = e->Data.Mouse;
						newevent->Data.Forms.MouseInfo = FakeMouseData;
						newevent->Data.Forms.MouseInfo.X -= resolvedLocation.x;
						newevent->Data.Forms.MouseInfo.Y -= resolvedLocation.y;
						fw().PushEvent(newevent);
					}
				}
				// FIXME: This will result in collisions with real mouse events.
				mouseDepressed = false;
			}
		}
		mouseInside = newInside;
	}

	if (e->Type == EVENT_KEY_DOWN || e->Type == EVENT_KEY_UP)
	{
		if (IsFocused())
		{
			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag =
			    (e->Type == EVENT_KEY_DOWN ? FormEventType::KeyDown : FormEventType::KeyUp);
			newevent->Data.Forms.KeyInfo = e->Data.Keyboard;
			fw().PushEvent(newevent);

			e->Handled = true;
		}
	}
	if (e->Type == EVENT_KEY_PRESS)
	{
		if (IsFocused())
		{
			newevent = new Event();
			newevent->Type = EVENT_FORM_INTERACTION;
			newevent->Data.Forms.RaisedBy = this;
			newevent->Data.Forms.EventFlag = FormEventType::KeyPress;
			newevent->Data.Forms.KeyInfo = e->Data.Keyboard;
			fw().PushEvent(newevent);

			e->Handled = true;
		}
	}
}

void Control::Render()
{
	if (!Visible || Size.x == 0 || Size.y == 0)
	{
		return;
	}

	if (controlArea == nullptr || controlArea->size != Vec2<unsigned int>(Size))
	{
		controlArea.reset(new Surface{Vec2<unsigned int>(Size)});
	}
	{
		sp<Palette> previousPalette;
		if (this->palette)
		{
			previousPalette = fw().renderer->getPalette();
			fw().renderer->setPalette(this->palette);
		}

		RendererSurfaceBinding b(*fw().renderer, controlArea);
		PreRender();
		OnRender();
		PostRender();
		if (this->palette)
		{
			fw().renderer->setPalette(previousPalette);
		}
	}

	fw().renderer->draw(controlArea, Location);
}

void Control::PreRender() { fw().renderer->clear(BackgroundColour); }

void Control::OnRender()
{
	// Nothing specifically for the base control
}

void Control::PostRender()
{
	for (auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++)
	{
		Control *c = *ctrlidx;
		if (c->Visible)
		{
			c->Render();
		}
	}
	if (showBounds)
	{
		fw().renderer->drawRect({0, 0}, Size, Colour{255, 0, 0, 255});
	}
}

void Control::Update()
{
	for (auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++)
	{
		Control *c = *ctrlidx;
		c->Update();
	}
}

void Control::ConfigureFromXML(tinyxml2::XMLElement *Element)
{
	UString nodename;
	UString specialpositionx = "";
	UString specialpositiony = "";
	UString attribvalue;
	std::map<UString, RadioButton **> radiogroups;

	if (Element->Attribute("id") != nullptr && UString(Element->Attribute("id")) != "")
	{
		nodename = Element->Attribute("id");
		this->Name = nodename;
	}

	if (Element->Attribute("visible") != nullptr && UString(Element->Attribute("visible")) != "")
	{
		UString vistxt = Element->Attribute("visible");
		vistxt = vistxt.substr(0, 1).toUpper();
		this->Visible = (vistxt == "Y" || vistxt == "T");
	}

	tinyxml2::XMLElement *node;
	for (node = Element->FirstChildElement(); node != nullptr; node = node->NextSiblingElement())
	{
		nodename = node->Name();

		if (nodename == "palette")
		{
			auto pal = fw().data->load_palette(node->GetText());
			if (!pal)
			{
				LogError("Control referenced palette \"%s\" that cannot be loaded",
				         node->GetText());
			}
			this->palette = pal;
		}

		else if (nodename == "backcolour")
		{
			if (node->Attribute("a") != nullptr && UString(node->Attribute("a")) != "")
			{
				this->BackgroundColour = Colour{
				    Strings::ToU8(node->Attribute("r")), Strings::ToU8(node->Attribute("g")),
				    Strings::ToU8(node->Attribute("b")), Strings::ToU8(node->Attribute("a"))};
			}
			else
			{
				this->BackgroundColour =
				    Colour{Strings::ToU8(node->Attribute("r")), Strings::ToU8(node->Attribute("g")),
				           Strings::ToU8(node->Attribute("b"))};
			}
		}
		else if (nodename == "position")
		{
			if (Strings::IsInteger(node->Attribute("x")))
			{
				Location.x = Strings::ToInteger(node->Attribute("x"));
			}
			else
			{
				specialpositionx = node->Attribute("x");
			}
			if (Strings::IsInteger(node->Attribute("y")))
			{
				Location.y = Strings::ToInteger(node->Attribute("y"));
			}
			else
			{
				specialpositiony = node->Attribute("y");
			}
		}
		else if (nodename == "size")
		{
			Size.x = Strings::ToInteger(node->Attribute("width"));
			Size.y = Strings::ToInteger(node->Attribute("height"));
		}

		// Child controls
		else if (nodename == "control")
		{
			auto c = new Control(this);
			c->ConfigureFromXML(node);
		}
		else if (nodename == "label")
		{
			Label *l = new Label(this);
			l->ConfigureFromXML(node);
		}
		else if (nodename == "graphic")
		{
			Graphic *g = new Graphic(this);
			g->ConfigureFromXML(node);
		}
		else if (nodename == "textbutton")
		{
			TextButton *tb = new TextButton(this);
			tb->ConfigureFromXML(node);
		}
		else if (nodename == "graphicbutton")
		{
			GraphicButton *gb = gb = new GraphicButton(this);
			gb->ConfigureFromXML(node);
			if (node->Attribute("scrollprev") != nullptr &&
			    UString(node->Attribute("scrollprev")) != "")
			{
				attribvalue = node->Attribute("scrollprev");
				gb->ScrollBarPrev = this->FindControlTyped<ScrollBar>(attribvalue);
			}
			if (node->Attribute("scrollnext") != nullptr &&
			    UString(node->Attribute("scrollnext")) != "")
			{
				attribvalue = node->Attribute("scrollnext");
				gb->ScrollBarNext = this->FindControlTyped<ScrollBar>(attribvalue);
			}
		}
		else if (nodename == "checkbox")
		{
			auto cb = new CheckBox(this);
			cb->ConfigureFromXML(node);
		}
		else if (nodename == "radiobutton")
		{
			RadioButton **group = nullptr;
			if (node->Attribute("groupid") != nullptr && UString(node->Attribute("groupid")) != "")
			{
				attribvalue = node->Attribute("groupid");
				if (radiogroups.find(attribvalue) == radiogroups.end())
				{
					radiogroups[attribvalue] = new RadioButton *(nullptr);
				}
				group = radiogroups[attribvalue];
			}
			else
			{
				LogError("Radiobutton \"%s\" has no group", node->Attribute("id"));
			}
			auto rb = new RadioButton(this, group);
			rb->ConfigureFromXML(node);
		}
		else if (nodename == "scroll")
		{
			auto sb = new ScrollBar(this);
			sb->ConfigureFromXML(node);
		}

		else if (nodename == "listbox")
		{
			ScrollBar *sb = nullptr;

			if (node->Attribute("scrollbarid") != nullptr &&
			    UString(node->Attribute("scrollbarid")) != "")
			{
				attribvalue = node->Attribute("scrollbarid");
				sb = this->FindControlTyped<ScrollBar>(attribvalue);
			}
			auto lb = new ListBox(this, sb);
			lb->ConfigureFromXML(node);
		}

		else if (nodename == "textedit")
		{
			TextEdit *te = new TextEdit(this);
			te->ConfigureFromXML(node);
		}
	}

	if (specialpositionx != "")
	{
		if (specialpositionx == "left")
		{
			Location.x = 0;
		}
		else if (specialpositionx == "centre")
		{
			if (owningControl == nullptr)
			{
				Location.x = (fw().Display_GetWidth() / 2) - (Size.x / 2);
			}
			else
			{
				Location.x = (owningControl->Size.x / 2) - (Size.x / 2);
			}
		}
		else if (specialpositionx == "right")
		{
			if (owningControl == nullptr)
			{
				Location.x = fw().Display_GetWidth() - Size.x;
			}
			else
			{
				Location.x = owningControl->Size.x - Size.x;
			}
		}
	}

	if (specialpositiony != "")
	{
		if (specialpositiony == "top")
		{
			Location.y = 0;
		}
		else if (specialpositiony == "centre")
		{
			if (owningControl == nullptr)
			{
				Location.y = (fw().Display_GetHeight() / 2) - (Size.y / 2);
			}
			else
			{
				Location.y = (owningControl->Size.y / 2) - (Size.y / 2);
			}
		}
		else if (specialpositiony == "bottom")
		{
			if (owningControl == nullptr)
			{
				Location.y = fw().Display_GetHeight() - Size.y;
			}
			else
			{
				Location.y = owningControl->Size.y - Size.y;
			}
		}
	}
	LogInfo("Control \"%s\" has %d subcontrols (%d, %d, %d, %d)", this->Name.c_str(),
	        Controls.size(), Location.x, Location.y, Size.x, Size.y);
}

void Control::UnloadResources() {}

Control *Control::operator[](int Index) const { return Controls.at(Index); }

Control *Control::FindControl(UString ID) const
{
	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		Control *ctrl = *c;
		if (ctrl->Name == ID)
		{
			return ctrl;
		}
		Control *childControl = ctrl->FindControl(ID);
		if (childControl)
			return childControl;
	}
	return nullptr;
}

Control *Control::GetParent() const { return owningControl; }

Control *Control::GetRootControl()
{
	if (owningControl == nullptr)
	{
		return this;
	}
	else
	{
		return owningControl->GetRootControl();
	}
}

Form *Control::GetForm()
{
	Control *c = GetRootControl();
	return dynamic_cast<Form *>(c);
}

std::list<UString> Control::WordWrapText(sp<OpenApoc::BitmapFont> Font, UString WrapText) const
{
	int txtwidth;
	std::list<UString> lines;

	txtwidth = Font->GetFontWidth(WrapText);

	if (txtwidth > Size.x)
	{
		UString textleft = WrapText;
		unsigned int estlinelength = Font->GetEstimateCharacters(Size.x);

		while (textleft.length() > 0)
		{
			if (textleft.length() > estlinelength)
			{
				int charidx;
				for (charidx = estlinelength - 1; charidx > 1; charidx--)
				{
					// TODO: Need to implement a list of line break characters
					if (textleft.substr(charidx, 1) == UString(" "))
					{
						lines.push_back(textleft.substr(0, charidx));
						textleft = textleft.substr(charidx + 1, textleft.length() - charidx);
						break;
					}
				}
				if (charidx == 1)
				{
					LogWarning(
					    "No break in line \"%s\" found - this will probably overflow the control",
					    textleft.c_str());
					break;
				}
			}
			else
			{
				lines.push_back(textleft);
				textleft = "";
			}
		}
	}
	else
	{
		lines.push_back(WrapText);
	}

	return lines;
}

void Control::SetParent(Control *Parent) { owningControl = Parent; }

Vec2<int> Control::GetLocationOnScreen() const
{
	Vec2<int> r(resolvedLocation.x, resolvedLocation.y);
	return r;
}

Control *Control::CopyTo(Control *CopyParent)
{
	Control *copy = new Control(CopyParent, takesFocus);
	CopyControlData(copy);
	return copy;
}

void Control::CopyControlData(Control *CopyOf)
{
	lastCopiedTo = CopyOf;

	CopyOf->Name = this->Name;
	CopyOf->Location = this->Location;
	CopyOf->Size = this->Size;
	CopyOf->BackgroundColour = this->BackgroundColour;
	CopyOf->takesFocus = this->takesFocus;
	CopyOf->showBounds = this->showBounds;
	CopyOf->Visible = this->Visible;

	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		Control *ctrl = *c;
		if (ctrl->canCopy)
		{
			ctrl->CopyTo(CopyOf);
		}
	}
}

bool Control::eventIsWithin(const Event *e) const
{
	if (e->Type == EVENT_MOUSE_MOVE || e->Type == EVENT_MOUSE_DOWN || e->Type == EVENT_MOUSE_UP)
	{
		return (e->Data.Mouse.X >= resolvedLocation.x &&
		        e->Data.Mouse.X < resolvedLocation.x + Size.x &&
		        e->Data.Mouse.Y >= resolvedLocation.y &&
		        e->Data.Mouse.Y < resolvedLocation.y + Size.y);
	}
	else if (e->Type == EVENT_FINGER_DOWN || e->Type == EVENT_FINGER_UP ||
	         e->Type == EVENT_FINGER_MOVE)
	{
		return (e->Data.Finger.X >= resolvedLocation.x &&
		        e->Data.Finger.X < resolvedLocation.x + Size.x &&
		        e->Data.Finger.Y >= resolvedLocation.y &&
		        e->Data.Finger.Y < resolvedLocation.y + Size.y);
	}
	// Only mouse & finger events have a location to be within
	return false;
}
}; // namespace OpenApoc
