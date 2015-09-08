#include "framework/palette.h"
#include <cassert>

namespace OpenApoc
{

Palette::Palette(unsigned int size, Colour initialColour) : colours(size)
{
	for (unsigned int i = 0; i < size; i++)
		colours[i] = initialColour;
}

Palette::~Palette() {}

Colour &Palette::GetColour(unsigned int idx)
{
	assert(idx < colours.size());
	return colours[idx];
}

void Palette::SetColour(unsigned int idx, Colour &c)
{
	assert(idx < colours.size());
	colours[idx] = c;
}

}; // namespace OpenApoc
