
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
	void SetChecked(bool checked) override;

	virtual Control *CopyTo(Control *CopyParent) override;
};

}; // namespace OpenApoc
