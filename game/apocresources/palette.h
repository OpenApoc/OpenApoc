
#pragma once

#include "framework/includes.h"

namespace OpenApoc {

class Data;
class RGBImage;

class Palette
{
	private:
		std::unique_ptr<Colour[]> colours;
		std::shared_ptr<RGBImage> img;

	public:
		Palette( Data &d, std::string Filename );
		~Palette();

		Colour &GetColour(int Index);
		void SetColour(int Index, Colour &Col);
		std::shared_ptr<RGBImage> getImage();

		void DumpPalette( std::string Filename );
};

}; //namespace OpenApoc
