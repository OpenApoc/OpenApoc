#pragma once
#include "renderer.h"

namespace OpenApoc
{

class RendererFactory
{
  public:
	virtual Renderer *create() = 0;
	virtual ~RendererFactory() {}
};

RendererFactory *getGL20RendererFactory();
RendererFactory *getGL30RendererFactory();
RendererFactory *getGLES30RendererFactory();
}; // namespace OpenApoc
