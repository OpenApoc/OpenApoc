#include "framework/palette.h"
#include <cassert>

namespace OpenApoc {

Palette::Palette(int size, Colour initialColour)
	: colours(size)
{
	for (int i = 0; i < size; i++)
		colours[i] = initialColour;
}

Palette::~Palette(){}

Colour&
Palette::GetColour(int idx)
{
	assert(idx < colours.size());
	return colours[idx];
}

void
Palette::SetColour(int idx, Colour &c)
{
	assert(idx < colours.size());
	colours[idx] = c;
}

}; //namespace OpenApoc
