#include "library/sp.h"

#include "forms/radiobutton.h"
#include "framework/framework.h"

namespace OpenApoc
{

RadioButton::RadioButton(sp<RadioButtonGroup> Group, sp<Image> ImageChecked,
                         sp<Image> ImageUnchecked)
    : CheckBox(ImageChecked, ImageUnchecked), group(Group)
{
}

RadioButton::~RadioButton() = default;

sp<Control> RadioButton::copyTo(sp<Control> CopyParent)
{
	sp<RadioButton> copy;
	sp<RadioButtonGroup> newGroup;
	if (CopyParent)
	{
		if (this->group)
		{
			auto groupIt = CopyParent->radiogroups.find(this->group->ID);
			if (groupIt == CopyParent->radiogroups.end())
			{
				CopyParent->radiogroups[this->group->ID] = mksp<RadioButtonGroup>(this->group->ID);
			}
			newGroup = CopyParent->radiogroups[this->group->ID];
		}
		copy = CopyParent->createChild<RadioButton>(newGroup, imagechecked, imageunchecked);
	}
	else
	{
		copy = mksp<RadioButton>(newGroup, imagechecked, imageunchecked);
	}
	copyControlData(copy);
	if (newGroup)
	{
		newGroup->radioButtons.push_back(copy);
	}
	return copy;
}

void RadioButton::setChecked(bool checked)
{
	if (checked != Checked)
	{
		if (group && checked)
		{
			for (auto &c : group->radioButtons)
			{
				auto button = c.lock();
				if (button)
				{
					button->setChecked(false);
				}
			}
		}
		CheckBox::setChecked(checked);
	}
}

RadioButtonGroup::RadioButtonGroup(UString ID) : ID(ID) {}

}; // namespace OpenApoc
