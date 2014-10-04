#pragma once

#include <cstdint>

namespace OpenApoc {

class Colour
{
	private:
	public:
		Colour(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255)
			: r(r),g(g),b(b),a(a){};
		uint8_t r, g, b, a;
};

}; //namespace OpenApoc
