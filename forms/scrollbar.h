
#pragma once

#include "control.h"
#include "forms_enums.h"

namespace OpenApoc
{

class Framework;

class ScrollBar : public Control
{
  private:
	bool capture;
	float grippersize;
	float segmentsize;

	int Value;
	Orientation BarOrientation;
	void LoadResources();

  protected:
	virtual void OnRender() override;

  public:
	enum class ScrollBarRenderStyles
	{
		SolidButtonStyle,
		MenuButtonStyle
	};

	ScrollBarRenderStyles RenderStyle;
	Colour GripperColour;
	int Minimum;
	int Maximum;
	int LargeChange;
	Control *AssociatedControl;

	ScrollBar(Framework &fw, Control *Owner);
	ScrollBar(Framework &fw, Control *Owner, Control *AssociateWith);
	virtual ~ScrollBar();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;
	virtual int GetValue() const { return Value; }
	virtual void SetValue(int newValue);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
