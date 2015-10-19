#include "library/sp.h"
#include "framework/renderer.h"

namespace OpenApoc
{

RendererSurfaceBinding::RendererSurfaceBinding(Renderer &r, sp<Surface> s)
    : prevBinding(r.getSurface()), r(r)
{
	r.setSurface(s);
}

RendererSurfaceBinding::~RendererSurfaceBinding() { r.setSurface(prevBinding); }

Renderer::~Renderer() {}

RendererImageData::~RendererImageData() {}

}; // namespace OpenApoc
