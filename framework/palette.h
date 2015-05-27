
#pragma once

#include "framework/includes.h"
#include "image.h"
#include "renderer.h"

namespace OpenApoc {

class Palette
{
	public:
		std::vector<Colour> colours;
		std::unique_ptr<RendererImageData> rendererPrivateData;

		Palette(unsigned int size, Colour initialColour = {0,0,0,0});
		~Palette();

		Colour &GetColour(unsigned int Index);
		void SetColour(unsigned int Index, Colour &Col);
};

}; //namespace OpenApoc
