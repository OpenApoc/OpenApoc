#include "framework/palette.h"

namespace OpenApoc
{

Palette::Palette(unsigned int size, Colour initialColour) : colours(size)
{
	for (unsigned int i = 0; i < size; i++)
		colours[i] = initialColour;
}

Palette::~Palette() = default;

Palette::Palette(const Palette &source) { this->colours = source.colours; }

}; // namespace OpenApoc
