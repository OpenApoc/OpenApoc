
#pragma once

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc
{

class ScrollBar;

class ListBox : public Control
{
  private:
	// std::vector<Control*> items;
	ScrollBar *scroller;
	bool scroller_is_internal;

	void ConfigureInternalScrollBar();

  protected:
	virtual void OnRender() override;

  public:
	int ItemSize, ItemSpacing;
	Orientation ListOrientation;

	ListBox(Control *Owner);
	ListBox(Control *Owner, ScrollBar *ExternalScrollBar);
	virtual ~ListBox();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	void Clear();
	void AddItem(Control *Item);
	Control *RemoveItem(Control *Item);
	Control *RemoveItem(int Index);
	Control *operator[](int Index);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
