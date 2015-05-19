#include "framework/renderer.h"
#include "framework/image.h"

#include <allegro5/allegro.h>

namespace OpenApoc {
namespace {

class AllegroRendererImageData : public RendererImageData
{
public:
	ALLEGRO_BITMAP *bitmap;
	AllegroRendererImageData(Vec2<int> size)
	{
		bitmap = al_create_bitmap(size.x, size.y);
	}
	virtual ~AllegroRendererImageData()
	{
		if (bitmap)
			al_destroy_bitmap(bitmap);
	}
};

class AllegroRenderer : public Renderer
{
private:
	friend class RendererSurfaceBinding;
	virtual void setSurface(std::shared_ptr<Surface> s);
	virtual std::shared_ptr<Surface> getSurface();
public:
	AllegroRenderer();
	virtual ~AllegroRenderer();
	virtual void clear(Colour c = Colour{0,0,0,0});
	virtual void setPalette(std::shared_ptr<Palette> p);
	virtual void draw(Image &i, Vec2<float> position);
	virtual void drawRotated(Image &i, Vec2<float> center, Vec2<float> position, float angle);
	virtual void drawScaled(Image &i, Vec2<float> position, Vec2<float> size, Scaler scaler = Scaler::Linear);
	virtual void drawTinted(Image &i, Vec2<float> position, Colour tint);
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c);
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0);
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0);
	virtual void flush();
	virtual UString getName();
	virtual Surface *getDefaultSurface();

};

}; //anonymous namespace
OpenApoc::Renderer *
OpenApoc::Renderer::createRenderer()
{
	return new AllegroRenderer();
}
}; //namesapce OpenApoc
