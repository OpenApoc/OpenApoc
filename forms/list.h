
#pragma once

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc
{

class ScrollBar;

class ListBox : public Control
{
  private:
	bool scroller_is_internal;
	sp<Control> hovered, selected;

	void ConfigureInternalScrollBar();

  protected:
	virtual void OnRender() override;
	virtual void PostRender() override;

  public:
	sp<ScrollBar> scroller;
	int ItemSize, ItemSpacing;
	Orientation ListOrientation;
	Colour HoverColour, SelectedColour;

	ListBox();
	ListBox(sp<ScrollBar> ExternalScrollBar);
	virtual ~ListBox();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	void setSelected(sp<Control> c);

	void Clear();
	void AddItem(sp<Control> Item);
	sp<Control> RemoveItem(sp<Control> Item);
	sp<Control> RemoveItem(int Index);
	sp<Control> operator[](int Index);

	virtual sp<Control> CopyTo(sp<Control> CopyParent) override;
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element) override;

	template <typename T> sp<T> GetHoveredData() const
	{
		if (hovered != nullptr)
		{
			return hovered->GetData<T>();
		}
		return nullptr;
	}

	template <typename T> sp<T> GetSelectedData() const
	{
		if (selected != nullptr)
		{
			return selected->GetData<T>();
		}
		return nullptr;
	}
};

}; // namespace OpenApoc
