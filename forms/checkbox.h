
#pragma once
#include "library/sp.h"

#include "control.h"

namespace OpenApoc
{

class Sample;
class Image;

class CheckBox : public Control
{

  protected:
	sp<Image> imagechecked;
	sp<Image> imageunchecked;

	sp<Sample> buttonclick;

	bool Checked;

	virtual void OnRender() override;

  public:
	CheckBox(sp<Image> ImageChecked = nullptr, sp<Image> ImageUnchecked = nullptr);
	virtual ~CheckBox();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;
	virtual bool IsChecked() const { return Checked; }
	virtual void SetChecked(bool checked);

	virtual sp<Control> CopyTo(sp<Control> CopyParent) override;
	virtual void ConfigureFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
