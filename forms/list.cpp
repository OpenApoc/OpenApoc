
#include "forms/list.h"
#include "forms/scrollbar.h"

namespace OpenApoc
{

ListBox::ListBox(Framework &fw, Control *Owner) : ListBox(fw, Owner, nullptr) {}

ListBox::ListBox(Framework &fw, Control *Owner, ScrollBar *ExternalScrollBar)
    : Control(fw, Owner), scroller(ExternalScrollBar), ItemSize(64),
      ListOrientation(Orientation::Vertical)
{
	if (scroller != nullptr)
	{
		scroller->AssociatedControl = this;
		scroller_is_internal = false;
	}
}

ListBox::~ListBox() { Clear(); }

void ListBox::ConfigureInternalScrollBar()
{
	scroller = new ScrollBar(fw, this);
	scroller->Size.x = 16;
	scroller->Size.y = 16;
	switch (ListOrientation)
	{
		case Orientation::Vertical:
			scroller->Location.x = this->Size.x - 16;
			scroller->Location.y = 0;
			break;
		case Orientation::Horizontal:
			scroller->Location.x = 0;
			scroller->Location.y = this->Size.y - 16;
			break;
	}
	scroller->canCopy = false;
	scroller_is_internal = true;
}

void ListBox::OnRender()
{
	if (scroller_is_internal)
	{
		switch (ListOrientation)
		{
			case Orientation::Vertical:
				scroller->Location.x = this->Size.x - scroller->Size.x;
				scroller->Size.y = this->Size.y;
				break;
			case Orientation::Horizontal:
				scroller->Location.y = this->Size.y - scroller->Size.y;
				scroller->Size.x = this->Size.x;
				break;
		}
	}

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
					ctrl->Size.x = (scroller_is_internal ? scroller->Location.x : this->Size.x);
					ctrl->Size.y = ItemSize;
					offset += ctrl->Size.y + 1;
					break;
				case Orientation::Horizontal:
					ctrl->Location.x = offset - scroller->GetValue();
					ctrl->Location.y = 0;
					ctrl->Size.x = ItemSize;
					ctrl->Size.y = (scroller_is_internal ? scroller->Location.y : this->Size.y);
					offset += ctrl->Size.x + 1;
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
		copy = new ListBox(fw, CopyParent);
	}
	else
	{
		copy = new ListBox(fw, CopyParent, static_cast<ScrollBar *>(scroller->lastCopiedTo));
	}
	copy->ItemSize = this->ItemSize;
	copy->ListOrientation = this->ListOrientation;
	CopyControlData(copy);
	return copy;
}

}; // namespace OpenApoc
