
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
	Control *hovered, *selected;

	void ConfigureInternalScrollBar();

  protected:
	virtual void OnRender() override;
	virtual void PostRender() override;

  public:
	ScrollBar *scroller;
	int ItemSize, ItemSpacing;
	Orientation ListOrientation;
	Colour HoverColour, SelectedColour;

	ListBox(Control *Owner);
	ListBox(Control *Owner, ScrollBar *ExternalScrollBar);
	virtual ~ListBox();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;
	void *getHoveredData() const;
	void *getSelectedData() const;

	void setSelected(Control *c);

	void Clear();
	void AddItem(Control *Item);
	Control *RemoveItem(Control *Item);
	Control *RemoveItem(int Index);
	Control *operator[](int Index);

	virtual Control *CopyTo(Control *CopyParent) override;
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
