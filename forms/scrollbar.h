
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
	void OnRender() override;

  public:
	enum class ScrollBarRenderStyle
	{
		Flat,
		Menu
	};

	ScrollBarRenderStyle RenderStyle;
	Colour GripperColour;
	int Minimum;
	int Maximum;
	int LargeChange;

	ScrollBar();
	~ScrollBar() override;

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;
	virtual int GetValue() const { return Value; }
	virtual bool SetValue(int newValue);
	virtual void ScrollPrev();
	virtual void ScrollNext();

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureSelfFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
