
#pragma once

#include "control.h"

namespace OpenApoc
{

class Framework;

class VScrollBar : public Control
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

	VScrollBar(Framework &fw, Control *Owner);
	VScrollBar(Framework &fw, Control *Owner, Control *AssociateWith);
	virtual ~VScrollBar();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
