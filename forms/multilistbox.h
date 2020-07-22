#pragma once
#include "forms/control.h"
#include "forms/forms_enums.h"
#include "library/colour.h"
#include "library/sp.h"

#include <set>

namespace OpenApoc
{

class ScrollBar;
class Image;

class MultilistBox : public Control
{
  public:
	MultilistBox();
	MultilistBox(sp<ScrollBar> ExternalScrollBar);
	~MultilistBox() override;

  private:
	// Hovered Item.
	sp<Control> hoveredItem;
	// Selected Item during selection action.
	sp<Control> selectedItem;
	// The set of selected Controls.
	std::set<sp<Control>> selectedSet;
	Vec2<int> scrollOffset = {0, 0};
	// Execution of selection action.
	bool selectionAction = false;
	// Strategy func which implements the decision about visibility/invisibility of certain Item.
	// arg - child control
	// TODO: move to the Control class
	std::function<bool(sp<Control>)> isVisibleItem;
	// Additional operations during the selection action.
	// arg1 - child control
	// arg2 - selection (true) or deselection (false) operation
	// return - item should be selected (true/false)
	std::function<bool(Event *, sp<Control>, bool)> funcHandleSelection;
	// Render the "hover" effect.
	// arg - child control
	std::function<void(sp<Control>)> funcHoverItemRender;
	// Render the "selection" effect.
	// arg - child control
	std::function<void(sp<Control>)> funcSelectionItemRender;

  protected:
	void onRender() override;
	void postRender() override;

  public:
	sp<ScrollBar> scroller;
	int ItemSize, ItemSpacing;
	Orientation ListOrientation, ScrollOrientation;
	Colour HoverColour, SelectedColour;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	// Set selection status for Item.
	void setSelected(sp<Control> Item, bool select);
	// Select all items.
	void selectAll();
	// Deselect all selected items.
	void clearSelection();
	// Get vector of controls (with preservation of order) that have been selected.
	std::vector<sp<Control>> getSelectedItems() const;
	// Get set of controls that have been selected.
	std::set<sp<Control>> &getSelectedSet() { return selectedSet; }
	// Get hovered control.
	sp<Control> getHoveredItem() const { return hoveredItem; }
	// Setter for the isVisibleItem function.
	void setFuncIsVisibleItem(std::function<bool(sp<Control>)> func);
	// Setter for the funcHandleSelection function.
	void setFuncHandleSelection(std::function<bool(Event *, sp<Control>, bool)> func);
	// Setter for the funcHoverItemRender function.
	void setFuncHoverItemRender(std::function<void(sp<Control>)> func);
	// Setter for the funcSelectionItemRender function.
	void setFuncSelectionItemRender(std::function<void(sp<Control>)> func);

	void clear();
	void addItem(sp<Control> Item);
	void replaceItem(sp<Control> Item);
	sp<Control> removeItem(sp<Control> Item);
	sp<Control> removeItem(int Index);
	sp<Control> operator[](int Index);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;

	template <typename T> sp<T> getHoveredData() const
	{
		if (hoveredItem != nullptr)
		{
			return hoveredItem->getData<T>();
		}
		return nullptr;
	}

	template <typename T> sp<Control> removeByData(const sp<T> data)
	{
		this->setDirty();
		sp<Control> Item;
		for (auto i = Controls.begin(); i != Controls.end(); i++)
		{
			if ((*i)->getData<T>() == data)
			{
				Item = *i;
				break;
			}
		}
		return removeItem(Item);
	}
};
} // namespace OpenApoc
