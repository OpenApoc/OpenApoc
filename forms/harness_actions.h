#pragma once

#include "library/sp.h"

namespace OpenApoc
{

class Form;

void notifyVisibleForm(const sp<Form> &form);
void installFormsHarnessActions();

} // namespace OpenApoc
