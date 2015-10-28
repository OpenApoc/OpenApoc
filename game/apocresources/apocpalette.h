#pragma once

#include "library/strings.h"
#include "library/sp.h"

namespace OpenApoc
{
class Palette;
class Data;
sp<Palette> loadApocPalette(Data &data, const UString fileName);
sp<Palette> loadPCXPalette(Data &data, const UString fileName);
};
