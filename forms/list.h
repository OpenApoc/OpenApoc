
#pragma once

#include "control.h"

namespace OpenApoc
{

class Framework;
class VScrollBar;

class ListBox : public Control
{
  private:
	// std::vector<Control*> items;
	VScrollBar *scroller;
	bool scroller_is_internal;

	void ConfigureInternalScrollBar();

  protected:
	virtual void OnRender() override;

  public:
	int ItemHeight;

	ListBox(Framework &fw, Control *Owner);
	ListBox(Framework &fw, Control *Owner, VScrollBar *ExternalScrollBar);
	virtual ~ListBox();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	void Clear();
	void AddItem(Control *Item);
	Control *RemoveItem(Control *Item);
	Control *RemoveItem(int Index);
	Control *operator[](int Index);
};

}; // namespace OpenApoc
