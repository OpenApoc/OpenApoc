#pragma once
#include "library/vec.h"
#include "library/colour.h"
#include <string>
#include <memory>

namespace OpenApoc {

class Image;
class Palette;
class Surface;

class RendererImageData
{
	public:
		virtual ~RendererImageData();
};


class Renderer
{
	private:
		friend class RendererSurfaceBinding;
		virtual void setSurface(std::shared_ptr<Surface> s) = 0;
		virtual std::shared_ptr<Surface> getSurface() = 0;
	public:
		enum class Scaler
		{
			Nearest,
			Linear,
		};
		virtual ~Renderer();
		virtual void clear(Colour c = Colour{0,0,0,0}) = 0;
		virtual void setPalette(std::shared_ptr<Palette> p) = 0;
		virtual void draw(std::shared_ptr<Image> i, Vec2<float> position) = 0;
		virtual void drawRotated(Image &i, Vec2<float> center, Vec2<float> position, float angle) = 0;
		virtual void drawScaled(Image &i, Vec2<float> position, Vec2<float> size, Scaler scaler = Scaler::Linear) = 0;
		virtual void drawTinted(Image &i, Vec2<float> position, Colour tint) = 0;
		virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c) = 0;
		virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0) = 0;
		virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0) = 0;
		virtual void flush() = 0;
		virtual std::string getName() = 0;

		virtual std::shared_ptr<Surface> getDefaultSurface() = 0;
};

class RendererSurfaceBinding
{
	private:
		//Disallow copy
		RendererSurfaceBinding(const RendererSurfaceBinding &) = delete;
		std::shared_ptr<Surface> prevBinding;
		Renderer &r;
	public:
		RendererSurfaceBinding(Renderer& r, std::shared_ptr<Surface> surface);
		~RendererSurfaceBinding();
};

};//namespace openapoc
