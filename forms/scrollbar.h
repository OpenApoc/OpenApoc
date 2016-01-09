
#pragma once

#include "control.h"
#include "forms_enums.h"
#include "library/sp.h"

namespace OpenApoc
{

class Image;
class Sample;

class ScrollBar : public Control
{
  private:
	bool capture;
	float grippersize;
	float segmentsize;
	sp<Image> gripperbutton;
	sp<Sample> buttonerror;

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

	ScrollBar(Control *Owner);
	virtual ~ScrollBar();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;
	virtual int GetValue() const { return Value; }
	virtual bool SetValue(int newValue);
	virtual void ScrollPrev();
	virtual void ScrollNext();

	virtual Control *CopyTo(Control *CopyParent) override;
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
