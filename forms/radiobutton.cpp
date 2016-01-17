#include "library/sp.h"

#include "forms/radiobutton.h"
#include "framework/framework.h"

namespace OpenApoc
{

RadioButton::RadioButton(Control *Owner, RadioButton **Group, sp<Image> ImageChecked,
                         sp<Image> ImageUnchecked)
    : CheckBox(Owner, ImageChecked, ImageUnchecked), group(Group)
{
}

RadioButton::~RadioButton() {}

Control *RadioButton::CopyTo(Control *CopyParent)
{
	RadioButton *copy = new RadioButton(CopyParent, group, imagechecked, imageunchecked);
	*group = nullptr;
	CopyControlData(copy);
	return copy;
}

void RadioButton::SetChecked(bool checked)
{
	if (checked && !Checked)
	{
		if (*group != nullptr)
		{
			(*group)->Checked = false;
		}
		*group = this;
		CheckBox::SetChecked(checked);
	}
}

}; // namespace OpenApoc
