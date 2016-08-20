#pragma once

#include "checkbox.h"
#include <list>

namespace OpenApoc
{

class RadioButton;

class RadioButtonGroup
{
  public:
	RadioButtonGroup(UString ID);
	UString ID;
	std::list<wp<RadioButton>> radioButtons;
};

class RadioButton : public CheckBox
{

  private:
	sp<RadioButtonGroup> group;

  public:
	RadioButton(sp<RadioButtonGroup> radioButtonGroup = nullptr, sp<Image> ImageChecked = nullptr,
	            sp<Image> ImageUnchecked = nullptr);
	~RadioButton() override;
	void setChecked(bool checked) override;

	sp<Control> copyTo(sp<Control> CopyParent) override;
};

}; // namespace OpenApoc
