#include "forms/control.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/forms.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/font.h"
#include "framework/framework.h"
#include "framework/image.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "framework/trace.h"
#include "library/sp.h"

namespace OpenApoc
{

Control::Control(bool takesFocus)
    : mouseInside(false), mouseDepressed(false), resolvedLocation(0, 0), Visible(true),
      Name("Control"), Location(0, 0), Size(0, 0), SelectionSize(0, 0),
      BackgroundColour(0, 0, 0, 0), takesFocus(takesFocus), showBounds(false), Enabled(true),
      canCopy(true), funcPreRender(nullptr)
{
}

Control::~Control() { unloadResources(); }

bool Control::isFocused() const
{
	// FIXME: Need to think of a method of 'sharing' focus across multiple forms/non-form active
	// areas
	return false;
}

void Control::resolveLocation()
{
	auto previousLocation = resolvedLocation;
	auto parentControl = this->getParent();
	if (parentControl == nullptr)
	{
		resolvedLocation.x = Location.x;
		resolvedLocation.y = Location.y;
	}
	else
	{
		if (Location.x > parentControl->Size.x || Location.y > parentControl->Size.y)
		{
			resolvedLocation.x = -99999;
			resolvedLocation.y = -99999;
		}
		else
		{
			resolvedLocation.x = parentControl->resolvedLocation.x + Location.x;
			resolvedLocation.y = parentControl->resolvedLocation.y + Location.y;
		}
	}

	for (auto ctrlidx = Controls.rbegin(); ctrlidx != Controls.rend(); ctrlidx++)
	{
		auto c = *ctrlidx;
		c->resolveLocation();
	}

	if (previousLocation != resolvedLocation)
	{
		this->setDirty();
	}
}

bool Control::isPointInsideControlBounds(int x, int y) const
{
	const Vec2<int> &Size =
	    (SelectionSize.x == 0 || SelectionSize.y == 0) ? this->Size : SelectionSize;

	return x >= resolvedLocation.x && x < resolvedLocation.x + Size.x && y >= resolvedLocation.y &&
	       y < resolvedLocation.y + Size.y;
}

bool Control::isPointInsideControlBounds(Event *e, sp<Control> c) const
{
	if (!e || !c)
	{
		return false;
	}

	const Vec2<int> &Size =
	    (c->SelectionSize.x == 0 || c->SelectionSize.y == 0) ? c->Size : c->SelectionSize;
	int eventX = e->forms().MouseInfo.X + e->forms().RaisedBy->resolvedLocation.x;
	int eventY = e->forms().MouseInfo.Y + e->forms().RaisedBy->resolvedLocation.y;

	return eventX >= c->resolvedLocation.x && eventX < c->resolvedLocation.x + Size.x &&
	       eventY >= c->resolvedLocation.y && eventY < c->resolvedLocation.y + Size.y;
}

void Control::eventOccured(Event *e)
{
	for (auto ctrlidx = Controls.rbegin(); ctrlidx != Controls.rend(); ctrlidx++)
	{
		auto c = *ctrlidx;
		if (c->Visible && c->Enabled)
		{
			c->eventOccured(e);
			if (e->Handled)
			{
				return;
			}
		}
	}

	if (e->type() == EVENT_MOUSE_MOVE || e->type() == EVENT_MOUSE_DOWN ||
	    e->type() == EVENT_MOUSE_UP)
	{
		bool newInside = isPointInsideControlBounds(e->mouse().X, e->mouse().Y);
		// (e->mouse().X >= resolvedLocation.x && e->mouse().X < resolvedLocation.x + Size.x &&
		// e->mouse().Y >= resolvedLocation.y && e->mouse().Y < resolvedLocation.y + Size.y);

		if (e->type() == EVENT_MOUSE_MOVE)
		{
			if (newInside)
			{
				if (!mouseInside)
				{
					this->pushFormEvent(FormEventType::MouseEnter, e);
				}
				this->pushFormEvent(FormEventType::MouseMove, e);
				e->Handled = true;
			}
			else
			{
				if (mouseInside)
				{
					this->pushFormEvent(FormEventType::MouseLeave, e);
					mouseDepressed = false;
				}
			}
		}

		if (e->type() == EVENT_MOUSE_DOWN)
		{
			if (newInside)
			{
				this->pushFormEvent(FormEventType::MouseDown, e);
				mouseDepressed = true;
			}
		}

		if (e->type() == EVENT_MOUSE_UP)
		{
			if (newInside)
			{
				this->pushFormEvent(FormEventType::MouseUp, e);

				if (mouseDepressed)
				{
					this->pushFormEvent(FormEventType::MouseClick, e);
				}
			}
			mouseDepressed = false;
		}
		mouseInside = newInside;
	}

	if (e->type() == EVENT_FINGER_DOWN || e->type() == EVENT_FINGER_UP ||
	    e->type() == EVENT_FINGER_MOVE)
	{
		// This right now is a carbon copy of mouse event-handling. Maybe we should do something
		// else?
		// FIXME: Use something other than mouseInside? fingerInside maybe?
		bool newInside = isPointInsideControlBounds(e->mouse().X, e->mouse().Y);
		// (e->finger().X >= resolvedLocation.x && e->finger().X < resolvedLocation.x + Size.x &&
		//  e->finger().Y >= resolvedLocation.y && e->finger().Y < resolvedLocation.y + Size.y);

		// FIXME: Right now we'll just copy touch data to the mouse location. That's... not exactly
		// right.
		// FIXME: We're only doing event processing for the "primary" finger.
		if (e->finger().IsPrimary || 1)
		{
			FrameworkMouseEvent FakeMouseData;
			FakeMouseData.X = e->finger().X;
			FakeMouseData.Y = e->finger().Y;
			FakeMouseData.DeltaX = e->finger().DeltaX;
			FakeMouseData.DeltaY = e->finger().DeltaY;
			FakeMouseData.WheelHorizontal = 0;
			FakeMouseData.WheelVertical = 0;
			FakeMouseData.Button = 1; // Always left mouse button?

			if (e->type() == EVENT_FINGER_MOVE)
			{
				MouseEvent fakeMouseEvent(EVENT_MOUSE_MOVE);
				fakeMouseEvent.mouse() = FakeMouseData;
				if (newInside)
				{
					if (!mouseInside)
					{
						this->pushFormEvent(FormEventType::MouseEnter, &fakeMouseEvent);
					}

					this->pushFormEvent(FormEventType::MouseMove, &fakeMouseEvent);

					e->Handled = true;
				}
				else
				{
					if (mouseInside)
					{
						this->pushFormEvent(FormEventType::MouseLeave, &fakeMouseEvent);
					}
				}
			}

			if (e->type() == EVENT_FINGER_DOWN)
			{
				MouseEvent fakeMouseEvent(EVENT_MOUSE_DOWN);
				fakeMouseEvent.mouse() = FakeMouseData;
				if (newInside)
				{
					this->pushFormEvent(FormEventType::MouseDown, &fakeMouseEvent);
					mouseDepressed = true;

					// e->Handled = true;
				}
			}

			if (e->type() == EVENT_FINGER_UP)
			{
				MouseEvent fakeMouseEvent(EVENT_MOUSE_UP);
				fakeMouseEvent.mouse() = FakeMouseData;
				if (newInside)
				{
					this->pushFormEvent(FormEventType::MouseUp, &fakeMouseEvent);

					if (mouseDepressed)
					{
						this->pushFormEvent(FormEventType::MouseClick, &fakeMouseEvent);
					}
				}
				// FIXME: This will result in collisions with real mouse events.
				mouseDepressed = false;
			}
		}
		mouseInside = newInside;
	}

	if (e->type() == EVENT_KEY_DOWN || e->type() == EVENT_KEY_UP)
	{
		if (isFocused())
		{
			this->pushFormEvent(
			    e->type() == EVENT_KEY_DOWN ? FormEventType::KeyDown : FormEventType::KeyUp, e);

			e->Handled = true;
		}
	}
	if (e->type() == EVENT_KEY_PRESS)
	{
		if (isFocused())
		{
			this->pushFormEvent(FormEventType::KeyPress, e);

			e->Handled = true;
		}
	}

	if (e->type() == EVENT_TEXT_INPUT)
	{
		if (isFocused())
		{
			this->pushFormEvent(FormEventType::TextInput, e);

			e->Handled = true;
		}
	}
}

void Control::render()
{
	TRACE_FN_ARGS1("Name", this->Name);

	if (!Visible || Size.x == 0 || Size.y == 0)
	{
		return;
	}

	if (controlArea == nullptr || controlArea->size != Vec2<unsigned int>(Size))
	{
		this->dirty = true;
		controlArea.reset(new Surface{Vec2<unsigned int>(Size)});
	}
	if (this->dirty)
	{
		sp<Palette> previousPalette;
		if (this->palette)
		{
			previousPalette = fw().renderer->getPalette();
			fw().renderer->setPalette(this->palette);
		}

		RendererSurfaceBinding b(*fw().renderer, controlArea);
		onRender();
		postRender();
		if (this->palette)
		{
			fw().renderer->setPalette(previousPalette);
		}
	}

	this->dirty = false;

	if (Enabled)
	{
		fw().renderer->draw(controlArea, Location);
	}
	else
	{
		fw().renderer->drawTinted(controlArea, Location, Colour(255, 255, 255, 128));
	}
}

/**
 * Used if controls require computations before rendering.
 * Any operations other than graphical.
 */
void Control::preRender()
{
	for (auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++)
	{
		auto c = *ctrlidx;
		if (c->Visible)
		{
			c->preRender();
		}
	}

	if (funcPreRender)
	{
		funcPreRender(shared_from_this());
	}
}

void Control::onRender() { fw().renderer->clear(BackgroundColour); }

void Control::postRender()
{
	for (auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++)
	{
		auto c = *ctrlidx;
		if (c->Visible)
		{
			c->render();
		}
	}
	if (showBounds)
	{
		fw().renderer->drawRect({0, 0}, Size, Colour{255, 0, 0, 255});
	}
}

void Control::update()
{
	for (auto ctrlidx = Controls.begin(); ctrlidx != Controls.end(); ctrlidx++)
	{
		auto c = *ctrlidx;
		c->update();
	}
}

void Control::configureFromXml(pugi::xml_node *node)
{
	configureSelfFromXml(node);
	configureChildrenFromXml(node);
}

void Control::configureChildrenFromXml(pugi::xml_node *parent)
{
	UString nodename;
	UString attribvalue;
	for (auto node = parent->first_child(); node; node = node.next_sibling())
	{
		nodename = node.name();

		// Child controls
		if (nodename == "control")
		{
			auto c = this->createChild<Control>();
			c->configureFromXml(&node);
		}
		else if (nodename == "label")
		{
			auto l = this->createChild<Label>();
			l->configureFromXml(&node);
		}
		else if (nodename == "graphic")
		{
			auto g = this->createChild<Graphic>();
			g->configureFromXml(&node);
		}
		else if (nodename == "textbutton")
		{
			auto tb = this->createChild<TextButton>();
			tb->configureFromXml(&node);
		}
		else if (nodename == "graphicbutton")
		{
			auto gb = this->createChild<GraphicButton>();
			gb->configureFromXml(&node);
			UString scrollPrev = node.attribute("scrollprev").as_string();
			if (!scrollPrev.empty())
			{
				gb->ScrollBarPrev = this->findControlTyped<ScrollBar>(scrollPrev);
			}
			UString scrollNext = node.attribute("scrollnext").as_string();
			if (!scrollNext.empty())
			{
				gb->ScrollBarNext = this->findControlTyped<ScrollBar>(scrollNext);
			}
			bool scrollLarge = node.attribute("scrolllarge").as_bool();
			if (scrollLarge)
			{
				gb->scrollLarge = true;
			}
		}
		else if (nodename == "checkbox")
		{
			auto cb = this->createChild<CheckBox>();
			cb->configureFromXml(&node);
		}
		else if (nodename == "tristatebox")
		{
			auto cb = this->createChild<TriStateBox>();
			cb->configureFromXml(&node);
		}
		else if (nodename == "radiobutton")
		{
			sp<RadioButtonGroup> group = nullptr;
			UString groupID = node.attribute("groupid").as_string();
			if (!groupID.empty())
			{
				if (radiogroups.find(groupID) == radiogroups.end())
				{
					radiogroups[groupID] = mksp<RadioButtonGroup>(groupID);
				}
				group = radiogroups[groupID];
			}
			else
			{
				LogError("Radiobutton \"%s\" has no group", node.attribute("id").as_string());
			}
			auto rb = this->createChild<RadioButton>(group);
			rb->configureFromXml(&node);
			if (group)
			{
				group->radioButtons.push_back(rb);
			}
		}
		else if (nodename == "scroll")
		{
			auto sb = this->createChild<ScrollBar>();
			sb->configureFromXml(&node);
			UString yAttr = node.attribute("largepercent").as_string();
			if (Strings::isInteger(yAttr))
			{
				sb->LargePercent = node.attribute("largepercent").as_int();
			}
		}
		else if (nodename == "listbox")
		{
			sp<ScrollBar> sb = nullptr;
			UString scrollBarID = node.attribute("scrollbarid").as_string();

			if (!scrollBarID.empty())
			{
				sb = this->findControlTyped<ScrollBar>(scrollBarID);
			}
			auto lb = this->createChild<ListBox>(sb);
			lb->configureFromXml(&node);
		}
		else if (nodename == "multilistbox")
		{
			sp<ScrollBar> sb = nullptr;
			UString scrollBarID = node.attribute("scrollbarid").as_string();

			if (!scrollBarID.empty())
			{
				sb = this->findControlTyped<ScrollBar>(scrollBarID);
			}
			auto lb = this->createChild<MultilistBox>(sb);
			lb->configureFromXml(&node);
		}
		else if (nodename == "textedit")
		{
			auto te = this->createChild<TextEdit>();
			te->configureFromXml(&node);
		}
		else if (nodename == "ticker")
		{
			auto tk = this->createChild<Ticker>();
			tk->configureFromXml(&node);
		}
		else if (nodename == "subform")
		{
			auto f = ui().getForm(node.attribute("src").as_string());
			if (f)
			{
				f->setParent(shared_from_this());
				f->configureSelfFromXml(&node);
			}
		}
	}
}

void Control::configureSelfFromXml(pugi::xml_node *node)
{
	UString specialpositionx = "";
	UString specialpositiony = "";

	if (node->attribute("id"))
	{
		this->Name = node->attribute("id").as_string();
	}
	if (node->attribute("visible"))
	{
		this->Visible = node->attribute("visible").as_bool();
	}
	if (node->attribute("border"))
	{
		this->showBounds = node->attribute("border").as_bool();
	}

	auto parentControl = this->getParent();
	for (auto child = node->first_child(); child; child = child.next_sibling())
	{
		UString childName = child.name();

		if (childName == "palette")
		{
			auto pal = fw().data->loadPalette(child.text().get());
			if (!pal)
			{
				LogError("Control referenced palette \"%s\" that cannot be loaded",
				         child.text().get());
			}
			this->palette = pal;
		}

		else if (childName == "backcolour")
		{
			uint8_t r = child.attribute("r").as_uint();
			uint8_t g = child.attribute("g").as_uint();
			uint8_t b = child.attribute("b").as_uint();
			uint8_t a = child.attribute("a").as_uint(255);
			this->BackgroundColour = Colour{r, g, b, a};
		}
		else if (childName == "position")
		{
			UString xAttr = child.attribute("x").as_string();
			if (Strings::isInteger(xAttr))
			{
				Location.x = child.attribute("x").as_int();
			}
			else
			{
				specialpositionx = xAttr;
			}
			UString yAttr = child.attribute("y").as_string();
			if (Strings::isInteger(yAttr))
			{
				Location.y = child.attribute("y").as_int();
			}
			else
			{
				specialpositiony = yAttr;
			}
		}
		else if (childName == "size")
		{
			UString specialsizex = "";
			UString specialsizey = "";

			UString widthAttr = child.attribute("width").as_string();
			// if size ends with % this means that it is special (percentage) size
			if (Strings::isInteger(widthAttr) && !widthAttr.endsWith("%"))
			{
				Size.x = child.attribute("width").as_int();
			}
			else
			{
				specialsizex = widthAttr;
			}
			UString heightAttr = child.attribute("height").as_string();
			// if size ends with % this means that it is special (percentage) size
			if (Strings::isInteger(heightAttr) && !heightAttr.endsWith("%"))
			{
				Size.y = child.attribute("height").as_int();
			}
			else
			{
				specialsizey = heightAttr;
			}

			if (specialsizex != "")
			{
				if (specialsizex.str().back() == '%')
				{
					// Skip % sign at the end
					auto sizeRatio = Strings::toFloat(specialsizex) / 100.0f;
					setRelativeWidth(sizeRatio);
				}
				else
				{
					LogWarning("Control \"%s\" has not supported size x value \"%s\"", this->Name,
					           specialsizex);
				}
			}

			if (specialsizey != "")
			{
				if (specialsizey.str().back() == '%')
				{
					auto sizeRatio = Strings::toFloat(specialsizey) / 100.0f;
					setRelativeHeight(sizeRatio);
				}
				else if (specialsizey == "item")
				{
					// get size y for parent control
					sp<Control> parent = parentControl;
					sp<ListBox> listBox;
					while (parent != nullptr &&
					       !(listBox = std::dynamic_pointer_cast<ListBox>(parent)))
					{
						parent = parent->getParent();
					}
					if (listBox != nullptr)
					{
						Size.y = listBox->ItemSize;
					}
					else
					{
						LogWarning(
						    "Control \"%s\" with \"item\" size.y does not have ListBox parent ",
						    this->Name);
					}
				}
				else
				{
					LogWarning("Control \"%s\" has not supported size y value \"%s\"", this->Name,
					           specialsizey);
				}
			}
		}
	}

	if (specialpositionx != "")
	{
		if (specialpositionx == "left")
		{
			align(HorizontalAlignment::Left);
		}
		else if (specialpositionx == "centre")
		{
			align(HorizontalAlignment::Centre);
		}
		else if (specialpositionx == "right")
		{
			align(HorizontalAlignment::Right);
		}
	}

	if (specialpositiony != "")
	{
		if (specialpositiony == "top")
		{
			align(VerticalAlignment::Top);
		}
		else if (specialpositiony == "centre")
		{
			align(VerticalAlignment::Centre);
		}
		else if (specialpositiony == "bottom")
		{
			align(VerticalAlignment::Bottom);
		}
	}

	LogInfo("Control \"%s\" has %zu subcontrols (%d, %d, %d, %d)", this->Name, Controls.size(),
	        Location.x, Location.y, Size.x, Size.y);
}

void Control::unloadResources() {}

sp<Control> Control::operator[](int Index) const { return Controls.at(Index); }

sp<Control> Control::findControl(UString ID) const
{
	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		auto ctrl = *c;
		if (ctrl->Name == ID)
		{
			return ctrl;
		}
		auto childControl = ctrl->findControl(ID);
		if (childControl)
			return childControl;
	}
	return nullptr;
}

bool Control::replaceChildByName(sp<Control> ctrl)
{
	for (int i = 0; i < Controls.size(); i++)
	{
		if (Controls[i]->Name == ctrl->Name)
		{
			Controls[i] = ctrl;
			ctrl->owningControl = shared_from_this();
			setDirty();
			return true;
		}
	}

	return false;
}

sp<Control> Control::getParent() const { return owningControl.lock(); }

sp<Control> Control::getRootControl()
{
	auto parent = owningControl.lock();
	if (parent == nullptr)
	{
		return shared_from_this();
	}
	else
	{
		return parent->getRootControl();
	}
}

sp<Form> Control::getForm()
{
	auto c = getRootControl();
	return std::dynamic_pointer_cast<Form>(c);
}

void Control::setParent(sp<Control> Parent, int position)
{
	if (Parent)
	{
		auto previousParent = this->owningControl.lock();
		if (previousParent)
		{
			LogError("Reparenting control");
		}
		if (position == -1)
		{
			Parent->Controls.push_back(shared_from_this());
		}
		else
		{
			Parent->Controls.insert(Parent->Controls.begin() + position, shared_from_this());
		}
		Parent->setDirty();
	}
	owningControl = Parent;
}

sp<Control> Control::getAncestor(sp<Control> Parent)
{
	sp<Control> ancestor = shared_from_this();
	while (ancestor != nullptr)
	{
		if (ancestor->getParent() == Parent)
		{
			break;
		}
		else
		{
			ancestor = ancestor->getParent();
		}
	}
	return ancestor;
}

Vec2<int> Control::getLocationOnScreen() const
{
	Vec2<int> r(resolvedLocation.x, resolvedLocation.y);
	return r;
}

void Control::setRelativeWidth(float widthFactor)
{
	if (widthFactor == 0)
	{
		Size.x = 0;
	}
	else
	{
		Vec2<int> parentSize = getParentSize();
		Size.x = (int)(parentSize.x * widthFactor);
	}
}

void Control::setRelativeHeight(float heightFactor)
{
	if (heightFactor == 0)
	{
		Size.y = 0;
	}
	else
	{
		Vec2<int> parentSize = getParentSize();
		Size.y = (int)(parentSize.y * heightFactor);
	}
}

Vec2<int> Control::getParentSize() const
{
	auto parent = getParent();
	if (parent != nullptr)
	{
		return parent->Size;
	}
	else
	{
		return Vec2<int>{fw().displayGetWidth(), fw().displayGetHeight()};
	}
}

int Control::align(HorizontalAlignment HAlign, int ParentWidth, int ChildWidth)
{
	int x = 0;
	switch (HAlign)
	{
		case HorizontalAlignment::Left:
			x = 0;
			break;
		case HorizontalAlignment::Centre:
			x = (ParentWidth / 2) - (ChildWidth / 2);
			break;
		case HorizontalAlignment::Right:
			x = ParentWidth - ChildWidth;
			break;
	}
	return x;
}

int Control::align(VerticalAlignment VAlign, int ParentHeight, int ChildHeight)
{
	int y = 0;
	switch (VAlign)
	{
		case VerticalAlignment::Top:
			y = 0;
			break;
		case VerticalAlignment::Centre:
			y = (ParentHeight / 2) - (ChildHeight / 2);
			break;
		case VerticalAlignment::Bottom:
			y = ParentHeight - ChildHeight;
			break;
	}
	return y;
}

void Control::align(HorizontalAlignment HAlign)
{
	Location.x = align(HAlign, getParentSize().x, Size.x);
}

void Control::align(VerticalAlignment VAlign)
{
	Location.y = align(VAlign, getParentSize().y, Size.y);
}

void Control::align(HorizontalAlignment HAlign, VerticalAlignment VAlign)
{
	align(HAlign);
	align(VAlign);
}

sp<Control> Control::copyTo(sp<Control> CopyParent)
{
	sp<Control> copy;
	if (CopyParent)
	{
		copy = CopyParent->createChild<Control>(takesFocus);
	}
	else
	{
		copy = mksp<Control>(takesFocus);
	}
	copyControlData(copy);
	return copy;
}

void Control::copyControlData(sp<Control> CopyOf)
{
	lastCopiedTo = CopyOf;

	CopyOf->Name = this->Name;
	CopyOf->Location = this->Location;
	CopyOf->Size = this->Size;
	CopyOf->SelectionSize = this->SelectionSize;
	CopyOf->BackgroundColour = this->BackgroundColour;
	CopyOf->takesFocus = this->takesFocus;
	CopyOf->showBounds = this->showBounds;
	CopyOf->Visible = this->Visible;

	for (auto &c : Controls)
	{
		if (c->canCopy)
		{
			c->copyTo(CopyOf);
		}
	}
}

bool Control::eventIsWithin(const Event *e) const
{
	if (e->type() == EVENT_MOUSE_MOVE || e->type() == EVENT_MOUSE_DOWN ||
	    e->type() == EVENT_MOUSE_UP)
	{
		return (e->mouse().X >= resolvedLocation.x && e->mouse().X < resolvedLocation.x + Size.x &&
		        e->mouse().Y >= resolvedLocation.y && e->mouse().Y < resolvedLocation.y + Size.y);
	}
	else if (e->type() == EVENT_FINGER_DOWN || e->type() == EVENT_FINGER_UP ||
	         e->type() == EVENT_FINGER_MOVE)
	{
		return (e->finger().X >= resolvedLocation.x &&
		        e->finger().X < resolvedLocation.x + Size.x &&
		        e->finger().Y >= resolvedLocation.y && e->finger().Y < resolvedLocation.y + Size.y);
	}
	// Only mouse & finger events have a location to be within
	return false;
}

void Control::pushFormEvent(FormEventType type, Event *parentEvent)
{
	FormsEvent *event = nullptr;
	switch (type)
	{
		// Mouse events fall-through
		case FormEventType::MouseEnter:
		case FormEventType::MouseLeave:
			// Only mouseEnter/mouseLeave could cause a state change (e.g. stop highlighting a
			// button)
			this->setDirty();
		// Fall-though
		case FormEventType::MouseMove:
		{
			event = new FormsEvent();
			event->forms().RaisedBy = shared_from_this();
			event->forms().EventFlag = type;
			event->forms().MouseInfo = parentEvent->mouse();
			event->forms().MouseInfo.X -= resolvedLocation.x;
			event->forms().MouseInfo.Y -= resolvedLocation.y;
			fw().pushEvent(event);
			break;
		}
		case FormEventType::MouseDown:
		case FormEventType::MouseUp:
		case FormEventType::MouseClick:
		{
			event = new FormsEvent();
			event->forms().RaisedBy = shared_from_this();
			event->forms().EventFlag = type;
			event->forms().MouseInfo = parentEvent->mouse();
			event->forms().MouseInfo.X -= resolvedLocation.x;
			event->forms().MouseInfo.Y -= resolvedLocation.y;
			fw().pushEvent(event);
			this->setDirty();
			break;
		}
		// Keyboard events fall-through
		case FormEventType::KeyDown:
		case FormEventType::KeyUp:
		case FormEventType::KeyPress:
		{
			event = new FormsEvent();
			event->forms().RaisedBy = shared_from_this();
			event->forms().EventFlag = type;
			event->forms().KeyInfo = parentEvent->keyboard();
			fw().pushEvent(event);
			this->setDirty();
			break;
		}
		// Input event special
		case FormEventType::TextInput:
		{
			event = new FormsEvent();
			event->forms().RaisedBy = shared_from_this();
			event->forms().EventFlag = type;
			event->forms().Input = parentEvent->text();
			fw().pushEvent(event);
			this->setDirty();
			break;
		}
		// Forms events fall-through
		case FormEventType::ButtonClick:
		case FormEventType::CheckBoxChange:
		case FormEventType::ListBoxChangeHover:
		case FormEventType::ListBoxChangeSelected:
		case FormEventType::ScrollBarChange:
		case FormEventType::TextChanged:
		case FormEventType::CheckBoxSelected:
		case FormEventType::CheckBoxDeSelected:
		case FormEventType::TriStateBoxChange:
		case FormEventType::TriStateBoxState1Selected:
		case FormEventType::TriStateBoxState2Selected:
		case FormEventType::TriStateBoxState3Selected:
		case FormEventType::TextEditCancel:
		case FormEventType::TextEditFinish:
		{
			event = new FormsEvent();
			if (parentEvent)
			{
				event->forms() = parentEvent->forms();
			}
			event->forms().RaisedBy = shared_from_this();
			event->forms().EventFlag = type;
			fw().pushEvent(event);
			this->setDirty();
			break;
		}
		default:
			LogError("Unexpected event type %d", (int)type);
	}
	this->triggerEventCallbacks(event);
}

void Control::triggerEventCallbacks(FormsEvent *e)
{
	for (auto &callback : this->callbacks[e->forms().EventFlag])
	{
		callback(e);
	}
}

void Control::addCallback(FormEventType event, std::function<void(FormsEvent *e)> callback)
{
	this->callbacks[event].push_back(callback);
}

bool Control::click()
{
	if (!Visible || !Enabled)
	{
		return false;
	}
	FormsEvent *event = nullptr;
	event = new FormsEvent();
	event->forms().RaisedBy = shared_from_this();
	event->forms().EventFlag = FormEventType::MouseClick;
	fw().pushEvent(event);
	this->setDirty();
	this->triggerEventCallbacks(event);
	return true;
}

void Control::setDirty()
{
	this->dirty = true;
	auto parent = this->getParent();
	if (parent)
	{
		parent->setDirty();
	}
}

void Control::setVisible(bool value)
{
	if (value != this->Visible)
	{
		this->Visible = value;
		this->setDirty();
	}
}

}; // namespace OpenApoc
