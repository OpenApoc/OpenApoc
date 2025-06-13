#pragma once

#include "library/colour.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <memory>

namespace OpenApoc
{

class Image;
class Palette;
class Surface;

class RendererImageData
{
  public:
	virtual sp<Image> readBack();
	virtual ~RendererImageData();
};

class Renderer
{
  private:
	friend class RendererSurfaceBinding;
	virtual void setSurface(sp<Surface> s) = 0;
	virtual sp<Surface> getSurface() = 0;

  public:
	enum class Scaler
	{
		Nearest,
		Linear,
	};
	virtual ~Renderer();
	virtual void clear(Colour c = Colour{0, 0, 0, 0}) = 0;
	virtual void setPalette(sp<Palette> p) = 0;
	virtual sp<Palette> getPalette() = 0;
	virtual void draw(sp<Image> i, Vec2<float> position) = 0;
	virtual void drawRotated(sp<Image> i, Vec2<float> center, Vec2<float> position,
	                         float angle) = 0;
	virtual void drawScaled(sp<Image> i, Vec2<float> position, Vec2<float> size,
	                        Scaler scaler = Scaler::Linear) = 0;
	virtual void drawTinted(sp<Image> i, Vec2<float> position, Colour tint) = 0;
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c) = 0;
	// drawRect() is expected to grow inwards, so {position, position + size} specifies the bounds
	// of the rect no matter the thickness
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c,
	                      float thickness = 1.0) = 0;
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0) = 0;
	virtual void flush() = 0;
	virtual UString getName() = 0;

	virtual void newFrame(){};

	virtual sp<Surface> getDefaultSurface() = 0;
};

class RendererSurfaceBinding
{
  private:
	// Disallow copy
	RendererSurfaceBinding(const RendererSurfaceBinding &) = delete;
	sp<Surface> prevBinding;
	Renderer &r;

  public:
	RendererSurfaceBinding(Renderer &r, sp<Surface> surface);
	~RendererSurfaceBinding();
};

}; // namespace OpenApoc
