
#pragma once

#include "control.h"

namespace OpenApoc
{

class Framework;

class HScrollBar : public Control
{
  private:
	bool capture;

	void LoadResources();

  protected:
	virtual void OnRender() override;

  public:
	Colour GripperColour;
	int Minimum;
	int Maximum;
	int Value;
	int LargeChange;
	Control *AssociatedControl;

	HScrollBar(Framework &fw, Control *Owner);
	HScrollBar(Framework &fw, Control *Owner, Control *AssociateWith);
	virtual ~HScrollBar();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;
	virtual void SetValue(int newValue);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
