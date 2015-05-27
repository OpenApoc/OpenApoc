#pragma once
#include "renderer.h"

namespace OpenApoc {

class RendererFactory
{
public:
	virtual Renderer *create() = 0;
	virtual ~RendererFactory() {};
};

void registerRenderer(RendererFactory *factory, UString name);

template <typename T>
class RendererRegister
{
public:
	RendererRegister(UString name)
	{
		registerRenderer(new T, name);
	}
};

}; //namespace OpenApoc
