
#include "forms/list.h"
#include "forms/scrollbar.h"

namespace OpenApoc
{

ListBox::ListBox(Control *Owner) : ListBox(Owner, nullptr) {}

ListBox::ListBox(Control *Owner, ScrollBar *ExternalScrollBar)
    : Control(Owner), scroller(ExternalScrollBar), ItemSize(64), ItemSpacing(1),
      ListOrientation(Orientation::Vertical)
{
	scroller_is_internal = (scroller == nullptr);
}

ListBox::~ListBox() { Clear(); }

void ListBox::ConfigureInternalScrollBar()
{
	scroller = new ScrollBar(this);
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
	scroller->Visible = false;
	scroller_is_internal = true;
}

void ListBox::OnRender()
{
	int offset = 0;

	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		Control *ctrl = *c;
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
	switch (ListOrientation)
	{
		case Orientation::Vertical:
			scroller->Maximum = (offset - this->Size.y);
			break;
		case Orientation::Horizontal:
			scroller->Maximum = (offset - this->Size.x);
			break;
	}
	scroller->LargeChange =
	    static_cast<int>(std::max((scroller->Maximum - scroller->Minimum + 2) / 10.0f, 4.0f));
}

void ListBox::EventOccured(Event *e) { Control::EventOccured(e); }

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
	while (Controls.size() > 0)
	{
		delete Controls.back();
		Controls.pop_back();
	}
	if (scroller_is_internal)
	{
		ConfigureInternalScrollBar();
	}
	ResolveLocation();
}

void ListBox::AddItem(Control *Item)
{
	Controls.push_back(Item);
	Item->SetParent(this);
	ResolveLocation();
}

Control *ListBox::RemoveItem(Control *Item)
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
	return nullptr;
}

Control *ListBox::RemoveItem(int Index)
{
	Control *c = Controls.at(Index);
	Controls.erase(Controls.begin() + Index);
	ResolveLocation();
	return c;
}

Control *ListBox::operator[](int Index) { return Controls.at(Index); }

Control *ListBox::CopyTo(Control *CopyParent)
{
	ListBox *copy;
	if (scroller_is_internal)
	{
		copy = new ListBox(CopyParent);
	}
	else
	{
		copy = new ListBox(CopyParent, static_cast<ScrollBar *>(scroller->lastCopiedTo));
	}
	copy->ItemSize = this->ItemSize;
	copy->ItemSpacing = this->ItemSpacing;
	copy->ListOrientation = this->ListOrientation;
	CopyControlData(copy);
	return copy;
}

	void ListBox::ConfigureFromXML(tinyxml2::XMLElement* Element)
	{
		Control::ConfigureFromXML(Element);
		tinyxml2::XMLElement *subnode;
		UString attribvalue;

		subnode = Element->FirstChildElement("item");
		if (subnode != nullptr)
		{
			if (subnode->Attribute("size") != nullptr &&
				UString(subnode->Attribute("size")) != "")
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
	}
}; // namespace OpenApoc
