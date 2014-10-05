
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Framework;

class Palette
{
	private:
		std::unique_ptr<Colour[]> colours;

	public:
		Palette( Framework &fw, std::string Filename );
		~Palette();

		Colour &GetColour(int Index);
		void SetColour(int Index, Colour &Col);

		void DumpPalette( std::string Filename );
};

}; //namespace OpenApoc
