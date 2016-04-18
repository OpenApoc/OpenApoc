#include "forms/list.h"
#include "forms/scrollbar.h"
#include "framework/event.h"
#include "framework/framework.h"
#include <tinyxml2.h>

namespace OpenApoc
{

ListBox::ListBox() : ListBox(nullptr) {}

ListBox::ListBox(sp<ScrollBar> ExternalScrollBar)
    : Control(), scroller_is_internal(ExternalScrollBar == nullptr), scroller(ExternalScrollBar),
      ItemSize(64), ItemSpacing(1), ListOrientation(Orientation::Vertical), HoverColour(0, 0, 0, 0),
      SelectedColour(0, 0, 0, 0), AlwaysEmitSelectionEvents(false)
{
}

ListBox::~ListBox() {}

void ListBox::ConfigureInternalScrollBar()
{
	scroller = this->createChild<ScrollBar>();
	scroller->Size.x = 16;
	scroller->Size.y = 16;
	switch (ListOrientation)
	{
		case Orientation::Vertical:
			scroller->Location.x = this->Size.x - scroller->Size.x;
			scroller->Location.y = 0;
			scroller->Size.y = this->Size.y;
			break;
		case Orientation::Horizontal:
			scroller->Location.x = 0;
			scroller->Location.y = this->Size.y - scroller->Size.y;
			scroller->Size.x = this->Size.x;
			break;
	}
	scroller->canCopy = false;
	scroller_is_internal = true;
}

void ListBox::OnRender()
{
	int offset = 0;
	if (scroller == nullptr)
	{
		ConfigureInternalScrollBar();
	}

	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		auto ctrl = *c;
		if (ctrl != scroller)
		{
			switch (ListOrientation)
			{
				case Orientation::Vertical:
					ctrl->Location.x = 0;
					ctrl->Location.y = offset - scroller->GetValue();
					if (ItemSize != 0)
					{
						ctrl->Size.x = (scroller_is_internal ? scroller->Location.x : this->Size.x);
						ctrl->Size.y = ItemSize;
					}
					offset += ctrl->Size.y + ItemSpacing;
					break;
				case Orientation::Horizontal:
					ctrl->Location.x = offset - scroller->GetValue();
					ctrl->Location.y = 0;
					if (ItemSize != 0)
					{
						ctrl->Size.x = ItemSize;
						ctrl->Size.y = (scroller_is_internal ? scroller->Location.y : this->Size.y);
					}
					offset += ctrl->Size.x + ItemSpacing;
					break;
			}
		}
	}
	ResolveLocation();
	switch (ListOrientation)
	{
		case Orientation::Vertical:
			scroller->Maximum = std::max(offset - this->Size.y, scroller->Minimum);
			break;
		case Orientation::Horizontal:
			scroller->Maximum = std::max(offset - this->Size.x, scroller->Minimum);
			break;
	}
	scroller->LargeChange =
	    static_cast<int>(std::max((scroller->Maximum - scroller->Minimum + 2) / 10.0f, 4.0f));
}

void ListBox::PostRender()
{
	Control::PostRender();
	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		auto ctrl = *c;
		if (ctrl != scroller)
		{
			if (ctrl == hovered)
			{
				fw().renderer->drawRect(ctrl->Location, ctrl->Size, HoverColour);
			}
			if (ctrl == selected)
			{
				fw().renderer->drawRect(ctrl->Location, ctrl->Size, SelectedColour);
			}
		}
	}
}

void ListBox::EventOccured(Event *e)
{
	Control::EventOccured(e);
	if (e->Type() == EVENT_FORM_INTERACTION)
	{
		sp<Control> ctrl = e->Forms().RaisedBy;
		sp<Control> child = ctrl->GetAncestor(shared_from_this());
		if (e->Forms().EventFlag == FormEventType::MouseMove)
		{
			// FIXME: Scrolling amount should match wheel amount
			// Should wheel orientation match as well? Who has horizontal scrolls??
			if (ctrl == shared_from_this() || child != nullptr)
			{
				int wheelDelta =
				    e->Forms().MouseInfo.WheelVertical + e->Forms().MouseInfo.WheelHorizontal;
				if (wheelDelta > 0)
				{
					scroller->ScrollPrev();
				}
				else if (wheelDelta < 0)
				{
					scroller->ScrollNext();
				}
			}

			if (ctrl == shared_from_this() || ctrl == scroller)
			{
				child = nullptr;
			}
			if (hovered != child)
			{
				hovered = child;
				this->pushFormEvent(FormEventType::ListBoxChangeHover, e);
			}
		}
		else if (e->Forms().EventFlag == FormEventType::MouseDown)
		{
			if (ctrl == shared_from_this() || ctrl == scroller)
			{
				child = nullptr;
			}
			if ((AlwaysEmitSelectionEvents || selected != child) && child != nullptr)
			{
				selected = child;
				this->pushFormEvent(FormEventType::ListBoxChangeSelected, e);
			}
		}
	}
}

void ListBox::Update()
{
	Control::Update();
	if (scroller == nullptr)
	{
		ConfigureInternalScrollBar();
	}
	if (scroller)
	{
		scroller->Update();
	}
}

void ListBox::UnloadResources() {}

void ListBox::Clear()
{
	for (auto &c : Controls)
	{
		c->SetParent(nullptr);
	}
	Controls.clear();
	this->selected = nullptr;
	this->hovered = nullptr;
	if (scroller_is_internal)
	{
		ConfigureInternalScrollBar();
	}
	ResolveLocation();
}

void ListBox::AddItem(sp<Control> Item)
{
	Item->SetParent(shared_from_this());
	ResolveLocation();
	if (selected == nullptr)
	{
		selected = Item;
	}
}

sp<Control> ListBox::RemoveItem(sp<Control> Item)
{
	for (auto i = Controls.begin(); i != Controls.end(); i++)
	{
		if (*i == Item)
		{
			Controls.erase(i);
			ResolveLocation();
			return Item;
		}
	}
	if (Item == this->selected)
	{
		this->selected = nullptr;
	}
	if (Item == this->hovered)
	{
		this->selected = nullptr;
	}
	return nullptr;
}

sp<Control> ListBox::RemoveItem(int Index)
{
	auto c = Controls.at(Index);
	Controls.erase(Controls.begin() + Index);
	ResolveLocation();
	if (c == this->selected)
	{
		this->selected = nullptr;
	}
	if (c == this->hovered)
	{
		this->selected = nullptr;
	}
	return c;
}

sp<Control> ListBox::operator[](int Index) { return Controls.at(Index); }

sp<Control> ListBox::CopyTo(sp<Control> CopyParent)
{
	sp<ListBox> copy;
	sp<ScrollBar> scrollCopy;
	if (!scroller_is_internal)
	{
		scrollCopy = std::dynamic_pointer_cast<ScrollBar>(scroller->lastCopiedTo.lock());
	}

	if (CopyParent)
	{
		copy = CopyParent->createChild<ListBox>(scrollCopy);
	}
	else
	{
		copy = mksp<ListBox>(scrollCopy);
	}

	copy->ItemSize = this->ItemSize;
	copy->ItemSpacing = this->ItemSpacing;
	copy->ListOrientation = this->ListOrientation;
	copy->HoverColour = this->HoverColour;
	copy->SelectedColour = this->SelectedColour;
	CopyControlData(copy);
	return copy;
}

void ListBox::ConfigureFromXML(tinyxml2::XMLElement *Element)
{
	Control::ConfigureFromXML(Element);
	tinyxml2::XMLElement *subnode;
	UString attribvalue;

	subnode = Element->FirstChildElement("item");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("size") != nullptr && UString(subnode->Attribute("size")) != "")
		{
			ItemSize = Strings::ToInteger(subnode->Attribute("size"));
		}
		if (subnode->Attribute("spacing") != nullptr &&
		    UString(subnode->Attribute("spacing")) != "")
		{
			ItemSpacing = Strings::ToInteger(subnode->Attribute("spacing"));
		}
	}
	subnode = Element->FirstChildElement("orientation");
	if (subnode != nullptr && UString(subnode->GetText()) != "")
	{
		UString value = subnode->GetText();
		if (value == "horizontal")
		{
			ListOrientation = Orientation::Horizontal;
		}
		else if (value == "vertical")
		{
			ListOrientation = Orientation::Vertical;
		}
	}
	subnode = Element->FirstChildElement("hovercolour");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("a") != nullptr && UString(subnode->Attribute("a")) != "")
		{
			HoverColour = Colour{
			    Strings::ToU8(subnode->Attribute("r")), Strings::ToU8(subnode->Attribute("g")),
			    Strings::ToU8(subnode->Attribute("b")), Strings::ToU8(subnode->Attribute("a"))};
		}
		else
		{
			HoverColour = Colour{Strings::ToU8(subnode->Attribute("r")),
			                     Strings::ToU8(subnode->Attribute("g")),
			                     Strings::ToU8(subnode->Attribute("b"))};
		}
	}
	subnode = Element->FirstChildElement("selcolour");
	if (subnode != nullptr)
	{
		if (subnode->Attribute("a") != nullptr && UString(subnode->Attribute("a")) != "")
		{
			SelectedColour = Colour{
			    Strings::ToU8(subnode->Attribute("r")), Strings::ToU8(subnode->Attribute("g")),
			    Strings::ToU8(subnode->Attribute("b")), Strings::ToU8(subnode->Attribute("a"))};
		}
		else
		{
			SelectedColour = Colour{Strings::ToU8(subnode->Attribute("r")),
			                        Strings::ToU8(subnode->Attribute("g")),
			                        Strings::ToU8(subnode->Attribute("b"))};
		}
	}
}

void ListBox::setSelected(sp<Control> c)
{
	// A sanity check to make sure the selected control actually belongs to this list
	bool found = false;
	for (auto child : this->Controls)
	{
		if (child == c)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		LogError(
		    "Trying set ListBox selected control to something that isn't a member of the list");
	}
	this->selected = c;
}
}; // namespace OpenApoc
