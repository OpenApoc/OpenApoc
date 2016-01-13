#include "library/sp.h"

#include "forms/radiobutton.h"
#include "framework/framework.h"

namespace OpenApoc
{

RadioButton::RadioButton(Control *Owner, RadioButton **Group, sp<Image> ImageChecked,
                         sp<Image> ImageUnchecked)
    : CheckBox(Owner, ImageChecked, ImageUnchecked), group(Group)
{
	if (*group == nullptr)
	{
		*group = this;
		Checked = true;
	}
}

RadioButton::~RadioButton() {}

Control *RadioButton::CopyTo(Control *CopyParent)
{
	RadioButton *copy = new RadioButton(CopyParent, group, imagechecked, imageunchecked);
	copy->Checked = this->Checked;
	CopyControlData(copy);
	return copy;
}

void RadioButton::SetChecked(bool checked)
{
	if (checked)
	{
		(*group)->SetChecked(false);
		*group = this;
	}
	CheckBox::SetChecked(checked);
}

}; // namespace OpenApoc
