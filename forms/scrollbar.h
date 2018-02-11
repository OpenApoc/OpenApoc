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

	double Value;
	int IsScrolling;
	double pause;
	Orientation BarOrientation;
	void loadResources();
	void UpdateScrollbarParameters();

  protected:
	void onRender() override;

	int Minimum;
	int Maximum;

  public:
	enum class ScrollBarRenderStyle
	{
		Flat,
		Menu
	};

	ScrollBarRenderStyle RenderStyle;
	Colour GripperColour;
	double LargeChange;
	int LargePercent;

	ScrollBar(sp<Image> gripperImage = nullptr);
	~ScrollBar() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;
	void SetScrolling(int dir);
	virtual int getValue() const
	{
		return (int)Value;
		;
	}
	virtual int getMinimum() const { return Minimum; }
	virtual int getMaximum() const { return Maximum; }
	virtual bool setValue(double newValue);
	virtual bool setMinimum(int newMininum);
	virtual bool setMaximum(int newMaximum);
	virtual void scrollPrev();
	virtual void scrollNext();

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
