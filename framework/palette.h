
#pragma once

#include "framework/includes.h"
#include "renderer.h"

namespace OpenApoc
{

class Palette
{
  public:
	std::vector<Colour> colours;
	sp<RendererImageData> rendererPrivateData;

	Palette(unsigned int size = 256, Colour initialColour = {0, 0, 0, 0});
	~Palette();

	const Colour &GetColour(unsigned int idx) const
	{
		assert(idx < colours.size());
		return colours[idx];
	}
	void SetColour(unsigned int idx, Colour c)
	{
		assert(idx < colours.size());
		colours[idx] = std::move(c);
	}

	// Copy constructor copies everything /except/ the renderer private data
	Palette(const Palette &);
};

}; // namespace OpenApoc
