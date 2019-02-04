#pragma once

#include "strings.h"
#include <cstdint>

namespace OpenApoc
{

class Colour
{
  private:
  public:
	Colour(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
	uint8_t r, g, b, a;
	bool operator==(const Colour &other) const
	{
		return (this->r == other.r && this->g == other.g && this->b == other.b &&
		        this->a == other.a);
	}
	bool operator!=(const Colour &other) const { return !(*this == other); }
	// convert html4 color name to a color object (case-insensitive)
	static Colour FromHtmlName(const UString &name);
	// convert a hex triplet such as #ff00ff into a color object (case-insensitive)
	static Colour FromHex(const UString &hexcode);
};

static_assert(sizeof(Colour) == 4, "Colour should be 4 bytes");

struct ColourArgB8888Le
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

static const Colour COLOUR_BLACK{0, 0, 0};
static const Colour COLOUR_RED{255, 0, 0};

}; // namespace OpenApoc
