#pragma once

#include "framework/includes.h"

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
};

static_assert(sizeof(Colour) == 4, "Colour should be 4 bytes");

struct Colour_ARGB8888LE
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

}; // namespace OpenApoc
