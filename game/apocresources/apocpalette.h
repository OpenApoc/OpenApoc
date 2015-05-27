#pragma once

#include "library/strings.h"

namespace OpenApoc
{
	class Palette;
	class Data;
	Palette* loadApocPalette(Data &data, const UString fileName);
};
