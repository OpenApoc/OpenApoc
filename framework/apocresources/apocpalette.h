#pragma once

#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{
class Palette;
class Data;
sp<Palette> loadApocPalette(Data &data, const UString fileName);
sp<Palette> loadPCXPalette(Data &data, const UString fileName);
sp<Palette> loadPNGPalette(Data &data, const UString fileName);
}; // namespace OpenApoc
