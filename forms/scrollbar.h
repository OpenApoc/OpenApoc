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
	int ScrollChange;
	int ScrollPercent;

	void updateScrollChangeValue();

	ScrollBar(sp<Image> gripperImage = nullptr);
	~ScrollBar() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;
	virtual int getValue() const { return Value; }
	virtual int getMinimum() const { return Minimum; }
	virtual int getMaximum() const { return Maximum; }
	virtual bool setValue(int newValue);
	virtual bool setMinimum(int newMininum);
	virtual bool setMaximum(int newMaximum);
	virtual void scrollPrev(int amount = 0);
	virtual void scrollNext(int amount = 0);
	void scrollMin() { setValue(Minimum); }
	void scrollMax() { setValue(Maximum); }
	void scrollWheel(Event *e);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
