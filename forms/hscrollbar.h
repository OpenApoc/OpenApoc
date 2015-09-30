
#pragma once

#include "control.h"

namespace OpenApoc
{

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

	HScrollBar(Framework &fw, Control *Owner);
	virtual ~HScrollBar();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;
	virtual void SetValue(int newValue);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
