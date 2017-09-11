#pragma once

#include "forms/control.h"
#include "library/sp.h"

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

	bool click() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;
	virtual int getState();
	virtual void setState(int state);
	virtual void nextState();

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
