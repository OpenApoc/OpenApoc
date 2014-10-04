#include "image.h"
#include "includes.h"

namespace OpenApoc {

Image::Image(ALLEGRO_BITMAP *bmp)
	: bmp(bmp), locked(false)
{
	this->height = al_get_bitmap_height(bmp);
	this->width = al_get_bitmap_width(bmp);
}

Image::Image(int width, int height, Colour initialColour)
	: bmp(al_create_bitmap(width, height)), height(height), width(width), locked(false)
{
	ALLEGRO_BITMAP *prevBmp = al_get_target_bitmap();
	al_set_target_bitmap(this->bmp);

	al_clear_to_color(al_map_rgba(initialColour.r, initialColour.g, initialColour.b, initialColour.a));

	al_set_target_bitmap(prevBmp);
}

Image::~Image()
{
	al_destroy_bitmap(this->bmp);
}

void Image::draw(float dx, float dy)
{
	al_draw_bitmap(this->bmp, dx, dy, 0);
}

void Image::drawRotated(float cx, float cy, float dx, float dy, float angle)
{
	al_draw_rotated_bitmap(this->bmp, cx, cy, dx, dy, angle, 0);
}

void Image::drawScaled(float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	al_draw_scaled_bitmap(this->bmp, sx, sy, sw, sh, dx, dy, dw, dh, 0);
}

void Image::saveBitmap(const std::string &fileName)
{
	al_save_bitmap(fileName.c_str(), this->bmp);
}

ImageLock::ImageLock(std::shared_ptr<Image> img)
	: img(img)
{
	assert(img->locked == false);
	img->locked = true;
	region = al_lock_bitmap(img->bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE);
}

ImageLock::~ImageLock()
{
	assert(img->locked == true);
	img->locked = false;
	al_unlock_bitmap(img->bmp);
}

Colour
ImageLock::get(int x, int y)
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
ImageLock::set(int x, int y, Colour &c)
{
	uint8_t *dataPtr = (uint8_t*)this->region->data;
	dataPtr += (this->region->pitch * y);
	dataPtr += (this->region->pixel_size * x);
	*dataPtr++ = c.r;
	*dataPtr++ = c.g;
	*dataPtr++ = c.b;
	*dataPtr++ = c.a;
}

}; //namespace OpenApoc
