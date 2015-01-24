#include "image.h"
#include "includes.h"
#include "game/apocresources/palette.h"

namespace OpenApoc {

Image::~Image(){}

RGBImage::RGBImage(ALLEGRO_BITMAP *bmp)
	: bmp(bmp), locked(false)
{
	this->height = al_get_bitmap_height(bmp);
	this->width = al_get_bitmap_width(bmp);
}

RGBImage::RGBImage(int width, int height, Colour initialColour)
	: bmp(al_create_bitmap(width, height)), locked(false)
{
	ALLEGRO_BITMAP *prevBmp = al_get_target_bitmap();
	al_set_target_bitmap(this->bmp);
	this->height = height;
	this->width = width;

	al_clear_to_color(al_map_rgba(initialColour.r, initialColour.g, initialColour.b, initialColour.a));

	al_set_target_bitmap(prevBmp);
}

RGBImage::~RGBImage()
{
	al_destroy_bitmap(this->bmp);
}

void RGBImage::draw(float dx, float dy)
{
	al_draw_bitmap(this->bmp, dx, dy, 0);
}

void RGBImage::drawRotated(float cx, float cy, float dx, float dy, float angle)
{
	al_draw_rotated_bitmap(this->bmp, cx, cy, dx, dy, angle, 0);
}

void RGBImage::drawScaled(float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	al_draw_scaled_bitmap(this->bmp, sx, sy, sw, sh, dx, dy, dw, dh, 0);
}

void RGBImage::saveBitmap(const std::string &fileName)
{
	al_save_bitmap(fileName.c_str(), this->bmp);
}

RGBImageLock::RGBImageLock(std::shared_ptr<RGBImage> img)
	: img(img)
{
	assert(img->locked == false);
	img->locked = true;
	region = al_lock_bitmap(img->bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
}

RGBImageLock::~RGBImageLock()
{
	assert(img->locked == true);
	img->locked = false;
	al_unlock_bitmap(img->bmp);
}

Colour
RGBImageLock::get(int x, int y)
{
	Colour c;
	uint8_t *dataPtr = (uint8_t*)this->region->data;
	dataPtr += (this->region->pitch * y);
	dataPtr += (this->region->pixel_size * x);
	c.r = *dataPtr++;
	c.g = *dataPtr++;
	c.b = *dataPtr++;
	c.a = *dataPtr++;
	return c;
}

void
RGBImageLock::set(int x, int y, Colour &c)
{
	uint8_t *dataPtr = (uint8_t*)this->region->data;
	dataPtr += (this->region->pitch * y);
	dataPtr += (this->region->pixel_size * x);
	*dataPtr++ = c.r;
	*dataPtr++ = c.g;
	*dataPtr++ = c.b;
	*dataPtr++ = c.a;
}

PaletteImageLock::PaletteImageLock(std::shared_ptr<PaletteImage> img)
	: img(img)
{
	assert(img->locked == false);
	img->locked = true;
}

PaletteImageLock::~PaletteImageLock()
{
	assert(img->locked == true);
	img->locked = false;
}

uint8_t
PaletteImageLock::get(int x, int y)
{
	return img->indices[y*img->width + x];
}

void
PaletteImageLock::set(int x, int y, uint8_t idx)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(x < img->width);
	assert(y < img->height);
	img->indices[y*img->width + x] = idx;
}

PaletteImage::PaletteImage(int width, int height, uint8_t initialIndex)
	: indices(width*height), locked(false), pal(nullptr)
{
	this->width = width;
	this->height = height;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
			this->indices[width*y + x] = initialIndex;
	}
}

PaletteImage::~PaletteImage(){}

//FIXME TESTING-ONLY HACKS

void
PaletteImage::drawRotated(float cx, float cy, float dx, float dy, float angle)
{
	this->toRGBImage()->drawRotated(cx, cy, dx, dy, angle);
}

void
PaletteImage::drawScaled(float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	this->toRGBImage()->drawScaled(sx, sy, sw, sh, dx, dy, dw, dh);
}

void
PaletteImage::draw(float dx, float dy)
{
	this->toRGBImage()->draw(dx, dy);
}

void
PaletteImage::saveBitmap(const std::string &filename)
{
	this->toRGBImage()->saveBitmap(filename);
}

std::shared_ptr<RGBImage>
PaletteImage::toRGBImage()
{
	return this->toRGBImage(this->pal);
}

std::shared_ptr<RGBImage>
PaletteImage::toRGBImage(std::shared_ptr<Palette> p)
{
	auto img = std::make_shared<RGBImage>(this->width, this->height);
	RGBImageLock dst(img);
	for (int y = 0; y < this->height; y++)
	{
		for (int x = 0; x < this->width; x++)
		{
			uint8_t idx = this->indices[y*width + x];
			dst.set(x, y, p->GetColour(idx));
		}
	}
	return img;
}

void
PaletteImage::setPalette(std::shared_ptr<Palette> newPal)
{
	this->pal = newPal;
}

}; //namespace OpenApoc
