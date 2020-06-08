#include "forms/listbox.h"
#include "dependencies/pugixml/src/pugixml.hpp"
#include "forms/scrollbar.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/renderer.h"

namespace OpenApoc
{

ListBox::ListBox() : ListBox(nullptr) {}

ListBox::ListBox(sp<ScrollBar> ExternalScrollBar)
    : Control(), scroller_is_internal(ExternalScrollBar == nullptr), scroller(ExternalScrollBar),
      ItemSize(64), ItemSpacing(1), ListOrientation(Orientation::Vertical),
      ScrollOrientation(ListOrientation), HoverColour(0, 0, 0, 0), SelectedColour(0, 0, 0, 0)
{
}

ListBox::~ListBox() = default;

void ListBox::configureInternalScrollBar()
{
	scroller = this->createChild<ScrollBar>();
	scroller->Size.x = 16;
	scroller->Size.y = 16;
	switch (ScrollOrientation)
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

void ListBox::onRender()
{
	Control::onRender();

	Vec2<int> controlOffset = {0, 0};
	if (scroller == nullptr)
	{
		configureInternalScrollBar();
	}

	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		auto ctrl = *c;
		if (ctrl != scroller && ctrl->isVisible())
		{
			if (ListOrientation == ScrollOrientation && ItemSize != 0)
			{
				switch (ScrollOrientation)
				{
					case Orientation::Vertical:
						ctrl->Size.x = (scroller_is_internal ? scroller->Location.x : this->Size.x);
						ctrl->Size.y = ItemSize;
						break;
					case Orientation::Horizontal:
						ctrl->Size.x = ItemSize;
						ctrl->Size.y = (scroller_is_internal ? scroller->Location.y : this->Size.y);
						break;
				}
			}

			switch (ListOrientation)
			{
				case Orientation::Vertical:
					if (ListOrientation != ScrollOrientation &&
					    controlOffset.y + ctrl->Size.y > Size.y)
					{
						controlOffset.y = 0;
						controlOffset.x += ctrl->Size.x + ItemSpacing;
					}
					ctrl->Location = controlOffset - this->scrollOffset;
					controlOffset.y += ctrl->Size.y + ItemSpacing;
					break;
				case Orientation::Horizontal:
					if (ListOrientation != ScrollOrientation &&
					    controlOffset.x + ctrl->Size.x > Size.x)
					{
						controlOffset.x = 0;
						controlOffset.y += ctrl->Size.y + ItemSpacing;
					}
					ctrl->Location = controlOffset - this->scrollOffset;
					controlOffset.x += ctrl->Size.x + ItemSpacing;
					break;
			}
		}
	}

	resolveLocation();
	switch (ScrollOrientation)
	{
		case Orientation::Vertical:
			scroller->setMaximum(std::max(controlOffset.y - this->Size.y, scroller->getMinimum()));
			break;
		case Orientation::Horizontal:
			scroller->setMaximum(std::max(controlOffset.x - this->Size.x, scroller->getMinimum()));
			break;
	}
	scroller->updateLargeChangeValue();
}

void ListBox::postRender()
{
	Control::postRender();
	for (auto c = Controls.begin(); c != Controls.end(); c++)
	{
		auto ctrl = *c;
		if (ctrl != scroller && ctrl->isVisible())
		{
			if (ctrl == hovered)
			{
				if (HoverImage)
				{
					fw().renderer->draw(HoverImage, ctrl->Location);
				}
				else
				{
					fw().renderer->drawRect(ctrl->Location, ctrl->Size, HoverColour);
				}
			}
			if (ctrl == selected)
			{
				if (SelectedImage)
				{
					fw().renderer->draw(SelectedImage, ctrl->Location);
				}
				else
				{
					fw().renderer->drawRect(ctrl->Location, ctrl->Size, SelectedColour);
				}
			}
		}
	}
}

void ListBox::eventOccured(Event *e)
{
	// ListBox does not pass mousedown and mouseup events when out of bounds
	if ((e->type() != EVENT_MOUSE_DOWN && e->type() != EVENT_MOUSE_UP) || eventIsWithin(e))
	{
		Control::eventOccured(e);
	}
	if (e->type() == EVENT_FORM_INTERACTION)
	{
		sp<Control> ctrl = e->forms().RaisedBy;
		sp<Control> child = ctrl->getAncestor(shared_from_this());
		if (e->forms().EventFlag == FormEventType::MouseMove)
		{
			// FIXME: Scrolling amount should match wheel amount
			// Should wheel orientation match as well? Who has horizontal scrolls??
			if (ctrl == shared_from_this() || child != nullptr)
			{
				int wheelDelta =
				    e->forms().MouseInfo.WheelVertical + e->forms().MouseInfo.WheelHorizontal;
				if (wheelDelta > 0)
				{
					scroller->scrollPrev();
				}
				else if (wheelDelta < 0)
				{
					scroller->scrollNext();
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
		else if (e->forms().EventFlag == FormEventType::MouseDown)
		{
			if (ctrl == shared_from_this() || ctrl == scroller || ctrl != child)
			{
				child = nullptr;
			}
			if (selected != child && child != nullptr)
			{
				selected = child;
				this->pushFormEvent(FormEventType::ListBoxChangeSelected, e);
			}
		}
	}
}

void ListBox::update()
{
	Control::update();
	if (scroller == nullptr)
	{
		configureInternalScrollBar();
	}
	if (scroller)
	{
		size_t scrollerLength = Controls.empty() ? 0 : ItemSpacing * (Controls.size() - 1);
		// deduct the listbox size from the content size
		// assume item sizes are variable if ItemSize is 0 (ie: need to calculate manually)
		switch (ListOrientation)
		{
			case Orientation::Vertical:
			{
				if (ItemSize == 0)
				{
					for (const auto &i : Controls)
						scrollerLength += i->Size.y;
				}
				else
				{
					scrollerLength += Controls.size() * ItemSize;
				}
				scrollerLength = scrollerLength > Size.y ? scrollerLength - Size.y : 0;
				break;
			}
			case Orientation::Horizontal:
			{
				if (ItemSize == 0)
				{
					for (const auto &i : Controls)
						scrollerLength += i->Size.x;
				}
				else
				{
					scrollerLength += Controls.size() * ItemSize;
				}
				scrollerLength = scrollerLength > Size.x ? scrollerLength - Size.x : 0;
				break;
			}
			default:
				LogWarning("Unknown ListBox::ListOrientation value: %d",
				           static_cast<int>(ListOrientation));
				break;
		}
		scroller->setMaximum(scroller->getMinimum() + scrollerLength);
		scroller->update();
		Vec2<int> newScrollOffset = this->scrollOffset;
		switch (ScrollOrientation)
		{
			case Orientation::Vertical:
				newScrollOffset.y = scroller->getValue();
				break;
			case Orientation::Horizontal:
				newScrollOffset.x = scroller->getValue();
				break;
		}
		if (newScrollOffset != this->scrollOffset)
		{
			this->scrollOffset = newScrollOffset;
			this->setDirty();
		}
	}
}

void ListBox::unloadResources() {}

void ListBox::clear()
{
	for (auto &c : Controls)
	{
		c->setParent(nullptr);
	}
	Controls.clear();
	this->selected = nullptr;
	this->hovered = nullptr;
	if (scroller_is_internal)
	{
		configureInternalScrollBar();
	}
	resolveLocation();
	this->setDirty();
}

void ListBox::addItem(sp<Control> Item)
{
	Item->setParent(shared_from_this());
	Item->ToolTipFont = this->ToolTipFont;
	resolveLocation();
	if (selected == nullptr && AutoSelect)
	{
		selected = Item;
	}
	this->setDirty();
}

void ListBox::replaceItem(sp<Control> Item)
{
	auto newData = Item->getData<void>();
	this->setDirty();
	bool found = false;
	for (size_t i = 0; i < Controls.size(); i++)
	{
		auto oldItem = Controls[i];
		if (oldItem->getData<void>() == newData)
		{
			Controls.erase(Controls.begin() + i);
			Item->setParent(shared_from_this(), i);
			Item->ToolTipFont = this->ToolTipFont;
			resolveLocation();
			if (oldItem == this->selected)
			{
				this->selected = Item;
			}
			if (oldItem == this->hovered)
			{
				this->hovered = Item;
			}
			found = true;
			break;
		}
	}
	if (!found)
	{
		addItem(Item);
	}
}

sp<Control> ListBox::removeItem(sp<Control> Item)
{
	this->setDirty();
	if (Item == this->selected)
	{
		this->selected = nullptr;
	}
	if (Item == this->hovered)
	{
		this->hovered = nullptr;
	}
	for (auto i = Controls.begin(); i != Controls.end(); i++)
	{
		if (*i == Item)
		{
			Controls.erase(i);
			resolveLocation();
			Item->setParent(nullptr);
			return Item;
		}
	}
	return nullptr;
}

sp<Control> ListBox::removeItem(int Index)
{
	this->setDirty();
	auto c = Controls.at(Index);
	Controls.erase(Controls.begin() + Index);
	resolveLocation();
	if (c == this->selected)
	{
		this->selected = nullptr;
	}
	if (c == this->hovered)
	{
		this->hovered = nullptr;
	}
	c->setParent(nullptr);
	return c;
}

sp<Control> ListBox::operator[](int Index) { return Controls.at(Index); }

sp<Control> ListBox::copyTo(sp<Control> CopyParent)
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
	copy->ScrollOrientation = this->ScrollOrientation;
	copy->HoverColour = this->HoverColour;
	copy->SelectedColour = this->SelectedColour;
	copy->AutoSelect = this->AutoSelect;
	copyControlData(copy);
	return copy;
}

void ListBox::configureSelfFromXml(pugi::xml_node *node)
{
	Control::configureSelfFromXml(node);

	auto itemNode = node->child("item");
	if (itemNode)
	{
		if (itemNode.attribute("size"))
		{
			ItemSize = itemNode.attribute("size").as_int();
		}
		if (itemNode.attribute("spacing"))
		{
			ItemSpacing = itemNode.attribute("spacing").as_int();
		}
	}
	auto orientationNode = node->child("orientation");
	if (orientationNode)
	{
		UString value = orientationNode.text().get();
		if (value == "horizontal")
		{
			ListOrientation = Orientation::Horizontal;
			ScrollOrientation = Orientation::Horizontal;
		}
		else if (value == "vertical")
		{
			ListOrientation = Orientation::Vertical;
			ScrollOrientation = Orientation::Vertical;
		}
		if (orientationNode.attribute("list"))
		{
			value = orientationNode.attribute("list").as_string();
			if (value == "horizontal")
			{
				ListOrientation = Orientation::Horizontal;
			}
			else if (value == "vertical")
			{
				ListOrientation = Orientation::Vertical;
			}
		}
		if (orientationNode.attribute("scroll"))
		{
			value = orientationNode.attribute("scroll").as_string();
			if (value == "horizontal")
			{
				ScrollOrientation = Orientation::Horizontal;
			}
			else if (value == "vertical")
			{
				ScrollOrientation = Orientation::Vertical;
			}
		}
	}
	auto hoverColourNode = node->child("hovercolour");
	if (hoverColourNode)
	{
		uint8_t r = hoverColourNode.attribute("r").as_uint(0);
		uint8_t g = hoverColourNode.attribute("g").as_uint(0);
		uint8_t b = hoverColourNode.attribute("b").as_uint(0);
		uint8_t a = hoverColourNode.attribute("a").as_uint(255);
		HoverColour = {r, g, b, a};
	}
	auto selColourNode = node->child("selcolour");
	if (selColourNode)
	{
		uint8_t r = selColourNode.attribute("r").as_uint(0);
		uint8_t g = selColourNode.attribute("g").as_uint(0);
		uint8_t b = selColourNode.attribute("b").as_uint(0);
		uint8_t a = selColourNode.attribute("a").as_uint(255);
		SelectedColour = {r, g, b, a};
	}
	auto autoSelectNode = node->child("autoselect");
	if (autoSelectNode)
	{
		AutoSelect = autoSelectNode.text().as_bool(true);
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
	if (c && !found)
	{
		LogError(
		    "Trying set ListBox selected control to something that isn't a member of the list");
	}
	this->selected = c;
	this->setDirty();
}

sp<Control> ListBox::getSelectedItem() { return selected; }

sp<Control> ListBox::getHoveredItem() { return hovered; }

}; // namespace OpenApoc
