
#pragma once
#include "checkbox.h"

namespace OpenApoc
{

class RadioButton : public CheckBox
{

  private:
	RadioButton **group;

  public:
	RadioButton(Control *Owner, RadioButton **Group, sp<Image> ImageChecked = nullptr,
	            sp<Image> ImageUnchecked = nullptr);
	virtual ~RadioButton();
	virtual void SetChecked(bool checked);

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
