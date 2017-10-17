#pragma once

#include "forms/control.h"
#include "forms/forms_enums.h"
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
	void loadResources();

  protected:
	void onRender() override;

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
	int LargePercent;

	void updateLargeChangeValue();

	ScrollBar(sp<Image> gripperImage = nullptr);
	~ScrollBar() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;
	virtual int getValue() const { return Value; }
	virtual bool setValue(int newValue);
	virtual void scrollPrev(bool small = false);
	virtual void scrollNext(bool small = false);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
