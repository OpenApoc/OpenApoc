#pragma once
#include "library/sp.h"

#include "control.h"

namespace OpenApoc
{

class Sample;
class Image;

class TriStateBox : public Control
{

  protected:
	sp<Image> image1;
	sp<Image> image2;
	sp<Image> image3;

	sp<Sample> buttonclick;

	int State;

	void onRender() override;

  public:
	TriStateBox(sp<Image> Image1 = nullptr, sp<Image> Image2 = nullptr, sp<Image> Image3 = nullptr);
	~TriStateBox() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;
	virtual int getState();
	virtual void setState(int state);
	virtual void nextState();

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
