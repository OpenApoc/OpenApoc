
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

	void OnRender() override;

  public:
	CheckBox(sp<Image> ImageChecked = nullptr, sp<Image> ImageUnchecked = nullptr);
	virtual ~CheckBox();

	void EventOccured(Event *e) override;
	void Update() override;
	void UnloadResources() override;
	virtual bool IsChecked() const { return Checked; }
	virtual void SetChecked(bool checked);

	sp<Control> CopyTo(sp<Control> CopyParent) override;
	void ConfigureSelfFromXML(tinyxml2::XMLElement *Element) override;
};

}; // namespace OpenApoc
