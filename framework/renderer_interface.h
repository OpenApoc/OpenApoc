#pragma once

namespace OpenApoc
{

class Renderer;

class RendererFactory
{
  public:
	virtual Renderer *create() = 0;
	virtual ~RendererFactory() = default;
};

RendererFactory *getGL20RendererFactory();
RendererFactory *getGLES30RendererFactory();
}; // namespace OpenApoc
