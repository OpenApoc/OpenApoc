
#pragma once
#include "library/sp.h"

#include "control.h"

namespace OpenApoc
{

class Sample;
class Image;

class CheckBox : public Control
{

  private:
	sp<Image> imagechecked;
	sp<Image> imageunchecked;

	sp<Sample> buttonclick;

	void LoadResources();

  protected:
	virtual void OnRender() override;

  public:
	bool Checked;

	CheckBox(Control *Owner);
	virtual ~CheckBox();

	virtual void EventOccured(Event *e) override;
	virtual void Update() override;
	virtual void UnloadResources() override;

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
