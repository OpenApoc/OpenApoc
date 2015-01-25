#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Colour
{
	private:
	public:
		Colour(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255)
			: r(r),g(g),b(b),a(a){};
		uint8_t r, g, b, a;
};

static_assert(sizeof(Colour) == 4, "Colour should be 4 bytes");

}; //namespace OpenApoc
