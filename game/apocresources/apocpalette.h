#pragma once

#include <string>

namespace OpenApoc
{
	class Palette;
	class Data;
	Palette* loadApocPalette(Data &data, const std::string fileName);
};
