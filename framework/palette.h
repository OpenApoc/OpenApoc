
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Palette
{
	private:
		std::vector<Colour> colours;
	public:
		Palette(int size, Colour initialColour = {0,0,0,0});
		~Palette();

		Colour &GetColour(int Index);
		void SetColour(int Index, Colour &Col);
};

}; //namespace OpenApoc
