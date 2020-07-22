#pragma once

#include "forms/control.h"
#include "library/sp.h"

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

	void onRender() override;

  public:
	CheckBox(sp<Image> ImageChecked = nullptr, sp<Image> ImageUnchecked = nullptr);
	~CheckBox() override;

	bool click() override;

	void eventOccured(Event *e) override;
	void update() override;
	void unloadResources() override;

	sp<Sample> getClickSound() const;
	void setClickSound(sp<Sample> sample);
	virtual bool isChecked() const { return Checked; }
	virtual void setChecked(bool checked);

	sp<Control> copyTo(sp<Control> CopyParent) override;
	void configureSelfFromXml(pugi::xml_node *node) override;
};

}; // namespace OpenApoc
