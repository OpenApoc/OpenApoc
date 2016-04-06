
#pragma once

#include "framework/includes.h"
#include "renderer.h"

namespace OpenApoc
{

class Palette
{
  public:
	std::vector<Colour> colours;
	std::unique_ptr<RendererImageData> rendererPrivateData;

	Palette(unsigned int size = 256, Colour initialColour = {0, 0, 0, 0});
	~Palette();

	Colour &GetColour(unsigned int Index);
	void SetColour(unsigned int Index, Colour Col);

	// Copy constructor copies everything /except/ the renderer private data
	Palette(const Palette &);
};

}; // namespace OpenApoc
