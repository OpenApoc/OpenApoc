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
	sp<Control> hovered;
	sp<Control> selected;
	std::set<sp<Control>> selectedSet;
	Vec2<int> scrollOffset = {0, 0};
	bool selection;

  protected:
	void preRender() override;
	void onRender() override;
	void postRender() override;

  public:
	sp<ScrollBar> scroller;
	int ItemSize, ItemSpacing;
	Orientation ListOrientation, ScrollOrientation;
	Colour HoverColour, SelectedColour;
	// Image to use instead of frame for hover
	sp<Image> HoverImage;
	// Image to use instead of frame for selection
	sp<Image> SelectedImage;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	// Set selection status for Item.
	void setSelected(sp<Control> Item, bool select);
	// Get controls that have been selected.
	std::vector<sp<Control>> getSelectedItems() const;
	sp<Control> getHoveredItem();

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
		if (hovered != nullptr)
		{
			return hovered->getData<T>();
		}
		return nullptr;
	}

	// template <typename T> sp<T> getSelectedData() const
	//{
	//	if (selected != nullptr)
	//	{
	//		return selected->getData<T>();
	//	}
	//	return nullptr;
	//}

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
}
