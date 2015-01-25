#include "gl_util.h"
#include "framework/image.h"
#include "framework/includes.h"
#include "game/apocresources/palette.h"

#include <memory>

namespace OpenApoc {

Image::~Image(){}

class RGBImageImpl
{
private:
	friend class RGBImageLock;
	friend class RGBImage;
	bool dirty;
	Vec2<int> size;
	GLuint texHandle;
	bool locked;
	std::vector<Colour> lockedPixels;
	std::shared_ptr<GL::RGBSpriteProgram> program;
public:
	RGBImageImpl(Vec2<int> size)
		: dirty(true),size(size),texHandle(0),locked(false)
	{
		glGenTextures(1, &this->texHandle);
		lockedPixels.resize(size.x*size.y);
		this->program = GL::RGBSpriteProgram::get();
	}
	~RGBImageImpl()
	{
		glDeleteTextures(1, &this->texHandle);
	}
	void update()
	{
		if (!this->dirty)
			return;
		glBindTexture(GL_TEXTURE_2D, this->texHandle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)this->lockedPixels.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		this->dirty = false;
	}
	void draw(GLuint fboTarget, Vec2<int> offset, Vec2<int> size, Vec2<int> screenSize)
	{
		this->update();
		GL::BindTexture t(texHandle, 0);
		this->program->enable(offset, size, screenSize);
		GL::IdentityQuad::draw(this->program->positionLoc);
	}

};

RGBImage::RGBImage(ALLEGRO_BITMAP *bmp)
{
	this->width = al_get_bitmap_width(bmp);
	this->height = al_get_bitmap_height(bmp);
	pimpl.reset(new RGBImageImpl{Vec2<int>{width, height}});

	ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READONLY);

	assert(lr);
	assert(lr->format == ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
	assert(lr->pixel_size == 4);


	Colour *dst = pimpl->lockedPixels.data();
	char *src = (char*)lr->data;

	for (int y = 0; y < pimpl->size.y; y++)
	{
		memcpy(dst, src, pimpl->size.x * sizeof(Colour));
		dst += pimpl->size.x;
		src += lr->pitch;
	}
	al_unlock_bitmap(bmp);
	pimpl->dirty = true;
}

RGBImage::RGBImage(int width, int height, Colour initialColour)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

RGBImage::~RGBImage()
{
}

void RGBImage::draw(float dx, float dy)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void RGBImage::drawRotated(float cx, float cy, float dx, float dy, float angle)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	this->drawScaled(0, 0, pimpl->size.x, pimpl->size.y, dx, dy, pimpl->size.x, pimpl->size.y);
}

void RGBImage::drawScaled(float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	assert(sx == 0);
	assert(sy == 0);
	assert(sw == pimpl->size.x);
	assert(sh == pimpl->size.y);
	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	std::cerr << "al_target_bitmap = 0x" << target << "\n";
	GLuint fbo = al_get_opengl_fbo(target);
	std::cerr << "fbo " << fbo << "\n";

	int screenHeight = al_get_bitmap_height(target);
	int screenWidth = al_get_bitmap_width(target);

	std::cerr << "screen height: " << screenHeight << "\n";
	std::cerr << "screen width : " << screenWidth << "\n";

	std::cerr << "Drawing sprite - size {" << sw << "," << sh <<"}\n";

	pimpl->draw(fbo, Vec2<int>{dx, dy}, Vec2<int>{dw, dh}, Vec2<int>{screenWidth, screenHeight});
	glUseProgram(0);
}

void RGBImage::saveBitmap(const std::string &fileName)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

RGBImageLock::RGBImageLock(std::shared_ptr<RGBImage> img, ImageLockUse use)
	: img(img), use(use)
{
	assert(img->pimpl->locked == false);
	img->pimpl->locked = true;
	if (use != ImageLockUse::Write)
	{
		std::cerr << __PRETTY_FUNCTION__ << "NO READBACK\n";
	}
	if (use == ImageLockUse::Write || use == ImageLockUse::ReadWrite)
	{
		img->pimpl->dirty = true;
	}
}

RGBImageLock::~RGBImageLock()
{
	assert(img->pimpl->locked);
	img->pimpl->locked = false;
	img->pimpl->update();
}

Colour
RGBImageLock::get(int x, int y)
{
	assert(x >= 0);
	assert(x < img->pimpl->size.x);
	assert(y >= 0);
	assert(y < img->pimpl->size.y);
	return img->pimpl->lockedPixels[y*img->pimpl->size.x + x];
}

void
RGBImageLock::set(int x, int y, Colour &c)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

class PaletteImageImpl
{
private:
public:
};

PaletteImageLock::PaletteImageLock(std::shared_ptr<PaletteImage> img, ImageLockUse use)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

PaletteImageLock::~PaletteImageLock()
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

uint8_t
PaletteImageLock::get(int x, int y)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void
PaletteImageLock::set(int x, int y, uint8_t idx)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

PaletteImage::PaletteImage(int width, int height, uint8_t initialIndex)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

PaletteImage::~PaletteImage(){}

//FIXME TESTING-ONLY HACKS

void
PaletteImage::drawRotated(float cx, float cy, float dx, float dy, float angle)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void
PaletteImage::drawScaled(float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void
PaletteImage::draw(float dx, float dy)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void
PaletteImage::saveBitmap(const std::string &filename)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

std::shared_ptr<RGBImage>
PaletteImage::toRGBImage()
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

std::shared_ptr<RGBImage>
PaletteImage::toRGBImage(std::shared_ptr<Palette> p)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

void
PaletteImage::setPalette(std::shared_ptr<Palette> newPal)
{
	std::cerr << __PRETTY_FUNCTION__ << "Not implemented\n";
	assert(0);
}

}; //namespace OpenApoc
